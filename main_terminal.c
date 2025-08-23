#include "sokoban.h"

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

int nanosleep(const struct timespec *req, struct timespec *rem);
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_WALL    "\033[44;94m"  // Bright blue foreground
#define COLOR_PLAYER  "\033[96m"     // Bright cyan foreground
#define COLOR_BOX     "\033[93m"     // Bright yellow foreground
#define COLOR_GOAL    "\033[43m"     // Dark yellow background
#define COLOR_PLAYER_ON_GOAL "\033[43;96m" // Dark yellow background, bright cyan foreground
#define COLOR_BOX_ON_GOAL "\033[43;93m"    // Dark yellow background, yellow foreground
#define COLOR_ICE "\033[46;97m"            // Dark cyan background, dark white foreground
#define COLOR_PLAYER_ON_ICE "\033[46;96m"  // Dark cyan background, bright cyan foreground
#define COLOR_BOX_ON_ICE "\033[46;93m"     // Dark cyan background, bright yellow foreground

// milliseconds to wait before running a "tic" during game events
#define TIC_DURATION_MS 100

void clear_screen(void) {
#ifdef _WIN32
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdOut, &csbi);
    DWORD count;
    DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;
    COORD homeCoords = { 0, 0 };
    FillConsoleOutputCharacter(hStdOut, ' ', cellCount, homeCoords, &count);
    SetConsoleCursorPosition(hStdOut, homeCoords);
#else
    printf("\033[H\033[J");
#endif
}

void delay(int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    nanosleep(&(struct timespec) { .tv_sec = milliseconds / 1000,
                                   .tv_nsec = (milliseconds % 1000) * 1000000
                                 }, NULL);
#endif
}

void print_board(GameState *state) {
    //clear_screen();
    for (int i = 0; i < state->rows; i++) {
        for (int j = 0; j < state->cols; j++) {
            switch (state->board[i][j]) {
                case WALL:
                    printf("%s%c%s", COLOR_WALL, WALL, COLOR_RESET);
                    break;
                case PLAYER:
                    printf("%s%c%s", COLOR_PLAYER, PLAYER, COLOR_RESET);
                    break;
                case BOX:
                    printf("%s%c%s", COLOR_BOX, BOX, COLOR_RESET);
                    break;
                case GOAL:
                    printf("%s%c%s", COLOR_GOAL, GOAL, COLOR_RESET);
                    break;
                case PLAYER_ON_GOAL:
                    printf("%s%c%s", COLOR_PLAYER_ON_GOAL, PLAYER_ON_GOAL, COLOR_RESET);
                    break;
                case BOX_ON_GOAL:
                    printf("%s%c%s", COLOR_BOX_ON_GOAL, BOX_ON_GOAL, COLOR_RESET);
                    break;
				case ICE:
					printf("%s%c%s", COLOR_ICE, ICE, COLOR_RESET);
					break;
				case PLAYER_ON_ICE:
					printf("%s%c%s", COLOR_PLAYER_ON_ICE, PLAYER_ON_ICE, COLOR_RESET);
					break;
				case BOX_ON_ICE:
					printf("%s%c%s", COLOR_BOX_ON_ICE, BOX_ON_ICE, COLOR_RESET);
					break;
                default:
                    printf("%c", state->board[i][j]);
                    break;
            }
        }
        printf("\n");
    }
}

int getch_noblock(void) {
#ifdef _WIN32
    if (_kbhit()) {
        return _getch();
    }
    return -1;
#else
    struct termios oldt, newt;
    int ch;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    // Handle arrow keys (ESC [ A, ESC [ B, etc.)
    if (ch == 27) { // ESC
        // Peek at the next two characters
        int next1 = getchar();
        int next2 = getchar();
        if (next1 == 91) { // [
            switch (next2) {
                case 65: // Up
                    return 'w';
                case 66: // Down
                    return 's';
                case 67: // Right
                    return 'd';
                case 68: // Left
                    return 'a';
            }
        }
    }
    return ch;
#endif
}

void enable_raw_mode(void) {
#ifndef _WIN32
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
#endif
}

void disable_raw_mode(void) {
#ifndef _WIN32
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
#endif
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <level_file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    GameState state;
    init_move_history(&state.history);
	
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    load_level(&state, file);
    fclose(file);

#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#else
    enable_raw_mode();
#endif

    print_board(&state);

    while (true) {
        int input = getch_noblock();
        if (input == 'q') {
            break;
        }

        bool updated = false;
        switch (input) {
            case 'w':
            case 'k':
#ifdef _WIN32
            case 72:
#endif
                updated = move_player(&state, -1, 0);
                break;
            case 'a':
            case 'h':
#ifdef _WIN32
            case 75:
#endif
                updated = move_player(&state, 0, -1);
                break;
            case 's':
            case 'j':
#ifdef _WIN32
            case 80:
#endif
                updated = move_player(&state, 1, 0);
                break;
            case 'd':
            case 'l':
#ifdef _WIN32
            case 77:
#endif
                updated = move_player(&state, 0, 1);
                break;
            case 'r':
				
				FILE *file = fopen(argv[1], "r");
				if (!file) {
					perror("Error opening file");
					exit(EXIT_FAILURE);
				}
                reset_game(&state, file);
                fclose(file);
                updated = true;
                break;
            case 'u':
                undo_move(&state);
                updated = true;
                break;
            default:
                continue;
        }
        if (updated) {
            print_board(&state);
		}
			
		// If there's an ongoing event, process it and redraw each gametic accordingly until resolved
		while (state.event.type != EVENT_NONE) {
			//printf("event! %dx%d (%d,%d)\n", state.event.x, state.event.y, state.event.dr, state.event.dc);
			delay(TIC_DURATION_MS);
			if (process_event(&state)) {
			  print_board(&state);
			}
		}
		
		if (is_game_won(&state)) {
			printf("Congratulations! You won!\n");
			printf("Total steps taken: %zu\n", state.history.size);
			printf("Move history: %.*s\n", (int)state.history.size, state.history.moves);
			break;
		}
    }

#ifndef _WIN32
    disable_raw_mode();
#endif

    clear_move_history(&state.history);
    return EXIT_SUCCESS;
}

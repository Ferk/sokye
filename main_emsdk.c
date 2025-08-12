#include "sokoban.h"
#include <stdio.h>
#include <stdlib.h>
#include <emscripten/emscripten.h>

// Global game state object
GameState game_state;
const char* current_level_data;

// Exported functions for JavaScript to call
EMSCRIPTEN_KEEPALIVE
void sokoban_init_web(const char* level_data) {
    current_level_data = level_data;
    init_move_history(&game_state.history);
    // In a real application, you would parse the level data string
    // instead of reading a file. We'll simulate this with a simple
    // string copy.
    // For this example, we assume `level_data` points to a static
    // string or one that persists.
    game_state.rows = 0;
    game_state.cols = 0;
    int row = 0, col = 0;
    for (const char* p = level_data; *p != '\0'; p++) {
        if (*p == '\n') {
            game_state.rows++;
            if (col > game_state.cols) {
                game_state.cols = col;
            }
            col = 0;
            if (game_state.rows >= MAX_ROWS) break;
            continue;
        }
        if (*p == PLAYER || *p == PLAYER_ON_GOAL) {
            game_state.player_row = row;
            game_state.player_col = col;
        }
        game_state.board[row][col] = *p;
        col++;
    }
}

EMSCRIPTEN_KEEPALIVE
void sokoban_handle_input(char input) {
    int dr = 0, dc = 0;
    bool updated = false;

    switch (input) {
        case 'w':
        case 'k':
            dr = -1; dc = 0;
            break;
        case 'a':
        case 'h':
            dr = 0; dc = -1;
            break;
        case 's':
        case 'j':
            dr = 1; dc = 0;
            break;
        case 'd':
        case 'l':
            dr = 0; dc = 1;
            break;
        case 'u':
            undo_move(&game_state);
            updated = true;
            break;
        case 'r':
            // For the web version, a "reset" means re-initializing from the stored level data
            sokoban_init_web(current_level_data);
            updated = true;
            break;
    }

    if (!updated && (dr != 0 || dc != 0)) {
        updated = move_player(&game_state, dr, dc);
    }
}

EMSCRIPTEN_KEEPALIVE
int sokoban_get_rows(void) {
    return game_state.rows;
}

EMSCRIPTEN_KEEPALIVE
int sokoban_get_cols(void) {
    return game_state.cols;
}

EMSCRIPTEN_KEEPALIVE
char sokoban_get_tile(int row, int col) {
    return get_tile(&game_state, row, col);
}

EMSCRIPTEN_KEEPALIVE
bool sokoban_is_game_won(void) {
    return is_game_won(&game_state);
}

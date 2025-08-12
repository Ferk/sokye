#include "sokoban.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define IS_WALL(x,y) \
  (x < 0 || x >= state->rows || y < 0 || y >= state->cols || state->board[x][y] == WALL)
#define IS_BOX(x,y) \
  ((state->board[x][y] == BOX) || (state->board[x][y] == BOX_ON_GOAL))
#define IS_PLAYER(x,y) \
  ((state->board[x][y] == PLAYER) || (state->board[x][y] == PLAYER_ON_GOAL))
#define REMOVE_BOX(x,y) \
  state->board[x][y] = ((state->board[x][y] == BOX_ON_GOAL)? GOAL : FLOOR)
#define REMOVE_PLAYER(x,y) \
  state->board[x][y] = ((state->board[x][y] == PLAYER_ON_GOAL)? GOAL : FLOOR)
#define ADD_BOX(x,y) \
  state->board[x][y] = ((state->board[x][y] == GOAL)? BOX_ON_GOAL : BOX)
#define ADD_PLAYER(x,y) \
  state->board[x][y] = ((state->board[x][y] == GOAL)? PLAYER_ON_GOAL : PLAYER)

static void add_move(MoveHistory *history, char move) {
    if (history->size >= history->capacity) {
        history->capacity *= 2;
        history->moves = (char *)realloc(history->moves, history->capacity * sizeof(char));
    }
    history->moves[history->size++] = move;
}

static void pop_move(MoveHistory *history) {
    if (history->size > 0) {
        history->size--;
    }
}

void init_move_history(MoveHistory *history) {
    history->capacity = 10;
    history->size = 0;
    history->moves = (char *)malloc(history->capacity * sizeof(char));
}

void clear_move_history(MoveHistory *history) {
    free(history->moves);
    history->moves = NULL;
    history->size = 0;
    history->capacity = 0;
}

void load_level(GameState *state, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        // This is a terminal-specific error, but for the sake of the example,
        // we keep a simple fopen. In a web version, the file would be loaded
        // differently (e.g., passed as a string).
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    state->rows = 0;
    state->cols = 0;
    char line[MAX_COLS + 2];
    while (fgets(line, sizeof(line), file)) {
        int j = 0;
        while (line[j] != '\n' && line[j] != '\0') {
            switch(line[j]) {
              case PLAYER:
              case PLAYER_ON_GOAL:
                state->player_row = state->rows;
                state->player_col = j;
				/* FALLTHROUGH -- avoid compiler warnings */
              case WALL:
              case GOAL:
              case BOX:
              case BOX_ON_GOAL:
                state->board[state->rows][j] = line[j];
                break;
              default:
                state->board[state->rows][j] = FLOOR;
            }
            j++;
        }
        if (state->cols < j) state->cols = j;
        for(; j < MAX_COLS; j++) {
            state->board[state->rows][j] = ' ';
        }
        state->rows++;
    }
    fclose(file);
}

void reset_game(GameState *state, const char *filename) {
    clear_move_history(&state->history);
    init_move_history(&state->history);
    load_level(state, filename);
}

bool is_game_won(GameState *state) {
    for (int i = 0; i < state->rows; i++) {
        for (int j = 0; j < state->cols; j++) {
            if (state->board[i][j] == GOAL || state->board[i][j] == PLAYER_ON_GOAL) {
                return false;
            }
        }
    }
    return true;
}

bool move_player(GameState *state, int dr, int dc) {
    int new_row = state->player_row + dr;
    int new_col = state->player_col + dc;

    if (IS_WALL(new_row, new_col)) {
        return false;
    }

    bool box_pushed = false;
    if (IS_BOX(new_row, new_col)) {
        int box_row = new_row + dr;
        int box_col = new_col + dc;
        if (IS_WALL(box_row, box_col) || IS_BOX(box_row, box_col)) {
            return false;
        }
        ADD_BOX(box_row, box_col);
        box_pushed = true;
    }

    REMOVE_PLAYER(state->player_row, state->player_col);
    ADD_PLAYER(new_row, new_col);
    state->player_row = new_row;
    state->player_col = new_col;

    if (box_pushed) {
        if (dr == -1) add_move(&state->history, 'U');
        else if (dr == 1) add_move(&state->history, 'D');
        else if (dc == -1) add_move(&state->history, 'L');
        else if (dc == 1) add_move(&state->history, 'R');
    } else {
        if (dr == -1) add_move(&state->history, 'u');
        else if (dr == 1) add_move(&state->history, 'd');
        else if (dc == -1) add_move(&state->history, 'l');
        else if (dc == 1) add_move(&state->history, 'r');
    }

    return true;
}

void undo_move(GameState *state) {
    if (state->history.size == 0) {
        return;
    }

    char last_move = state->history.moves[state->history.size - 1];
    pop_move(&state->history);

    int dr = 0, dc = 0;
    bool box_pushed = isupper(last_move);
    last_move = tolower(last_move);

    switch (last_move) {
        case 'u': dr = 1; break;
        case 'd': dr = -1; break;
        case 'l': dc = 1; break;
        case 'r': dc = -1; break;
    }
    int new_row = state->player_row + dr;
    int new_col = state->player_col + dc;

    if (IS_WALL(new_row, new_col)) {
        return;
    }
    
    REMOVE_PLAYER(state->player_row, state->player_col);
    if (box_pushed) {
        int box_row = state->player_row - dr;
        int box_col = state->player_col - dc;
        REMOVE_BOX(box_row, box_col);
        ADD_BOX(state->player_row, state->player_col);
    }
    ADD_PLAYER(new_row, new_col);

    state->player_row = new_row;
    state->player_col = new_col;
}

char get_tile(GameState *state, int row, int col) {
    if (row >= 0 && row < state->rows && col >= 0 && col < state->cols) {
        return state->board[row][col];
    }
    return ' ';
}

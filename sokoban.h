#ifndef SOKOBAN_H
#define SOKOBAN_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_ROWS 64
#define MAX_COLS 64

// Define constants for the characters
#define WALL '#'
#define PLAYER '@'
#define PLAYER_ON_GOAL '+'
#define BOX '$'
#define BOX_ON_GOAL '*'
#define GOAL '.'
#define FLOOR ' '

typedef struct {
    char *moves;
    size_t size;
    size_t capacity;
} MoveHistory;

typedef struct {
    char board[MAX_ROWS][MAX_COLS];
    int rows, cols;
    int player_row, player_col;
    MoveHistory history;
} GameState;

void init_move_history(MoveHistory *history);
void clear_move_history(MoveHistory *history);

// Core game logic functions
void load_level(GameState *state, const char *filename);
bool is_game_won(GameState *state);
bool move_player(GameState *state, int dr, int dc);
void undo_move(GameState *state);
char get_tile(GameState *state, int row, int col);
void reset_game(GameState *state, const char *filename);

#endif // SOKOBAN_H

#include "sokoban.h"
#include <emscripten/emscripten.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global game state object
GameState game_state;

void load_level_from_string(const char *level_string) {
  game_state.rows = 0;
  game_state.cols = 0;
  game_state.event.type = EVENT_NONE;
  int row = 0, col = 0;
  for (const char *p = level_string; *p != '\0'; p++) {
    if (*p == '\n') {
      game_state.rows++;
      if (col > game_state.cols) {
        game_state.cols = col;
      }
      col = 0;
      row++;
      if (game_state.rows >= MAX_ROWS) break;
      continue;
    }
    if (*p == PLAYER || *p == PLAYER_ON_GOAL || *p == PLAYER_ON_ICE) {
      game_state.player_row = row;
      game_state.player_col = col;
    }
    game_state.board[row][col] = *p;
    col++;
  }
  // Final check for the last row if there's no trailing newline
  if (col > game_state.cols) {
    game_state.cols = col;
  }
  game_state.rows++;
  printf("%dx%d level initialized!\n%s\n", game_state.rows, game_state.cols, level_string);
}

void sokoban_reset_web(void) {
  reset_game(&game_state);
}

// Exported functions for JavaScript to call
EMSCRIPTEN_KEEPALIVE
void sokoban_init_web(const char *level_data) {
  init_move_history(&game_state.history);
  load_level_from_string(level_data);
  remember_initial_state(&game_state);
}

EMSCRIPTEN_KEEPALIVE
bool sokoban_handle_input(char input) {
  int dr = 0, dc = 0;
  bool updated = false;

  switch (input) {
    case 'w':
    case 'k':
      dr = -1;
      dc = 0;
      break;
    case 'a':
    case 'h':
      dr = 0;
      dc = -1;
      break;
    case 's':
    case 'j':
      dr = 1;
      dc = 0;
      break;
    case 'd':
    case 'l':
      dr = 0;
      dc = 1;
      break;
    case 'u':
      undo_move(&game_state);
      updated = true;
      break;
    case 'r':
      sokoban_reset_web();
      updated = true;
      break;
  }

  if (!updated && (dr != 0 || dc != 0)) {
    updated = move_player(&game_state, dr, dc);
  }
  return updated;
}

EMSCRIPTEN_KEEPALIVE
bool sokoban_is_event_ongoing(void) {
  return (game_state.event.type != EVENT_NONE);
}

EMSCRIPTEN_KEEPALIVE
bool sokoban_process_event(void) {
  return process_event(&game_state);
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

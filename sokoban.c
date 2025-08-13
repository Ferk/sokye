#include "sokoban.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* How many history items would be allocated each time */
#define HISTORY_ALLOCATION_INCREMENT 16



#define IS_WALL(x,y) \
  (x < 0 || x >= state->rows || y < 0 || y >= state->cols || state->board[x][y] == WALL)
#define IS_BOX(x,y) \
  ((state->board[x][y] == BOX) || (state->board[x][y] == BOX_ON_GOAL) || (state->board[x][y] == BOX_ON_ICE))
#define IS_PLAYER(x,y) \
  ((state->board[x][y] == PLAYER) || (state->board[x][y] == PLAYER_ON_GOAL) || (state->board[x][y] == PLAYER_ON_ICE))
#define IS_ICE(x, y) \
  ((state->board[x][y] == ICE) || (state->board[x][y] == PLAYER_ON_ICE) || (state->board[x][y] == BOX_ON_ICE))

#define REMOVE_BOX(x, y) \
  state->board[x][y] = ((state->board[x][y] == BOX_ON_GOAL) ? GOAL : ((state->board[x][y] == BOX_ON_ICE) ? ICE : FLOOR))
#define REMOVE_PLAYER(x, y) \
  state->board[x][y] = ((state->board[x][y] == PLAYER_ON_GOAL) ? GOAL : ((state->board[x][y] == PLAYER_ON_ICE) ? ICE : FLOOR))
  
#define ADD_BOX(x, y) \
  state->board[x][y] = ((state->board[x][y] == GOAL) ? BOX_ON_GOAL : ((state->board[x][y] == ICE) ? BOX_ON_ICE : BOX))
#define ADD_PLAYER(x, y) \
  state->board[x][y] = ((state->board[x][y] == GOAL) ? PLAYER_ON_GOAL : ((state->board[x][y] == ICE) ? PLAYER_ON_ICE : PLAYER))


static void add_move(MoveHistory *history, char move) {
    if ((history->size + 1) % HISTORY_ALLOCATION_INCREMENT == 0) {
        history->moves = (char *)realloc(history->moves, (history->size + HISTORY_ALLOCATION_INCREMENT) * sizeof(char));
    }
    history->moves[history->size++] = move;
}

static void pop_move(MoveHistory *history) {
    if (history->size > 0) {
        history->size--;
    }
}

void init_move_history(MoveHistory *history) {
    history->size = 0;
    history->moves = (char *)malloc(HISTORY_ALLOCATION_INCREMENT * sizeof(char));
}

void clear_move_history(MoveHistory *history) {
	if(history->moves != NULL) {
        free(history->moves);
        history->moves = NULL;
	}
    history->size = 0;
}

void load_level(GameState *state, FILE *file) {
    if (!file) {
        // This is a terminal-specific error, but for the sake of the example,
        // we keep a simple fopen. In a web version, the file would be loaded
        // differently (e.g., passed as a string).
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
	state->event.type = EVENT_NONE;
    state->rows = 0;
    state->cols = 0;
    char line[MAX_COLS + 2];
    while (fgets(line, sizeof(line), file)) {
        int j = 0;
        while (line[j] != '\n' && line[j] != '\0') {
            switch(line[j]) {
              case PLAYER:
              case PLAYER_ON_GOAL:
              case PLAYER_ON_ICE:
                state->player_row = state->rows;
                state->player_col = j;
				/* FALLTHROUGH -- avoid compiler warnings */
              case WALL:
              case GOAL:
			  case ICE:
              case BOX:
              case BOX_ON_GOAL:
			  case BOX_ON_ICE:
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
}

void reset_game(GameState *state, FILE *file) {
    clear_move_history(&state->history);
    init_move_history(&state->history);
    load_level(state, file);
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

bool move_on_ice(GameState *state) {
	bool redraw = false;
	
	int new_x = state->event.x + state->event.dr;
	int new_y = state->event.y + state->event.dc;

    if (IS_BOX(new_x, new_y)) {
		// hit a box! event transfers to the box for next tic
		// (it may chain if there's a stack of boxes!)
		state->event.x = new_x;
		state->event.y = new_y;
		new_x = state->event.x + state->event.dr;
	    new_y = state->event.y + state->event.dc;
		
    } else if (IS_WALL(new_x, new_y)) {
		// hit a wall! stop
		state->event.type = EVENT_NONE;
	} else {
		// Move the player or the box
		if (IS_PLAYER(state->event.x, state->event.y)) {
			REMOVE_PLAYER(state->event.x, state->event.y);
			ADD_PLAYER(new_x, new_y);
			
			state->player_row = new_x;
			state->player_col = new_y;			
		}
		else if (IS_BOX(state->event.x, state->event.y)) {
			REMOVE_BOX(state->event.x, state->event.y);
			ADD_BOX(new_x, new_y);
		}
		redraw = true;

		if (!IS_ICE(new_x, new_y)) {
			// no longer slippery
			state->event.type = EVENT_NONE;
		} else {
			// update position for next event
			state->event.x = new_x;
			state->event.y = new_y;
		}
	}
	return redraw;
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
        REMOVE_BOX(new_row, new_col);
        ADD_BOX(box_row, box_col);
        box_pushed = true;
		if(state->board[box_row][box_col] == BOX_ON_ICE) {
			state->event.type = EVENT_ICE_MOVE;
			state->event.x = box_row;
			state->event.y = box_col;
			state->event.dr = dr;
			state->event.dc = dc;
			move_on_ice(state);
		}
    }

    REMOVE_PLAYER(state->player_row, state->player_col);
    ADD_PLAYER(new_row, new_col);
    state->player_row = new_row;
    state->player_col = new_col;
	
	if(state->board[new_row][new_col] == PLAYER_ON_ICE) {
		state->event.type = EVENT_ICE_MOVE;
		state->event.x = new_row;
		state->event.y = new_col;
		state->event.dr = dr;
		state->event.dc = dc;
        move_on_ice(state);
	}

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

/* Advance a game tic forwards */
bool process_event(GameState *state) {
	
	bool redraw = false;
	switch(state->event.type) {
	  case EVENT_ICE_MOVE:
	    redraw = move_on_ice(state);
	    break;
	  case EVENT_NONE:
	    break;
	  default:
	    perror("Unknown event type");
	    state->event.type = EVENT_NONE;
	}
    return redraw;
}

char get_tile(GameState *state, int row, int col) {
    if (row >= 0 && row < state->rows && col >= 0 && col < state->cols) {
        return state->board[row][col];
    }
    return '\0';
}

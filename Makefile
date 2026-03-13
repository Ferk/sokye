# Compiler and flags
CC = gcc
EMCC = emcc
CFLAGS = -Wall -Wextra -pedantic -std=c11
EMCCFLAGS = -s EXPORTED_FUNCTIONS="['_sokoban_init_web', '_sokoban_handle_input', '_sokoban_get_rows', '_sokoban_get_cols', '_sokoban_get_tile', '_sokoban_is_game_won']" -s EXPORTED_RUNTIME_METHODS="['cwrap']"

# Source files
TERMINAL_SRCS = sokoban.c main_terminal.c
WEB_SRCS = sokoban.c main_emsdk.c

# Object files
TERMINAL_OBJS = $(TERMINAL_SRCS:.c=.o)

# Executable names
TERMINAL_EXE = sokterm
WEB_JS = web/sokoban.js

.PHONY: all terminal web serve format clean

all: terminal web

terminal: $(TERMINAL_EXE)

web: $(WEB_JS)


%.o: %.c sokoban.h
	$(CC) $(CFLAGS) -c $< -o $@

$(TERMINAL_EXE): $(TERMINAL_SRCS)
	$(CC) $(CFLAGS) $^ -o $(TERMINAL_EXE)

$(WEB_JS): $(WEB_SRCS)
	$(EMCC) $(CFLAGS) $(EMCCFLAGS) $^ -o $(WEB_JS)

# Run a local web server (Python 3)
# To use Python 2, replace `python3 -m http.server` with `python -m SimpleHTTPServer`
serve: web
	@echo "Starting web server at http://localhost:8000"
	@python3 -m http.server --directory web/

format:
	clang-format --style=file -i *.c *.h
	perl -0pi -e 's/[ \t]+(\n)/$$1/g' web/*.js

clean:
	rm -f $(TERMINAL_EXE) $(TERMINAL_OBJS) $(WEB_JS) web/sokoban.wasm

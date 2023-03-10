BIN = build/edzin
CFLAGS =  -Wall -Wextra -Werror -pedantic -std=c99
TARGETS = src/main.c src/handlers.c src/ui.c src/lexer.c src/highlight.c src/smartbuf.c src/buffer.c src/window.c
OPTIONS = -DUO_ENABLE_ARROW_KEYS \
		  -DUO_CONTINUE_SCROLL_ON_LEFT \
		  -DUO_CONTINUE_SCROLL_ON_RIGHT \
		  -DUO_ENABLE_DELETE_LINE_JOIN

.PHONY: build
build: 
	$(CC) $(TARGETS) -o $(BIN) $(CFLAGS) -g -O0

.PHONY: uobuild
uobuild: 
	$(CC) $(TARGETS) -o $(BIN) $(CFLAGS) $(OPTIONS)

OBJ = $(shell find src -type f -iname '*.h' -or -iname '*.c')

.PHONY: lint
lint: $(OBJ)
	@clang-format -style=file -i $(OBJ)
	@echo "reformatted successfully"

T_BIN = build/_t
T_LIBS = -lcheck -lm -lpthread -lrt -lsubunit
T_OPTIONS = -DTEST

.PHONY: test
test:
	@$(CC) $(TARGETS) test/main.c -o $(T_BIN) $(T_LIBS) $(T_OPTIONS)

.PHONY: uotest
uotest:
	@$(CC) $(TARGETS) test/main.c -o $(T_PATH) $(T_LIBS) $(OPTIONS) $(T_OPTIONS)


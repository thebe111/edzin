DEBUG = 1
ENABLE_UO = 0
BIN = build/edzin
CFLAGS =  -Wall -Wextra -Werror -pedantic -std=c99
TARGETS = src/main.c src/handlers.c src/ui.c src/lexer.c src/highlight.c src/smartbuf.c src/buffer.c src/window.c
OPTIONS = -DUO_ENABLE_ARROW_KEYS \
		  -DUO_CONTINUE_SCROLL_ON_LEFT \
		  -DUO_CONTINUE_SCROLL_ON_RIGHT \
		  -DUO_ENABLE_DELETE_LINE_JOIN

ifeq ($(DEBUG),1)
	CFLAGS += -g -O0
endif

ifeq ($(ENABLE_UO),1)
	CFLAGS += $(OPTIONS)
endif

.PHONY: build
build: 
	$(CC) $(TARGETS) -o $(BIN) $(CFLAGS)

OBJ = $(shell find src -type f -iname '*.h' -or -iname '*.c')

.PHONY: lint
lint: $(OBJ)
	@clang-format -style=file -i $(OBJ)
	@echo "reformatted successfully"

T_BIN = build/_t
T_LIBS = -lcheck -lm -lpthread -lrt -lsubunit
T_OPTIONS = -DTEST

ifeq ($(ENABLE_UO),1)
	T_LIBS += $(OPTIONS) $(T_OPTIONS)
endif

.PHONY: test
test:
	@$(CC) $(TARGETS) test/main.c -o $(T_BIN) $(T_LIBS) $(T_OPTIONS)


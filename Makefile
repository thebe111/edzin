CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99
TARGETS = \
		  src/main.c \
		  src/handlers.c \
		  src/ui.c 

OPTIONS = \
		  -DUO_ENABLE_ARROW_KEYS \
		  -DUO_CONTINUE_SCROLL_ON_LEFT \
		  -DUO_CONTINUE_SCROLL_ON_RIGHT

.PHONY:
build: src/main.c src/handlers.c src/ui.c
	$(CC) $(TARGETS) -o build/edzin $(CFLAGS) # $(OPTIONS)

OBJ = $(shell find src -type f -iname '*.h' -or -iname '*.c')

.PHONY:
lint: $(OBJ)
	@clang-format -style=file -i $(OBJ)
	@echo "reformatted successfully"

LIBS = -lcheck -lm -lpthread -lrt -lsubunit

.PHONY:
test:
	$(CC) test/main.c -o build/_t $(LIBS)

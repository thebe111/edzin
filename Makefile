OBJ = $(shell find src -type f -iname '*.h' -or -iname '*.c')

.PHONY:
build: src/main.c src/handlers.c src/edzin/main.h src/edzin/handlers.h
	$(CC) \
		src/main.c \
		src/handlers.c \
		src/edzin/main.h \
		src/edzin/handlers.h \
		-o build/edzin \
		-Wall \
		-Wextra \
		-pedantic \
		-std=c99 
		# -DUO_ENABLE_ARROW_KEYS \
		# -DUO_CONTINUE_SCROLL_ON_LEFT \
		# -DUO_CONTINUE_SCROLL_ON_RIGHT

.PHONY:
lint: $(OBJ)
	@clang-format -style=file -i $(OBJ)
	@echo "reformatted successfully"

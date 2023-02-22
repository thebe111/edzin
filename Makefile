OBJ = $(shell find src -type f -iname '*.h' -or -iname '*.c')

.PHONY:
build: src/main.c src/user_options.c src/edzin/main.h src/edzin/user_options.h
	$(CC) \
		src/main.c \
		src/user_options.c \
		src/edzin/main.h \
		src/edzin/user_options.h \
		-o build/edzin \
		-Wall \
		-Wextra \
		-pedantic \
		-std=c99 
		# -DUO_ARROW_KEYS

.PHONY:
lint: $(OBJ)
	@clang-format -style=file -i $(OBJ)
	@echo "reformatted successfully"

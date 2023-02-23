# Edzin

An vim like editor for educational purposes (based on a tutorial)

### How To Run

```
$ make -B build
$ ./build/edzin
```

### User Options Flags

> default values

- `UO_ENABLE_ARROW_KEYS`: false
- `UO_CONTINUE_SCROLL_ON_LEFT`: false
- `UO_CONTINUE_SCROLL_ON_RIGHT`: false

### Memory Leaks

To prevent memory leaks was used a tool called [valgrind](https://valgrind.org/docs/manual/quick-start.html)

```
$ make -B build && valgrind --leak-check=full --show-leak-kinds=all -s ./build/edzin src/main.c
```

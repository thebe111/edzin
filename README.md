# Edzin

A vim like editor for educational purposes (based on a tutorial)

### How To Run

```
# with user options
$ make uobuild
$ ./build/edzin

# without user options 
$ make build
$ ./build/edzin
```

### How To Test

```
# with user options
$ make uotest 
$ ./build/_t

# without user options
$ make test
$ ./build/_t
```

### User Options Flags

> default values

- `UO_ENABLE_ARROW_KEYS`: false
- `UO_CONTINUE_SCROLL_ON_LEFT`: false
- `UO_CONTINUE_SCROLL_ON_RIGHT`: false
- `UO_ENABLE_DELETE_LINE_JOIN`: false

### Memory Leaks

To prevent memory leaks was used [valgrind](https://valgrind.org/docs/manual/quick-start.html)

```
$ make build && valgrind --leak-check=full --show-leak-kinds=all -s ./build/edzin src/main.c
```

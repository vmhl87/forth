all:
	gcc main.c -o run && ./run

debug:
	gcc main.c -g -o run && gdb run

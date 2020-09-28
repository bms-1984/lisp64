lisp64: lisp64.c mpc.c
	$(CC) -Wall $^ -lm -o $@

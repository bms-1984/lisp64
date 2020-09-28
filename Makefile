lisp64: lisp64.c mpc.c mpc.h
	$(CC) -Wall lisp64.c mpc.c -lm -o $@

CC=gcc

skedia: skedia.o graph.o expr.o expr_builtins.o
	$(CC) -o skedia skedia.o graph.o expr.o expr_builtins.o -lcurses -lm

skedia.o: skedia.c
	$(CC) -c skedia.c

graph.o: graph.c graph.h
	$(CC) -c graph.c


expr.o: expr.c expr.h
	$(CC) -c expr.c

expr_builtins.o : expr_builtins.c expr.h
	$(CC) -c expr_builtins.c


clean:
	rm *.o
	rm skedia

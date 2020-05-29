CC=gcc

skedia: skedia.o graph.o gallery.o expr.o expr_builtins.o
	$(CC) -o skedia skedia.o graph.o gallery.o expr.o expr_builtins.o -lcurses -lm

skedia.o: skedia.c
	$(CC) -c skedia.c

gallery.o: gallery.c gallery.h
	$(CC) -c gallery.c

graph.o: graph.c graph.h
	$(CC) -c graph.c


expr.o: expr.c expr.h
	$(CC) -c expr.c

expr_builtins.o : expr_builtins.c expr.h
	$(CC) -c expr_builtins.c


clean:
	rm *.o
	rm skedia

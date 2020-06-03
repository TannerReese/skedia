CC=gcc
flags=

skedia: skedia.o graph.o gallery.o expr.o expr_builtins.o
	$(CC) $(flags) -o skedia skedia.o graph.o gallery.o expr.o expr_builtins.o -lcurses -lm

skedia.o: skedia.c
	$(CC) $(flags) -c skedia.c

gallery.o: gallery.c gallery.h
	$(CC) $(flags) -c gallery.c

graph.o: graph.c graph.h
	$(CC) $(flags) -c graph.c


expr.o: expr.c expr.h
	$(CC) $(flags) -c expr.c

expr_builtins.o : expr_builtins.c expr.h
	$(CC) $(flags) -c expr_builtins.c


clean:
	rm *.o
	rm skedia

CC=gcc
flags=

# Build main program
main: skedia

skedia: skedia.o args.o graph.o gallery.o intersect.o expr.o expr_builtins.o
	$(CC) $(flags) -o skedia skedia.o args.o graph.o gallery.o intersect.o expr.o expr_builtins.o -lcurses -lm


# Build object files
skedia.o: skedia.c
	$(CC) $(flags) -c skedia.c

args.o: args.c args.h
	$(CC) $(flags) -c args.c

intersect.o: intersect.c intersect.h
	$(CC) $(flags) -c intersect.c

gallery.o: gallery.c gallery.h
	$(CC) $(flags) -c gallery.c

graph.o: graph.c graph.h
	$(CC) $(flags) -c graph.c


# Expression Parser object files
expr.o: expr.c expr.h
	$(CC) $(flags) -c expr.c

expr_builtins.o : expr_builtins.c expr.h
	$(CC) $(flags) -c expr_builtins.c


# Remove binary and object files
clean:
	rm -f *.o  # Remove object files
	rm -f skedia  # Remove binary



# Man Page compilation
docs: skedia.pdf skedia.html skedia.1.gz

skedia.pdf: skedia.man
	groff -mandoc -Tpdf skedia.man > skedia.pdf

skedia.html: skedia.man
	groff -mandoc -Thtml skedia.man > skedia.html

# Compressed Manpage to put in /usr/share/man/man1
skedia.1.gz: skedia.man
	gzip -9c skedia.man > skedia.1.gz

# Remove documentation files
clean-docs:
	rm -f skedia.pdf skedia.html skedia.1.gz


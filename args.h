#ifndef __ARGS_H
#define __ARGS_H

#include "graph.h"
#include "gallery.h"

struct args_s {
	// Indicate that the program should not start ncurses
	// And simply print the calculated intersections
	bool only_intersects;
	
	// Store location and size of graph in terminal and in the plane
	graph_t *grp;
	
	// Linked list of equations representing the gallery
	equat_t *gallery;
};

// Parse list of command line arguments using getopt
void parse_args(struct args_s *args, int argc, char *argv[]);

#endif


#ifndef _GALLERY_H
#define _GALLERY_H

#include "ncurses.h"
#include "expr.h"

// Mask used that should be used on color pairs to created inverted versions
#define INVERT_PAIR 0x80

#define GALLERY_WIDTH 25 // Size of gallery in ncurses
#define TEXTBOX_SIZE 64

// Represent equation attached to a textbox
typedef struct equat_s{
	// Contents of textbox
	char text[TEXTBOX_SIZE];
	// Indicate location of cursor in text
	// If curs == NULL then focus on color picker
	char *curs;
	// Store error code for any parse errors that occur
	parse_err_t err;
	
	// Left and Right hand side of equation
	expr_t left, right;
	
	// Color for curve of equation
	int color_pair;
	
	// Point to previous and next equation in the linked list
	struct equat_s *prev, *next;
} *equat_t;

// Evaluate equation by subtracting the right side from the left
double eval_equat(void *inp, double x, double y);

// Display linked list of equation to given window
void draw_gallery(WINDOW *win, equat_t top, bool show_curs);
// Use text of equation to generate the left and right hand expressions
parse_err_t parse_equat(equat_t eq);
// Create new equation at the end of gallery
// With the given null terminated text in the textbox
equat_t add_equat(equat_t *gallery, const char *text);

#endif

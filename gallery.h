#ifndef _GALLERY_H
#define _GALLERY_H

#include "ncurses.h"
#include "expr.h"

// Mask used that should be used on color pairs to created inverted versions
#define INVERT_PAIR 0x80

#define GALLERY_WIDTH 25 // Size of gallery in ncurses
#define TEXTBOX_SIZE 64

/* Represent equation attached to a textbox
 * Forms:
 *   Proper Equation / Non-Variable : Used for curves that will be drawn to graph
 *     Ex: "x^2 - x = sin(y)" or "x + y = f(x) + w"
 *   Equation for Variable : Variable to be used in other equations
 *     Ex: "w := x^2 - y" or "f(a, b, x, y) := a*b + x*y"
 */
typedef struct equat_s{
	// Contents of textbox
	char text[TEXTBOX_SIZE];
	
	// Indicates if current equation is being parsed
	// Used to prevent circular recursion when reparsing dependencies
	bool being_parsed : 1;
	// Indicates if this equation represents a variable
	bool is_variable : 1;
	
	union{
		// VARIABLE
		struct{
			// Name of variable and length if equat represents a variable
			// Example: "g := x + y" or "f(r, s) := r^s"
			char *name;
			size_t name_len;
			
			// Number of arguments to variable if equation represents variable
			int arity;
		};
		
		// NON-VARIABLE / PROPER EQUATION
		struct{
			// Left hand side of equation
			// Only necessary when equation doesn't represent variable
			expr_t left;
			
			// Variable equations are not drawn and so don't need a color
			// Color for curve of equation
			int color_pair;
		};
	};
	
	// Indicate location of cursor in text
	// If curs == NULL then focus on color picker
	char *curs;
	// Store error code for any parse errors that occur
	parse_err_t err;
	
	// Right hand side of equation
	expr_t right;
	
	// Point to previous and next equation in the linked list
	struct equat_s *prev, *next;
} *equat_t;

// Evaluate equation by subtracting the right side from the left
double eval_equat(void *inp, double x, double y);

// Display linked list of equation to given window
void draw_gallery(WINDOW *win, equat_t top, bool show_curs);
// Use text of equation to generate the left and right hand expressions
parse_err_t parse_equat(equat_t gallery, equat_t eq);
// Create new equation at the end of gallery
// With the given null terminated text in the textbox
equat_t add_equat(equat_t *gallery, const char *text);

#endif

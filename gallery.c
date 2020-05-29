#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "gallery.h"

// Locations to place x, y, and redius values for evaluation of expressions
static double xref, yref, rref;


// Functions to parse and evaluate equations
static expr_t translate_name(expr_t exp, const char *name, size_t n, void *inp){
	if(n > 1) return NULL;
	
	switch(*name){
		case 'x': return cached_expr(exp, &xref);
		case 'y': return cached_expr(exp, &yref);
		case 'r': return cached_expr(exp, &rref);
	}
	return NULL;
}

// Function passed to graph to draw curve
double eval_equat(void *inp, double x, double y){
	equat_t eq = inp;
	xref = x;
	yref = y;
	rref = hypot(x, y);
	
	return eval_expr(eq->left, NULL) - eval_expr(eq->right, NULL);
}



// Height of each textbox
#define TEXTBOX_HEIGHT 4
// Display linked list of equation to given window
void draw_gallery(WINDOW *win, equat_t top, bool show_curs){
	int wid, hei;
	getmaxyx(win, hei, wid);
	
	// Draw box around gallery
	wborder(win, '*', '*', '*', '*', '*', '*', '*', '*');
	
	int x, y, i = 0;
	// i represents the index of the textbox
	char *s;
	for(equat_t eq = top; eq; eq = eq->next){
		// Indicate if highlight should be drawn in textbox
		bool do_highlight = i == 0 && show_curs;
		
		x = 1;
		y = i * (TEXTBOX_HEIGHT + 1) + 1;
		for(s = eq->text; *s != '\0' || eq->curs == s && do_highlight; s++){
			if(s == eq->curs && do_highlight){
				wattron(win, COLOR_PAIR(INVERT_PAIR));
				// Display '\0' as ' ' when highlighted
				mvwaddch(win, y, x, *s == '\0' ? ' ' : *s);
				wattroff(win, COLOR_PAIR(INVERT_PAIR));
			}else{
				mvwaddch(win, y, x, *s);
			}
			x++;
			// Move print location back to beginning of next line to wrap text
			if(x >= wid - 1){
				x = 1;
				y++;
			}
			if(y >= hei - 1 || y >= i * (TEXTBOX_HEIGHT + 1) + TEXTBOX_HEIGHT - 1) break;
		}
		
		y = i * (TEXTBOX_HEIGHT + 1) + TEXTBOX_HEIGHT;
		if(y < hei - 1){
			if(eq->err == ERR_OK){
				// Create color picker bar at bottom
				
				// Use inverted color pair to indicate selection
				wattron(win, COLOR_PAIR(eq->color_pair | (i == 0 && !(eq->curs) && show_curs ? INVERT_PAIR : 0) ));
				for(x = 1; x < wid - 1; x++){
					mvwaddch(win, y, x, '-');
				}
				wattroff(win, COLOR_PAIR(eq->color_pair | (i == 0 && !(eq->curs) && show_curs ? INVERT_PAIR : 0)));
			}else{
				// Display parse error instead of color picker if there was one
				char buf[wid - 1]; // Create buffer to store and potentially truncate error message
				buf[0] = '\0';
				strncat(buf, parse_errstr[eq->err], wid - 1);
				if(i == 0 && !(eq->curs) && show_curs){
					// Highlight error if cursor on it to indicate the cursor's location
					wattron(win, COLOR_PAIR(INVERT_PAIR));
					mvwprintw(win, y, 1, "%s", buf);
					wattroff(win, COLOR_PAIR(INVERT_PAIR));
				}else{
					mvwprintw(win, y, 1, "%s", buf);
				}
			}
		}
		y++;
		
		// Draw divider between textboxes
		for(x = 1; x < hei - 1; x++){
			mvwaddch(win, y, x, '*');
		}
		
		i++; // Move textbox index ahead
	}
}

parse_err_t parse_equat(equat_t eq){
	// Split arg on '=' to create the left and right strings
	char *right;
	for(right = eq->text; *right && *right != '='; right++){}
	// If no '=' exists return err
	if(!(*right)){
		eq->err = ERR_BAD_EXPRESSION;
		return eq->err;
	}
	// If '=' exists break up the string
	// And place right at the beginning of the right side
	*(right++) = '\0';
	
	eq->err = ERR_OK;
	
	// If left hand expression already exists free it
	if(eq->left) free_expr(eq->left);
	// Parse left hand side
	eq->left = parse_expr(eq->text, translate_name, NULL, &(eq->err), eq);
	if(eq->err != ERR_OK){
		// Undo splitting
		*(--right) = '=';
		return eq->err;
	}
	
	// If right hand expression already exists free it
	if(eq->right) free_expr(eq->right);
	// Parse right hand side
	eq->right = parse_expr(right, translate_name, NULL, &(eq->err), eq);
	if(eq->err != ERR_OK){
		// Left expression successfully parsed and so must be properly deallocated
		if(eq->left){
			free_expr(eq->left);
			eq->left = NULL;
		}
		
		// Undo splitting
		*(--right) = '=';
		return eq->err;
	}
	
	// Undo splitting
	*(--right) = '=';
	return ERR_OK;
}

equat_t add_equat(equat_t *gallery, const char *text){
	// Seek to end of gallery to append equation
	equat_t prev = NULL, *new;
	for(new = gallery; *new; new = &((*new)->next)){
		prev = *new;
	}
	*new = malloc(sizeof(struct equat_s));
	// Place text into the textbox of the equation
	(*new)->text[0] = '\0';
	strncat((*new)->text, text, TEXTBOX_SIZE);
	(*new)->curs = (*new)->text;
	
	// Ensure that left and right are null to prevent parse_equat from accidentally freeing unallocated space
	(*new)->left = NULL;
	(*new)->right = NULL;
	
	// Set default parameters
	(*new)->color_pair = 1;
	(*new)->prev = prev;
	(*new)->next = NULL;
	
	return *new;
}



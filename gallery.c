#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "gallery.h"

// Locations to place x, y, and redius values for evaluation of expressions
static double xref, yref, rref;

// List of arguments for variable
static struct arg_s{
	char *name;
	size_t length;
	
	struct arg_s *next;
} *arguments;

// Functions to parse and evaluate equations
static expr_t translate_name(expr_t exp, const char *name, size_t n, void *inp){
	if(arguments){
		// Check argument list first if present
		int i = 0;
		for(struct arg_s *arg = arguments; arg; arg = arg->next){
			if(arg->name && strncmp(arg->name, name, arg->length < n ? arg->length : n) == 0){
				return arg_expr(exp, i);
			}
			i++;
		}
	}
	
	// Check global variables next
	if(n == 1){
		switch(*name){
			case 'x': return cached_expr(exp, &xref);
			case 'y': return cached_expr(exp, &yref);
			case 'r': return cached_expr(exp, &rref);
		}
	}
	
	// Finally check gallery for variable names that match
	// inp -> head of list of equations
	for(equat_t eq = inp; eq; eq = eq->next){
		if(eq->is_variable // Only consider equations that represent variables
		&& eq->name
		&& strncmp(eq->name, name, eq->name_len < n ? eq->name_len : n) == 0
		&& eq->right  // Ensure variable has expression
		){
			return apply_expr(exp, eq->right, eq->arity, NULL);
		}
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
				if(!(eq->is_variable)){ // Only draw color bar if equation doesn't represent variable
					// Draw color picker bar at bottom
					
					// Use inverted color pair to indicate selection
					wattron(win, COLOR_PAIR(eq->color_pair | (i == 0 && !(eq->curs) && show_curs ? INVERT_PAIR : 0) ));
					for(x = 1; x < wid - 1; x++){
						mvwaddch(win, y, x, '-');
					}
					wattroff(win, COLOR_PAIR(eq->color_pair | (i == 0 && !(eq->curs) && show_curs ? INVERT_PAIR : 0)));
				}
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


static parse_err_t parse_var_equat(equat_t eq){
	// Start with no arguments
	eq->arity = 0;
	
	// Iterate through left side of variable
	// Should be either <name> or <name>(<arg>, <arg>, ...)
	bool in_parenth = 0, extra_arg = 0;
	for(char *s = eq->text; *s != '\0'; s++){
		// Skip whitespace
		if(isspace(*s)) continue;
		
		if(isalpha(*s)){
			// If two alphanumeric sequences are consecutive without ',', '(', or ')' throw an error
			if(extra_arg){
				eq->err = ERR_TOO_MANY_VALUES;
				return eq->err;
			}
			
			if(!(eq->name)){
				// Get name of variable
				eq->name = s;
				while(isalnum(*s)) s++;
				eq->name_len = (size_t)(s - eq->name);
				s--; // Move s pointer to last alphanumeric
			}else{
				// Get argument and place it in arguments
				eq->arity++;
				
				// Seek to end of argument list
				struct arg_s **arg;
				for(arg = &arguments; *arg; arg = &((*arg)->next)){}
				
				// Allocate memory on heap for argument
				*arg = malloc(sizeof(struct arg_s));
				(*arg)->name = s;
				(*arg)->next = NULL;
					
				while(isalnum(*s)) s++;
				(*arg)->length = (size_t)(s - (*arg)->name);
				s--; // Move s pointer to last alphanumeric
			}
			
			extra_arg = 1;
		}else if(*s == '(' || *s == ')'){
			// If parenth already open or closed throw mismatch
			// Nested parenths not allowed
			if(in_parenth == (*s == '(')){
				eq->err = ERR_PARENTH_MISMATCH;
				return eq->err;
			}
			
			in_parenth = *s == '(';
			extra_arg = 0;
		}else if(*s == ','){
			extra_arg = 0;
		}else{
			eq->err = ERR_UNUSED_CHARACTER;
			return eq->err;
		}
	}
	
	// If parenth wasn't closed indicate parenth mismatch
	if(in_parenth){
		eq->err = ERR_PARENTH_MISMATCH;
		return eq->err;
	}
	
	eq->err = ERR_OK;
	return ERR_OK;
}

parse_err_t parse_equat(equat_t gallery, equat_t eq){
	if(eq->being_parsed) return ERR_BAD_EXPRESSION; // If equation already being parsed return
	eq->being_parsed = 1;
	
	// Split arg on '=' to create the left and right strings
	char *right;
	for(right = eq->text; *right && *right != '='; right++){}
	// If no '=' exists return err
	if(!(*right)){
		eq->err = ERR_BAD_EXPRESSION;
		eq->being_parsed = 0;
		return eq->err;
	}
	
	
	// If left hand expression already exists free it
	if(!(eq->is_variable) && eq->left) free_expr(eq->left);
	
	// If equation is separated by ':=' instead of '=' then treat equation as variable
	if(*(right - 1) == ':'){
		eq->is_variable = 1; // Designate equation as representing a variable
		
		// Initialize name to NULL
		eq->name = NULL;
		eq->name_len = 0;
		
		// Parse variable name and possibly arguments on left hand side
		*(right - 1) = '\0';
		eq->err = parse_var_equat(eq);
		*(right - 1) = ':';
		
		if(eq->err != ERR_OK){
			// Deallocate the arguments on error
			struct arg_s *arg = arguments, *tmp;
			while(arg){
				tmp = arg->next;
				free(arg);
				arg = tmp;
			}
			arguments = NULL;
			
			eq->being_parsed = 0;
			return eq->err;
		}
	}else{
		eq->is_variable = 0; // Designate equation as proper equation
		eq->err = ERR_OK;
		
		// Ensure no arguments are parsed on left hand side
		arguments = NULL;
		
		// Parse left hand side
		*right = '\0';  // Put null where '=' is to restrict parsing to left side
		eq->left = parse_expr(eq->text, translate_name, NULL, &(eq->err), gallery);
		*right = '=';  // Undo replacement
		if(eq->err != ERR_OK){
			eq->being_parsed = 0;
			return eq->err;
		}
	}
	
	// Move right to after the '='
	right++;
	
	expr_t old_ref = eq->right; // Save right hand side to later check for dependencies and update them
	
	// Parse right hand side
	eq->right = parse_expr(right, translate_name, NULL, &(eq->err), gallery);
	
	// Deallocate the arguments
	struct arg_s *arg = arguments, *tmp;
	while(arg){
		tmp = arg->next;
		free(arg);
		arg = tmp;
	}
	arguments = NULL;
	
	if(old_ref){
		// Construct target to check for dependency
		expr_t target = new_expr();
		apply_expr(target, old_ref, 0, NULL);
		
		// Reparse equations dependent on this one
		for(equat_t eq2 = gallery; eq2; eq2 = eq2->next){
			if(eq2 == eq) continue; // Skip self
			
			if(!(eq2->is_variable) && eq2->left){
				// If equation has left hand side check left for dependency
				if(expr_depends(eq2->left, target)){
					parse_equat(gallery, eq2);
				}
			}
			
			// If equation has right hand side check right for dependency
			if(eq2->right && expr_depends(eq2->right, target)){
				parse_equat(gallery, eq2);
			}
		}
		
		free_expr(target); // Deallocate dependency checking target
		free_expr(old_ref); // Finally free right hand side
	}
	
	if(eq->err != ERR_OK){
		// Left expression successfully parsed and so must be properly deallocated
		if(eq->left){
			free_expr(eq->left);
			eq->left = NULL;
		}
		
		eq->being_parsed = 0;
		return eq->err;
	}
	
	eq->being_parsed = 0;
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
	
	// Clear all union values
	(*new)->name = NULL;
	(*new)->name_len = 0;
	(*new)->left = NULL;
	(*new)->color_pair = 1;
	
	(*new)->is_variable = 0; // Default to proper equation
	(*new)->curs = (*new)->text;
	
	// Ensure that left and right are null to prevent parse_equat from accidentally freeing unallocated space
	(*new)->right = NULL;
	
	// Set default parameters
	(*new)->prev = prev;
	(*new)->next = NULL;
	
	return *new;
}



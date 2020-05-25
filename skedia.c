#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ncurses.h>
#include <argp.h>

#include "graph.h"
#include "expr.h"

#define KEY_SUP 0521
#define KEY_SDOWN 0520



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
} equat_t;

// Locations to store x and y coordinate in along with radius
// Pointed to by cached expressions in left and right
double xref, yref, rref;

// Doubly Linked list of equations and pointer to currently selected textbox
equat_t *gallery, *gal_curs;

// Name translation function for parsing variable names
expr_t translate_name(expr_t exp, const char *name, size_t n, void *inp);
// Evaluate equation by subtracting the right side from the left
double eval_equat(void *inp, double x, double y);

// Display linked list of equation to given window
void draw_gallery(WINDOW *win, bool show_curs);
// Use text of equation to generate the left and right hand expressions
parse_err_t parse_equat(equat_t *eq);
// Create new equation at the end of gallery
// With the given null terminated text in the textbox
equat_t *add_equat(const char *text);



// Store location and size of graph in terminal and in the plane
graph_t grp = {NULL, -5, 5, 10, 10};

error_t parse_opt(int key, char *arg, struct argp_state *state);

struct argp_option options[] = {
	{"width", 'w', "UNITS", 0, "Width of grid as float (def: 10)"},
	{"height", 'h', "UNITS", 0, "Height of grid as float (def: 10)"},
	{"center", 'e', "XPOS,YPOS", 0, "Position of the center of the grid (def: 0,0)"},
	{"input", 'i', "EQUATION", 0, "Add an equation for a curve"},
	{"color", 'c', "COLOR", 0, "Set the color of the curve specified before (def: red)"},
	{0}
};
struct argp argp = {options, parse_opt, "-i EQUATION1 [-c COLOR1] [-i EQUATION2 [-c COLOR2] ...", "Graph curves and functions\vColors are designated as red: r, green: g, blue: b, cyan: c, yellow: y, or magenta: m"};


int main(int argc, char *argv[]){
	argp_parse(&argp, argc, argv, 0, NULL, NULL);
	
	// Place gallery cursor at beginning
	gal_curs = gallery;
	
	// Initialize Ncurses
	initscr();
	raw();
	keypad(stdscr, TRUE);
	noecho();
	curs_set(0);
	refresh(); // Needed to allow sub-windows to draw
	
	// Init all colors for curves
	start_color();
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_BLUE, COLOR_BLACK);
	init_pair(4, COLOR_CYAN, COLOR_BLACK);
	init_pair(5, COLOR_YELLOW, COLOR_BLACK);
	init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
	// Init inverse color pairs
	#define INVERT_PAIR 0x10
	init_pair(0 | INVERT_PAIR, COLOR_BLACK, COLOR_WHITE); // Used to indicate cursor location in text
	init_pair(1 | INVERT_PAIR, COLOR_BLACK, COLOR_RED);
	init_pair(2 | INVERT_PAIR, COLOR_BLACK, COLOR_GREEN);
	init_pair(3 | INVERT_PAIR, COLOR_BLACK, COLOR_BLUE);
	init_pair(4 | INVERT_PAIR, COLOR_BLACK, COLOR_CYAN);
	init_pair(5 | INVERT_PAIR, COLOR_BLACK, COLOR_YELLOW);
	init_pair(6 | INVERT_PAIR, COLOR_BLACK, COLOR_MAGENTA);
	
	// Use part of screen for graph and part for gallery
	grp.win = newwin(0, 0, 0, 26);
	WINDOW *galwin = newwin(0, 25, 0, 0);
	
	int c;
	bool running = 1;
	// Determine whether the gallery and graph should be redrawn
	bool update_gallery = 1, update_graph = 1;
	// Indicate whether key strokes are sent to the graph or the gallery
	bool focus_on_graph = 1;
	while(running){
		if(update_graph){
			// Draw graph
			wclear(grp.win);  // Clear graph window
			draw_gridlines(grp);
			// Draw equations
			for(equat_t *eq = gallery; eq; eq = eq->next){
				wattron(grp.win, COLOR_PAIR(eq->color_pair));
				draw_curve(grp, eval_equat, eq);
				wattroff(grp.win, COLOR_PAIR(eq->color_pair));
			}
			wrefresh(grp.win);
		}
		
		if(update_gallery){
			// Draw gallery
			wclear(galwin);
			draw_gallery(galwin, !focus_on_graph);
			wrefresh(galwin);
		}
		
		update_gallery = 0;
		update_graph = 0;
		c = getch();
		if(c == (int)('C' & 0x1f) || c == (int)('Z' & 0x1f)){ // Check for Control-C or Control-Z
			running = 0;
		}else if(focus_on_graph){
			// Controls when focus is on graph
			
			// Graph needs to be redrawn after movement or zoom
			update_graph = 1;
			switch(c){
				case 'q':
				case 'Q': running = 0;
				break;
				
				// Movement controls
				case 'j':
				case KEY_DOWN: grp.y -= grp.hei / 10;
				break;
				case 'k':
				case KEY_UP: grp.y += grp.hei / 10;
				break;
				case 'h':
				case KEY_LEFT: grp.x -= grp.wid / 10;
				break;
				case 'l':
				case KEY_RIGHT: grp.x += grp.wid / 10;
				break;
				
				// Dilation controls in each dimension
				case 'J':
				case KEY_SDOWN: zoom_graph(&grp, 1, 1.1);
				break;
				case 'K':
				case KEY_SUP: zoom_graph(&grp, 1, 0.9);
				break;
				case 'H':
				case KEY_SLEFT: zoom_graph(&grp, 1.1, 1);
				break;
				case 'L':
				case KEY_SRIGHT: zoom_graph(&grp, 0.9, 1);
				break;
				
				// Dilation controls for both dimensions
				case '-': zoom_graph(&grp, 1.1, 1.1); // Zoom Out (-)
				break;
				case '=': zoom_graph(&grp, 0.9, 0.9); // Zoom In (+)
				break;
				case '0': setdims_graph(&grp, 10, 10); // Zoom Standard
				break;
				
				// Switch focus to textboxes in gallery
				case 'g':
				case 'G':
					focus_on_graph = 0;
					update_graph = 0;
					update_gallery = 1;
				break;
			}
		}else{
			// Controls when in textbox
			
			// Gallery needs to be redrawn after movement or change
			update_gallery = 1;
			
			// Cursor must exist for operations to be performed
			if(gal_curs){
				switch(c){
					case KEY_DOWN:
						if(gal_curs->curs){
							// If cursor is in text move it to the color picker
							gal_curs->curs = NULL;
						}else{
							if(gal_curs->next){
								// If cursor on color picker move it down to next textbox
								gal_curs = gal_curs->next;
							}
						}
					break;
					case KEY_UP:
						if(gal_curs->curs){
							if(gal_curs->prev){
								// If cursor is in text move it to previous textbox
								gal_curs = gal_curs->prev;
							}
						}else{
							// If cursor on color picker move into text
							gal_curs->curs = gal_curs->text;
						}
					break;
					case KEY_RIGHT:
						if(gal_curs->curs){
							// Move text cursor within selected textbox
							if(*(gal_curs->curs) != '\0' && gal_curs->curs < gal_curs->text + TEXTBOX_SIZE)
								gal_curs->curs++;
						}else{
							// Change color
							gal_curs->color_pair++;
							// Wrap color pair to fit in {1, 2, 3, 4, 5, 6}
							gal_curs->color_pair = (gal_curs->color_pair - 1) % 6 + 1;
							// Update graph to show color change
							update_graph = 1;
						}
					break;
					case KEY_LEFT:
						if(gal_curs->curs){
							if(gal_curs->curs > gal_curs->text)
								gal_curs->curs--;
						}else{
							// Change color
							gal_curs->color_pair += 5; // Equivalent to color_pair--
							// Wrap color pair to fit in {1, 2, 3, 4, 5, 6}
							gal_curs->color_pair = (gal_curs->color_pair - 1) % 6 + 1;
							// Update graph to show color change
							update_graph = 1;
						}
					break;
					
					case KEY_BACKSPACE:
					case 0x7f:
					case '\b': // Backspace
						if(gal_curs->curs && gal_curs->curs > gal_curs->text){
							for(char *s = gal_curs->curs - 1; *s != '\0' && s < gal_curs->text + TEXTBOX_SIZE - 1; s++){
								*s = *(s + 1);
							}
							// Move cursor backwards
							gal_curs->curs--;
						}
					break;
					case KEY_HOME:
						// Move cursor to beginning of text
						if(gal_curs->curs) gal_curs->curs = gal_curs->text;
					break;
					case KEY_END:
						// Move cursor to end of text
						if(gal_curs->curs){
							gal_curs->curs = gal_curs->text + strnlen(gal_curs->text, TEXTBOX_SIZE);
						}
					break;
					
					// Parse text in textbox to update equation
					case KEY_ENTER:
					case '\r':
					case '\n':
						// If in textbox parse text
						if(gal_curs->curs){
							parse_equat(gal_curs);
							
							// Update graph to reflect new equation
							update_graph = 1;
						}
					break;
					
					// Remove current textbox and equation
					case 'D' & 0x1f:
						if(gal_curs->prev){
							gal_curs->prev->next = gal_curs->next;
							if(gal_curs->next) gal_curs->next->prev = gal_curs->prev;
						}else{
							// If gal_curs at head then point gallery to next element
							gallery = gal_curs->next;
							gal_curs->next->prev = NULL;
						}
						
						if(gal_curs->left) free(gal_curs->left);
						if(gal_curs->right) free(gal_curs->right);
						free(gal_curs);
						gal_curs = gallery;
						
						// Update graph to remove the curve for this equation
						update_graph = 1;
					break;
					
					// Switch focus back to graph
					case 0x1b: // '\e' or Esc
						focus_on_graph = 1;
					break;
				}
				
				// If c can be printed then place it in the text
				if(isprint(c) && gal_curs->curs){
					char tmp, *s;
					for(s = gal_curs->curs; s < gal_curs->text + TEXTBOX_SIZE; s++){
						tmp = c;
						c = *s;
						*s = tmp;
						
						// If all characters up to the null have been moved end
						if(*s == '\0') break;
					}
					
					// Move cursor forward
					if(gal_curs->curs < gal_curs->text + TEXTBOX_SIZE - 1) gal_curs->curs++;
					
					// Ensure text is null terminated
					gal_curs->text[TEXTBOX_SIZE - 1] = '\0';
				}
			}
		}
		
		// Command runs for both modes
		if(c == (int)('A' & 0x1f)){ // ^A, Ctrl-A
			// Create new textbox (not necessary for gal_curs != NULL)
			// Create equation and textbox at end of gallery and move cursor to it
			gal_curs = add_equat("");
			
			// Move focus to gallery to type
			update_gallery = 1;
			focus_on_graph = 0;
		}
	}
	
	delwin(grp.win);
	endwin();
	return 0;
}



// Functions to parse and evaluate equations
expr_t translate_name(expr_t exp, const char *name, size_t n, void *inp){
	if(n > 1) return NULL;
	
	equat_t *eq = inp;
	switch(*name){
		case 'x': return cached_expr(exp, &xref);
		case 'y': return cached_expr(exp, &yref);
		case 'r': return cached_expr(exp, &rref);
	}
	return NULL;
}

double eval_equat(void *inp, double x, double y){
	equat_t *eq = inp;
	xref = x;
	yref = y;
	rref = hypot(x, y);
	
	return eval_expr(eq->left, NULL) - eval_expr(eq->right, NULL);
}



#define TEXTBOX_HEIGHT 4
void draw_gallery(WINDOW *win, bool show_curs){
	int wid, hei;
	getmaxyx(win, hei, wid);
	
	// Draw box around gallery
	wborder(win, '*', '*', '*', '*', '*', '*', '*', '*');
	
	int x, y, i = 0;
	// i represents the index of the textbox
	char *s;
	for(equat_t *eq = gal_curs; eq; eq = eq->next){
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

parse_err_t parse_equat(equat_t *eq){
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

equat_t *add_equat(const char *text){
	// Seek to end of gallery to append equation
	equat_t *prev = NULL, **new;
	for(new = &gallery; *new; new = &((*new)->next)){
		prev = *new;
	}
	*new = malloc(sizeof(equat_t));
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





// Argp parse function
error_t parse_opt(int key, char *arg, struct argp_state *state){
	double x, y;
	equat_t *tmp;
	switch(key){
		// Get window location and dimensions
		case 'w':
			if(sscanf(arg, "%lf", &x)){
				setdims_graph(&grp, x, grp.hei);
			}else argp_usage(state);
		break;
		case 'h':
			if(sscanf(arg, "%lf", &y)){
				setdims_graph(&grp, grp.wid, y);
			}else argp_usage(state);
		break;
		case 'e':
			if(sscanf(arg, "%lf,%lf", &x, &y) != EOF){
				grp.x = x - grp.wid / 2;
				grp.y = y + grp.hei / 2;
			}else argp_usage(state);
		break;
		
		case 'c':
			if(strlen(arg) > 1) break;
			
			// If there are no equalities no colors may be specified
			if(!gallery) break;
			
			// Get last equation / textbox in gallery
			for(tmp = gallery; tmp->next; tmp = tmp->next){}
			switch(arg[0]){
				case 'r': tmp->color_pair = 1;
				break;
				case 'g': tmp->color_pair = 2;
				break;
				case 'b': tmp->color_pair = 3;
				break;
				case 'c': tmp->color_pair = 4;
				break;
				case 'y': tmp->color_pair = 5;
				break;
				case 'm': tmp->color_pair = 6;
				break;
			}
		break;
		case 'i':
			// Create new equation at the end of the gallery
			tmp = add_equat(arg);
			
			// Parse text and check for errors
			if(parse_equat(tmp) != ERR_OK){
				// If there is an error while parsing return it
				fprintf(stderr, "Error %s while reading equation: %s\n", parse_errstr[(tmp)->err], arg);
				// If expressions were created during parsing deallocate them
				if(tmp->left) free_expr(tmp->left);
				if(tmp->right) free_expr(tmp->right);
				free(tmp);
				
				argp_usage(state);
				break;
			}
		break;
		default: return ARGP_ERR_UNKNOWN;
	}
	return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <ctype.h> // For int isprint(int c)

#include <ncurses.h>

#include "graph.h"
#include "gallery.h"
#include "intersect.h"
#include "expr.h"

#include "args.h"

// Identify shifted keys
#define KEY_SUP 0521
#define KEY_SDOWN 0520


// Linked list of equations representing the gallery and a pointer to the cursor
equat_t gallery = NULL, gcurs = NULL;
// Circular Linked list of intersections 
inter_t intersections = NULL;

// Store location and size of graph in terminal and in the plane
graph_t grp = {NULL, -5, 5, 10, 10};



int main(int argc, char *argv[]){
	struct args_s args = {0, &grp, &gallery};
	parse_args(&args, argc, argv);
	
	
	// Intersection Calculation
	// ---------------------
	// Check for '-x' flag to not start ncurses
	if(args.only_intersects){
		struct bound_s rect = {grp.x, grp.y, grp.wid, grp.hei, 1000, 1000};
		bool isfst = 1;
		
		// Iterate over all pairs of equations
		for(equat_t eq1 = gallery; eq1; eq1 = eq1->next) if(!(eq1->is_variable) && eq1->right){
			// Iterate over all equations after eq1
			for(equat_t eq2 = eq1->next; eq2; eq2 = eq2->next) if(!(eq2->is_variable) && eq2->right){
				// Find intersections and store them in intersections
				append_inters(
					&intersections, rect,
					eval_equat, eq1,
					eval_equat, eq2,
					30, (grp.wid < grp.hei ? grp.wid : grp.hei) / 10000
				);
				
				// If no intersections found move to next curve pair
				if(!intersections) continue;
				
				// Print Header for intersections between these curves
				printf("%s%s  &  %s\n", isfst ? "" : "\n", eq1->text, eq2->text);
				isfst = 0;
				
				inter_t inr = intersections;
				do{
					// Print each intersection
					printf("( %.17lf , %.17lf )\n", inr->x, inr->y);
					
					inr = inr->next;
				}while(inr != intersections);
				
				// Empty out intersection list for next pair
				free_inters(intersections);
				intersections = NULL;
			}
		}
		return 1;
	}
	
	
	
	// Place gallery cursor at beginning
	gcurs = gallery;
	
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
	// Init inverted color pairs
	init_pair(0 | INVERT_PAIR, COLOR_BLACK, COLOR_WHITE); // Used to indicate cursor location in text
	init_pair(1 | INVERT_PAIR, COLOR_BLACK, COLOR_RED);
	init_pair(2 | INVERT_PAIR, COLOR_BLACK, COLOR_GREEN);
	init_pair(3 | INVERT_PAIR, COLOR_BLACK, COLOR_BLUE);
	init_pair(4 | INVERT_PAIR, COLOR_BLACK, COLOR_CYAN);
	init_pair(5 | INVERT_PAIR, COLOR_BLACK, COLOR_YELLOW);
	init_pair(6 | INVERT_PAIR, COLOR_BLACK, COLOR_MAGENTA);
	
	// Use part of screen for graph and part for gallery
	grp.win = newwin(0, 0, 0, GALLERY_WIDTH + 1);
	WINDOW *galwin = newwin(0, GALLERY_WIDTH, 0, 0);
	

	// Main Loop
	// ---------------------
	int c;
	bool running = 1;
	// Determine whether the gallery and graph should be redrawn
	bool update_gallery = 1, update_graph = 1;
	// Variables for tracking terminal resizes
	int scrwid, scrhei, new_scrwid, new_scrhei;
	getmaxyx(stdscr, scrwid, scrhei);
	// Indicate whether key strokes are sent to the graph or the gallery
	bool focus_on_graph = 1;
	while(running){
		// Check if terminal dimensions to resize `galwin` and `grp.win` appropriately
		getmaxyx(stdscr, new_scrhei, new_scrwid);
		if(new_scrhei != scrhei || new_scrwid != scrwid){
			scrhei = new_scrhei;
			scrwid = new_scrwid;
			
			wresize(grp.win, scrhei, scrwid - GALLERY_WIDTH);
			wresize(galwin, scrhei, GALLERY_WIDTH);
			
			// On Resize both Graph and Gallery need to be redrawn
			update_graph = 1;
			update_gallery = 1;
		}
		
		
		// Graph Redrawing
		// --------------------------
		if(update_graph){
			// Draw graph
			wclear(grp.win);  // Clear graph window
			draw_gridlines(grp);
			
			// Draw equations
			for(equat_t eq = gallery; eq; eq = eq->next){
				if(!(eq->is_variable) && eq->right){ // Only draw equation if it doesn't represent a variable
					wattron(grp.win, COLOR_PAIR(eq->color_pair));
					draw_curve(grp, eval_equat, eq);
					wattroff(grp.win, COLOR_PAIR(eq->color_pair));
				}
			}
			
			// Draw Intersections
			if(intersections){
				inter_t inr = intersections;
				
				// Display the coordinates of the selected point
				int height, width;
				getmaxyx(grp.win, height, width);
				mvwprintw(grp.win, height - 1, 0, "(%.10lg, %.10lg)", inr->x, inr->y);
				
				do{
					// Check if inr should be highlighted (when its pointed to by intersections)
					wattron(grp.win, COLOR_PAIR(inr == intersections ? ((equat_t)(inr->param2))->color_pair | INVERT_PAIR : ((equat_t)(inr->param1))->color_pair));
					draw_point(grp, inr->x, inr->y, 'O');
					wattroff(grp.win, COLOR_PAIR(inr == intersections ? ((equat_t)(inr->param2))->color_pair | INVERT_PAIR : ((equat_t)(inr->param1))->color_pair));
					
					inr = inr->next;
				}while(inr != intersections);
				
			}
			
			wrefresh(grp.win);
		}
		
		
		// Draw Gallery
		// -----------------------
		if(update_gallery){
			// Draw gallery
			wclear(galwin);
			draw_gallery(galwin, gcurs, !focus_on_graph);
			wrefresh(galwin);
		}
		
		
		
		// Parse User Input
		// ------------------------
		update_gallery = 0;
		update_graph = 0;
		c = getch();
		if(c == (int)('C' & 0x1f) || c == (int)('Z' & 0x1f)){ // Check for Control-C or Control-Z
			running = 0;
		}else if(focus_on_graph){
			// Graph Controls
			// -------------------------------
			
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
				
				// Intersection Controls
				case 'n': // Generate Intersections
				case 'N':
				{
					// Create bounding rectangle
					struct bound_s rect = {grp.x, grp.y, grp.wid, grp.hei, 0, 0};
					getmaxyx(grp.win, rect.rows, rect.columns);
					
					// Iterate over all equations
					for(equat_t eq1 = gallery; eq1; eq1 = eq1->next) if(!(eq1->is_variable) && eq1->right){
						// Iterate over all equations after eq1
						for(equat_t eq2 = eq1->next; eq2; eq2 = eq2->next) if(!(eq2->is_variable) && eq2->right){
							append_inters(
								&intersections, rect,
								eval_equat, eq1,
								eval_equat, eq2,
								30, 0.000001
							);
						}
					}
				}
				break;
				case 'c': // Clear list of intersections
				case 'C':
					free_inters(intersections);
					intersections = NULL;
				break;
				case '.': // Move to Next Intersection
				case '>':
					if(intersections){
						intersections = intersections->next;
					}
				break;
				case ',': // Move to Previous Intersection
				case '<':
					if(intersections){
						intersections = intersections->prev;
					}
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
			// Gallery Controls
			// --------------------------------------
			
			// Gallery needs to be redrawn after movement or change
			update_gallery = 1;
			
			// Cursor must exist for operations to be performed
			if(gcurs){
				switch(c){
					case KEY_DOWN:
						if(gcurs->curs){
							// If cursor is in text move it to the color picker
							gcurs->curs = NULL;
						}else{
							if(gcurs->next){
								// If cursor on color picker move it down to next textbox
								gcurs = gcurs->next;
							}
						}
					break;
					case KEY_UP:
						if(gcurs->curs){
							if(gcurs->prev){
								// If cursor is in text move it to previous textbox
								gcurs = gcurs->prev;
							}
						}else{
							// If cursor on color picker move into text
							gcurs->curs = gcurs->text;
						}
					break;
					case KEY_RIGHT:
						if(gcurs->curs){
							// Move text cursor within selected textbox
							if(*(gcurs->curs) != '\0' && gcurs->curs < gcurs->text + TEXTBOX_SIZE)
								gcurs->curs++;
						}else{
							// Change color
							gcurs->color_pair++;
							// Wrap color pair to fit in {1, 2, 3, 4, 5, 6}
							gcurs->color_pair = (gcurs->color_pair - 1) % 6 + 1;
							// Update graph to show color change
							update_graph = 1;
						}
					break;
					case KEY_LEFT:
						if(gcurs->curs){
							if(gcurs->curs > gcurs->text)
								gcurs->curs--;
						}else{
							// Change color
							gcurs->color_pair += 5; // Equivalent to color_pair--
							// Wrap color pair to fit in {1, 2, 3, 4, 5, 6}
							gcurs->color_pair = (gcurs->color_pair - 1) % 6 + 1;
							// Update graph to show color change
							update_graph = 1;
						}
					break;
					
					case KEY_BACKSPACE:
					case 0x7f:
					case '\b': // Backspace
						if(gcurs->curs && gcurs->curs > gcurs->text){
							// Move through text copying characters backwards
							for(char *s = gcurs->curs - 1; *s != '\0' && s < gcurs->text + TEXTBOX_SIZE - 1; s++){
								*s = *(s + 1);
							}
							// Move cursor backwards
							gcurs->curs--;
						}
					break;
					case KEY_HOME:
						// Move cursor to beginning of text
						if(gcurs->curs) gcurs->curs = gcurs->text;
					break;
					case KEY_END:
						// Move cursor to end of text
						if(gcurs->curs){
							gcurs->curs = gcurs->text + strnlen(gcurs->text, TEXTBOX_SIZE);
						}
					break;
					
					// Parse text in textbox to update equation
					case KEY_ENTER:
					case '\r':
					case '\n':
						// If in textbox parse text
						if(gcurs->curs){
							parse_equat(gallery, gcurs);
							
							// Update graph to reflect new equation
							update_graph = 1;
						}
					break;
					
					// Remove current textbox and equation
					case 'D' & 0x1f:
						if(gcurs->prev){
							// If gcurs is not at head then connect linked before and after gcurs
							gcurs->prev->next = gcurs->next;
							if(gcurs->next) gcurs->next->prev = gcurs->prev;
						}else{
							// If gcurs at head then point gallery to next element
							gallery = gcurs->next;
							if(gcurs->next) gcurs->next->prev = NULL;
						}
						
						// Remove intersections attached to equation
						bool remd;
						do{
							remd = remove_inter(&intersections, eval_equat, gcurs);
						}while(remd);
						
						if(!(gcurs->is_variable) && gcurs->left) free(gcurs->left);
						if(gcurs->right) free(gcurs->right);
						free(gcurs);
						gcurs = gallery;
						
						// Update graph to remove the curve for this equation
						update_graph = 1;
					break;
				}
				
				// If c can be printed then place it in the text
				if(isprint(c) && gcurs->curs){
					char tmp, *s;
					for(s = gcurs->curs; s < gcurs->text + TEXTBOX_SIZE; s++){
						tmp = c;
						c = *s;
						*s = tmp;
						
						// If all characters up to the null have been moved end
						if(*s == '\0') break;
					}
					
					// Move cursor forward
					if(gcurs->curs < gcurs->text + TEXTBOX_SIZE - 1) gcurs->curs++;
					
					// Ensure text is null terminated
					gcurs->text[TEXTBOX_SIZE - 1] = '\0';
				}
			}
			
			// Perform regardless of if cursor is present
			if(c == (int)0x1b){ // '\e' or Esc
				// Switch focus back to graph
				focus_on_graph = 1;
			}
		}
		
		// Command runs for both modes
		if(c == (int)('A' & 0x1f)){ // ^A, Ctrl-A
			// Create new textbox (not necessary for gcurs != NULL)
			// Create equation and textbox at end of gallery and move cursor to it
			gcurs = add_equat(&gallery, "");
			
			// Move focus to gallery to type
			update_gallery = 1;
			focus_on_graph = 0;
		}
	}
	
	delwin(grp.win);
	endwin();
	return 0;
}


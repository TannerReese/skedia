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



// Represent equality between expressions in xy grid
typedef struct eqlt_s{
	// Variables to store x and y coordinate in
	// Pointed to by cached expressions in left and right
	double x, y;
	double r;
	
	// Left and Right hand side of equality
	expr_t left, right;
	// Index of color pair to use for curve
	int color_pair;
	
	// Point to next equality in the linked list
	struct eqlt_s *next;
} eqlt_t;

eqlt_t *equals;

// Name translation function for parsing variable names
expr_t translate_name(expr_t exp, const char *name, size_t n, void *inp);
// Evaluate equation by  subtracting the right side from the left
double eval_eqlt(void *inp, double x, double y);



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
	
	
	// Initialize Ncurses
	initscr();
	cbreak();
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
	
	// Use whole screen for graph
	grp.win = stdscr;
	
	int c;
	bool running = 1;
	while(running){
		// Clear graph window
		wclear(grp.win);
		draw_gridlines(grp);
		// Draw functions
		for(eqlt_t *eq = equals; eq; eq = eq->next){
			wattron(grp.win, COLOR_PAIR(eq->color_pair));
			draw_curve(grp, eval_eqlt, eq);
			wattroff(grp.win, COLOR_PAIR(eq->color_pair));
		}
		wrefresh(grp.win);
		
		c = getch();
		switch(c){
			case 'Q':
			case 'q': running = 0;
			break;
			
			// Movement controls
			case KEY_DOWN: grp.y -= grp.hei / 10;
			break;
			case KEY_UP: grp.y += grp.hei / 10;
			break;
			case KEY_LEFT: grp.x -= grp.wid / 10;
			break;
			case KEY_RIGHT: grp.x += grp.wid / 10;
			break;
			
			// Dilation controls in each dimension
			case KEY_SDOWN: zoom_graph(&grp, 1, 1.1);
			break;
			case KEY_SUP: zoom_graph(&grp, 1, 0.9);
			break;
			case KEY_SLEFT: zoom_graph(&grp, 1.1, 1);
			break;
			case KEY_SRIGHT: zoom_graph(&grp, 0.9, 1);
			break;
			
			// Dilation controls for both dimensions
			case '-': zoom_graph(&grp, 1.1, 1.1);
			break;
			case '=': zoom_graph(&grp, 0.9, 0.9);
			break;
			case '0': setdims_graph(&grp, 10, 10);
			break;
		}
	}
	
	delwin(grp.win);
	endwin();
	return 0;
}



// Functions to parse and evaluate equations
expr_t translate_name(expr_t exp, const char *name, size_t n, void *inp){
	if(n > 1) return NULL;
	
	eqlt_t *eq = inp;
	switch(*name){
		case 'x': return cached_expr(exp, &(eq->x));
		case 'y': return cached_expr(exp, &(eq->y));
		case 'r': return cached_expr(exp, &(eq->r));
	}
	return NULL;
}

double eval_eqlt(void *inp, double x, double y){
	eqlt_t *eq = inp;
	eq->x = x;
	eq->y = y;
	eq->r = hypot(x, y);
	
	return eval_expr(eq->left, NULL) - eval_expr(eq->right, NULL);
}



// Argp parse function
error_t parse_opt(int key, char *arg, struct argp_state *state){
	double x, y;
	eqlt_t **new;
	switch(key){
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
			if(!equals) break;
			
			eqlt_t *last;
			for(last = equals; last->next; last = last->next){}
			switch(arg[0]){
				case 'r': last->color_pair = 1;
				break;
				case 'g': last->color_pair = 2;
				break;
				case 'b': last->color_pair = 3;
				break;
				case 'c': last->color_pair = 4;
				break;
				case 'y': last->color_pair = 5;
				break;
				case 'm': last->color_pair = 6;
				break;
			}
		break;
		case 'i':
			for(new = &equals; *new; new = &((*new)->next)){}
			*new = malloc(sizeof(eqlt_t));
			(*new)->color_pair = 1;
			(*new)->next = NULL;
			
			// Split arg on equals to create the left and right strings
			char *right;
			for(right = arg; *right && *right != '='; right++){}
			// If no equals exists break
			if(!(*right)){
				argp_usage(state);
				break;
			}
			// If equals exists break up the string
			// And place right at the beginning of the right side
			*right = '\0';
			right++;
			
			parse_err_t err;
			(*new)->left = parse_expr(arg, translate_name, NULL, &err, *new);
			if(err != ERR_OK){
				free(*new);
				fprintf(stderr, "Error %s while reading left hand expression: %s\n", parse_errstr[err], arg);
				argp_usage(state);
				break;
			}
			
			(*new)->right = parse_expr(right, translate_name, NULL, &err, *new);
			if(err != ERR_OK){
				free(*new);
				fprintf(stderr, "Error %s while reading right hand expression: \"%s\"\n", parse_errstr[err], right);
				argp_usage(state);
				break;
			}
		break;
		default: return ARGP_ERR_UNKNOWN;
	}
	return 0;
}


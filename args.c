#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "args.h"

struct option longopts[] = {
	{"help", no_argument, NULL, '?'},
	{"usage", no_argument, NULL, 10},
	{"width", required_argument, NULL, 'w'},
	{"height", required_argument, NULL, 'h'},
	{"center", required_argument, NULL, 'e'},
	{"input", required_argument, NULL, 'i'},
	{"color", required_argument, NULL, 'c'},
	{"intersects", no_argument, NULL, 'x'},
	{0}
};

// Help message
const char help_msg[] = 
	"Usage: skedia [OPTIONS...] [-i EQU1 [-c COL1] [-i EQU2 [-c COL2] ...]]\n"
	"Graph curves and functions in the terminal\n"
	"\n"
	"    -i, --input=EQUATION     Add an equation for a curve\n"
	"    -c, --color=COLOR        Set the color of the curve specified before (def: red)\n"
	"    -e, --center=XPOS,YPOS   Position of the center of the grid (def: 0,0)\n"
	"    -h, --height=UNITS       Height of grid as float (def: 10)\n"
	"    -w, --width=UNITS        Width of grid as float (def: 10)\n"
	"    -x, --intersects         Only calculate and print the intersections\n"
	"                             of the given curves\n"
	"    -?, --help               Give this help list\n"
	"        --usage              Give a short usage message\n"
	"\n"
	"Mandatory or optional arguments to long options are also mandatory or optional\n"
	"for any corresponding short options.\n"
	"\n"
	"Colors are designated as red: r, green: g, blue: b, cyan: c, yellow: y, or magenta: m\n"
	"\nGraph Mode Keys:\n"
	"    Arrows / hjkl - Move graph\n"
	"    Shift Arrows / HJKL - Resize horizontally and vertically\n"
	"    '=' - Zoom In\n"
	"    '-' - Zoom Out\n"
	"    '0' - Return to default Zoom Level\n"
	"    n or N - Find Intersections between curves\n"
	"    c or C - Clear all Intersections\n"
	"    , or < - Move to prior Intersection\n"
	"    . or > - Move to next Intersection\n"
	"    Control-A (^A) - Switch to Gallery Mode and Create new textbox\n"
	"    g or G - Switch to Gallery Mode\n"
	"    Control-C (^C) or Control-Z (^Z) or q or Q - Exit\n"
	"\nGallery Mode Keys:\n"
	"    Left & Right Arrows - Move within textbox or change color\n"
	"    Up & Down Arrows - Move between textboxes and to color picker\n"
	"    Backspace - Remove character before cursor\n"
	"    Home - Go to beginning of textbox\n"
	"    End - Go to end of textbox\n"
	"    Control-A (^A) - Create new textbox at bottom of gallery\n"
	"    Control-D (^D) - Delete currently selected textbox and equation\n"
	"    Esc - Switch to Graph Mode\n"
	"    Control-C (^C) or Control-Z (^Z) - Exit\n"
	"\nAvailable builtin functions include sqrt, cbrt, exp, ln, log10, sin, cos, tan,\n"
	"sec, csc, cot, sinh, cosh, tanh, asin, acos, atan, atan2, abs, ceil, and floor\n"
	"\n"
;

// Usage message
const char usage_msg[] = 
	"Usage: skedia [-? | --help] [-w WIDTH] [-h HEIGHT] [-e XPOS,YPOS]\n"
	"              [-x | --intersects] [-i EQU1 [-c COL1] [-i EQU2 ...]]\n"
;



// Forward declaration
static int handle_arg(int key, char *arg, struct args_s *prms);

// Parse list of command line arguments using getopt
void parse_args(struct args_s *args, int argc, char *argv[]){
	int indexptr, key;
	opterr = 0;  // Send errors to key as '?' instead of printing error
	while((key = getopt_long(argc, argv, ":?w:h:e:xi:c:", longopts, &indexptr)) != -1){
		if((key = handle_arg(key, optarg, args)) >= 0){
			exit(key);
		}
	}
}

// Parse each argument
// Returns -1 if no exit necessary
// Returns an exit code greater than or equal to zero to indicate an exit
static int handle_arg(int key, char *arg, struct args_s *prms){
	double x, y;
	bool iserr = 0;  // Indicate if error while parsing occurred
	equat_t tmp;
	switch(key){
		// Print Help & Exit
		case '?':
			printf("%s", help_msg);
		return 0;
		
		// Print Usage
		case 10:
			printf("%s", usage_msg);
		return 0;
		
		// Error occurred while getopt was parsing
		case ':': iserr = 1;
		break;
		
		// Print Usage
		// Get window location and dimensions
		case 'w':
			if(sscanf(arg, "%lf", &x)){
				setdims_graph(prms->grp, x, prms->grp->hei);
			}else iserr = 1;
		break;
		case 'h':
			if(sscanf(arg, "%lf", &y)){
				setdims_graph(prms->grp, prms->grp->wid, y);
			}else iserr = 1;
		break;
		// Window center
		case 'e':
			if(sscanf(arg, "%lf,%lf", &x, &y) != EOF){
				// Set upper left corner of graph
				prms->grp->x = x - prms->grp->wid / 2;
				prms->grp->y = y + prms->grp->hei / 2;
			}else iserr = 1;
		break;
		
		// Set curve color
		case 'c':
			if(strlen(arg) > 1) break;
			
			// If there are no equalities no colors may be specified
			if(!*(prms->gallery)) break;
			
			// Get last equation / textbox in gallery
			for(tmp = *(prms->gallery); tmp->next; tmp = tmp->next){}
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
			tmp = add_equat(prms->gallery, arg);
			
			// Parse text and check for errors
			if(parse_equat(*(prms->gallery), tmp) != ERR_OK){
				// If there is an error while parsing return it
				fprintf(stderr, "Error %s while reading equation: %s\n", parse_errstr[(tmp)->err], arg);
				// If expressions were created during parsing deallocate them
				if(!(tmp->is_variable) && tmp->left) free_expr(tmp->left);
				if(tmp->right) free_expr(tmp->right);
				free(tmp);
				
				iserr = 1;
			}
		break;
		case 'x': prms->only_intersects = 1;
		break;
		
		// Error if unknown option encountered
		default: iserr = 1;
		break;
	}
	
	// Print Usage if error occurred
	if(iserr){
		printf("%s", usage_msg);
		return 1;
	}
	
	return -1;
}


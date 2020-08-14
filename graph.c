#include <math.h>
#include <string.h>
#include "graph.h"


bool to_graph(graph_t gr, int tx, int ty, double *px, double *py){
	int tw, th; // Store window width and height
	getmaxyx(gr.win, th, tw);
	if(px) *px = gr.x + tx * gr.wid / tw;
	if(py) *py = gr.y - ty * gr.hei / th;
	
	return 0 <= tx && tx < tw && 0 <= ty && ty < th;
}

bool from_graph(graph_t gr, double px, double py, int *tx, int *ty){
	int tw, th; // Store window width and height
	getmaxyx(gr.win, th, tw);
	if(tx) *tx = (int) ((px - gr.x) * tw / gr.wid);
	if(ty) *ty = (int) ((gr.y - py) * th / gr.hei);
	
	return gr.x <= px && px < gr.x + gr.wid && gr.y - gr.hei < py && py <= gr.y;
}



void zoom_graph(graph_t *gr, double scaleX, double scaleY){
	if(scaleX != 1){
		// Move gr.px so that center of viewport remains unmoved horizontally
		gr->x -= gr->wid * (scaleX - 1) / 2;
		gr->wid *= scaleX;
	}
	
	if(scaleY != 1){
		// Move gr.py so that center of viewport remains unmoved vertically
		gr->y += gr->hei * (scaleY - 1) / 2;
		gr->hei *= scaleY;
	}
}

void setdims_graph(graph_t *gr, double w, double h){
	gr->x += (gr->wid - w) / 2;
	gr->y -= (gr->hei - h) / 2;
	
	gr->wid = w;
	gr->hei = h;
}



void draw_gridlines(graph_t gr){
	// Width and height between gridlines
	double lgw, lgh;
	lgw = log10(gr.wid / 2.5);
	lgh = log10(gr.hei / 2.5);
	
	double cw, ch;
	// Use greatest power of ten less than the width and height
	cw = pow(10, floor(lgw));
	ch = pow(10, floor(lgh));
	// Check if demarcations of 2 * 10^p would work
	// log10(2) = 0.69897000433
	cw *= lgw - floor(lgw) > 0.69897000433 ? 5 : 1;
	ch *= lgh - floor(lgh) > 0.69897000433 ? 5 : 1;
	
	
	// Pre-calculate the initial values of x0 and y0 for multiple uses
	double x0_init, y0_init;
	x0_init = cw * floor((gr.x + gr.wid) / cw);
	y0_init = ch * (1 + floor((gr.y - gr.hei) / ch));
	
	// Store the width and height of the terminal window
	int tw, th;
	getmaxyx(gr.win, th, tw);
	
	// Calculate location of origin on screen
	int zeroX, zeroY;
	from_graph(gr, 0, 0, &zeroX, &zeroY);
	
	int x, y;
	double x0, y0; // Store location of gridlines
	for(x0 = x0_init; x0 > gr.x; x0 -= cw){
		from_graph(gr, x0, 0, &x, NULL);
		
		// Draw vertical line
		for(y = 0; y < th; y++){
			mvwaddch(gr.win, y, x, x == zeroX ? '$' : '|');
		}
		
		// Draw labels
		mvwprintw(gr.win, 0, x, "%.10lg", x0);
	}
	
	for(y0 = y0_init; y0 <= gr.y; y0 += ch){
		from_graph(gr, 0, y0, NULL, &y);
		
		// Prevent second loop from overwriting labels made by first
		if(y == 0) continue;
		
		// Draw horizontal line
		for(x = 0; x < tw; x++){
			mvwaddch(gr.win, y, x, y == zeroY ? '=' : '-');
		}
		
		// Redraw intersections as '+'
		for(x0 = x0_init; x0 > gr.x; x0 -= cw){
			from_graph(gr, x0, 0, &x, NULL);
			mvwaddch(gr.win, y, x, y == zeroY ? '#' : '+');
		}
		
		// Draw labels
		mvwprintw(gr.win, y, 0, "%.10lg", y0);
	}
}



/* Patterns
 * a_0 --- a_1
 *  |       |
 *  |       |
 * a_2 --- a_3
 *  x = a_3 * 8 + a_2 * 4 + a_1 * 2 + a_0
 *  pattern_to_char[x] -> character to place in cell
 */
static char pattern_to_char[] = {
	' ', '\'', '`', '-', // 0 - 3
	'.', '|', '+', ',',  // 4 - 7
	',', '+', '|', '.', // 8 - 11
	'-', '`', '\'', ' '  // 12 - 15
};

// Set and get bits from a bit array 
#define setbit(ba, i, v) ((char*)ba)[(i) / sizeof(char)] |= ((v) & 0x01) << ((i) % sizeof(char))
#define getbit(ba, i) ((((char*)ba)[(i) / sizeof(char)] >> ((i) % sizeof(char))) & 0x01)

void draw_curve(graph_t gr, double (*func)(void*, double, double), void *input){
	int x, y;
	int tw, th; // Store terminal window dimensions
	getmaxyx(gr.win, th, tw);
	
	// Store size of grid in x
	int sz = (tw + 1) * (th + 1);
	sz = (sz % sizeof(char) != 0) + sz / sizeof(char);
	// Create array to store signs of grid points bitwise in memory
	char ispos[sz];
	memset(ispos, 0, sz);
	
	double px, py;
	int i = 0;
	// Collect the signs from each point in the grid
	for(x = 0; x <= tw; x++){
		to_graph(gr, x, 0, &px, NULL);
		for(y = 0; y <= th; y++){
			to_graph(gr, 0, y, NULL, &py);
			setbit(ispos, i, func(input, px, py) >= 0);
			i++;
		}
	}
	
	char acc;
	i = 0;
	for(x = 0; x < tw; x++){
		for(y = 0; y < th; y++){
			// Use acc to bitwise accumulate the corners
			acc = 0;
			acc |= getbit(ispos, i);
			acc |= getbit(ispos, i + (th + 1)) << 1;
			acc |= getbit(ispos, i + 1) << 2;
			acc |= getbit(ispos, i + (th + 1) + 1) << 3;
			
			acc = pattern_to_char[acc];
			if(acc != ' ') mvwaddch(gr.win, y, x, acc);
			
			i++;
		}
		
		// This loop iterates over one fewer iteration that the above
		// So it is necessary to move over one more to get to the next row
		i++;
	}
}

void draw_func(graph_t gr, double (*func)(void*, double), void *input, bool isx_out){
	int tw, th;
	getmaxyx(gr.win, th, tw);
	
	int win, wout;
	
	char acc;
	double gin, gout;
	int x, y, top;
	double gx, gy;
	if(isx_out){
		// Function for x in terms of y
		for(y = 0; y < tw; y++){
			// Get location of curve at beginning of cell
			to_graph(gr, 0, y, NULL, &gy);
			gx = func(input, gy);
			from_graph(gr, gx, 0, &x, NULL);
			
			// Get location of curve at end of cell
			to_graph(gr, 0, y + 1, NULL, &gy);
			gx = func(input, gy);
			from_graph(gr, gx, 0, &top, NULL);
			
			if(top == x){
				// If top == x then the line passes vertically through the cell
				mvwaddch(gr.win, y, x, '|');
				continue;
			}else{
				// Draw end pieces of horizontal line
				if(0 < x && x < tw){
					mvwaddch(gr.win, y, x, top < x ? '\'' : '`');
				}
				
				if(0 < top && top < tw){
					mvwaddch(gr.win, y, top, top < x ? ',' : '.');
				}
			}
			
			// Draw horizontal line
			for(top > x ? x++ : x--; x != top; top > x ? x++ : x--){
				if(0 < x && x < tw) mvwaddch(gr.win, y, x, '-');
			}
		}
	}else{
		// Function for y in terms of x
		for(x = 0; x < tw; x++){
			// Get location of curve at beginning of cell
			to_graph(gr, x, 0, &gx, NULL);
			gy = func(input, gx);
			from_graph(gr, 0, gy, NULL, &y);
			
			// Get location of curve at end of cell
			to_graph(gr, x + 1, 0, &gx, NULL);
			gy = func(input, gx);
			from_graph(gr, 0, gy, NULL, &top);
			
			if(top == y){
				// If top == y then the line passes horizontally through the cell
				mvwaddch(gr.win, y, x, '-');
				continue;
			}else{
				// Draw end pieces of vertical line
				if(0 < y && y < th){
					mvwaddch(gr.win, y, x, top < y ? '\'' : '.');
				}
				
				if(0 < top && top < th){
					mvwaddch(gr.win, top, x, top < y ? ',' : '`');
				}
			}
			
			// Draw vertical line
			for(top > y ? y++ : y--; y != top; top > y ? y++ : y--){
				if(0 < y && y < th) mvwaddch(gr.win, y, x, '|');
			}
		}
	}
}


bool draw_point(graph_t gr, double x, double y, const chtype ch){
	int tx, ty;
	// When (x, y) is within the bounds of the screen
	if(from_graph(gr, x, y, &tx, &ty)){
		mvwaddch(gr.win, ty, tx, ch);
		return 1;
	}
	return 0;
}

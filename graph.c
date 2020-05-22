#include <math.h>
#include <string.h>
#include "graph.h"

bool to_graph(graph_t gr, int tx, int ty, double *px, double *py){
	if(px) *px = gr.px + (tx - gr.tx) * gr.pw / gr.tw;
	if(py) *py = gr.py - (ty - gr.ty) * gr.ph / gr.th;
	
	return gr.tx <= tx && tx < gr.tx + gr.tw && gr.ty <= ty && ty < gr.ty + gr.th;
}

bool from_graph(graph_t gr, double px, double py, int *tx, int *ty){
	if(tx) *tx = gr.tx + (int) ((px - gr.px) * gr.tw / gr.pw);
	if(ty) *ty = gr.ty + (int) ((gr.py - py) * gr.th / gr.ph);
	
	return gr.px <= px && px < gr.px + gr.pw && gr.py - gr.ph < py && py <= gr.py;
}



void zoom_graph(graph_t *gr, double scaleX, double scaleY){
	if(scaleX != 1){
		// Move gr.px so that center of viewport remains unmoved horizontally
		gr->px -= gr->pw * (scaleX - 1) / 2;
		gr->pw *= scaleX;
	}
	
	if(scaleY != 1){
		// Move gr.py so that center of viewport remains unmoved vertically
		gr->py += gr->ph * (scaleY - 1) / 2;
		gr->ph *= scaleY;
	}
}

void setdims_graph(graph_t *gr, double w, double h){
	gr->px += (gr->pw - w) / 2;
	gr->py -= (gr->ph - h) / 2;
	
	gr->pw = w;
	gr->ph = h;
}



void draw_gridlines(graph_t gr){
	// Width and height between gridlines
	double lgw, lgh;
	lgw = log10(gr.pw / 2.5);
	lgh = log10(gr.ph / 2.5);
	
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
	x0_init = cw * floor((gr.px + gr.pw) / cw);
	y0_init = ch * (1 + floor((gr.py - gr.ph) / ch));
	
	// Calculate location of origin on screen
	int zeroX, zeroY;
	from_graph(gr, 0, 0, &zeroX, &zeroY);
	
	int x, y;
	double x0, y0; // Store location of gridlines
	for(x0 = x0_init; x0 > gr.px; x0 -= cw){
		from_graph(gr, x0, 0, &x, NULL);
		
		// Draw vertical line
		for(y = gr.ty; y < gr.ty + gr.th; y++){
			mvaddch(y, x, x == zeroX ? '$' : '|');
		}
		
		// Draw labels
		mvprintw(gr.ty, x, "%.4lg", x0);
	}
	
	for(y0 = y0_init; y0 <= gr.py; y0 += ch){
		from_graph(gr, 0, y0, NULL, &y);
		
		// Prevent second loop from overwriting labels made by first
		if(y == gr.ty) continue;
		
		// Draw horizontal line
		for(x = gr.tx; x < gr.tx + gr.tw; x++){
			mvaddch(y, x, y == zeroY ? '=' : '-');
		}
		
		// Redraw intersections as '+'
		for(x0 = x0_init; x0 > gr.px; x0 -= cw){
			from_graph(gr, x0, 0, &x, NULL);
			mvaddch(y, x, y == zeroY ? '#' : '+');
		}
		
		// Draw labels
		mvprintw(y, gr.tx, "%.4lg", y0);
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
	
	// Store size of grid in x
	int sz = (gr.tw + 1) * (gr.th + 1);
	sz = (sz % sizeof(char) != 0) + sz / sizeof(char);
	// Create array to store signs of grid points bitwise in memory
	char ispos[sz];
	memset(ispos, 0, sz);
	
	double px, py;
	int i = 0;
	// Collect the signs from each point in the grid
	for(x = gr.tx; x <= gr.tw + gr.tx; x++){
		to_graph(gr, x, 0, &px, NULL);
		for(y = gr.ty; y <= gr.ty + gr.th; y++){
			to_graph(gr, 0, y, NULL, &py);
			setbit(ispos, i, func(input, px, py) >= 0);
			i++;
		}
	}
	
	char acc;
	i = 0;
	for(x = gr.tx; x < gr.tw + gr.tx; x++){
		for(y = gr.ty; y < gr.th + gr.ty; y++){
			// Use acc to bitwise accumulate the corners
			acc = 0;
			acc |= getbit(ispos, i);
			acc |= getbit(ispos, i + (gr.th + 1)) << 1;
			acc |= getbit(ispos, i + 1) << 2;
			acc |= getbit(ispos, i + (gr.th + 1) + 1) << 3;
			
			acc = pattern_to_char[acc];
			if(acc != ' ') mvaddch(y, x, acc);
			
			i++;
		}
		
		// This loop iterates over one fewer iteration that the above
		// So it is necessary to move over one more to get to the next row
		i++;
	}
}

void draw_func(graph_t gr, double (*func)(void*, double), void *input, bool isx_out){
	int win, wout;
	
	char acc;
	double gin, gout;
	int x, y, top;
	double gx, gy;
	if(isx_out){
	}else{
		for(x = gr.tx; x < gr.tx + gr.tw; x++){
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
				mvaddch(y, x, '-');
				continue;
			}else{
				// Draw end pieces of vertical line
				if(gr.ty < y && y < gr.ty + gr.th){
					mvaddch(y, x, top < y ? '\'' : '.');
				}
				
				if(gr.ty < top && top < gr.ty + gr.th){
					mvaddch(top, x, top < y ? ',' : '`');
				}
			}
			
			// Draw vertical line
			for(top > y ? y++ : y--; y != top; top > y ? y++ : y--){
				if(gr.ty < y && y < gr.ty + gr.th) mvaddch(y, x, '|');
			}
		}
	}
}

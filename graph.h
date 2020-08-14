#ifndef _GRAPH_H
#define _GRAPH_H

#include <ncurses.h>

typedef struct{
	// Ncurses window to draw graph to
	WINDOW* win;
	
	// Define the characteristics of the graph within the plane
	// X, Y coordinate of the left corner within the plane
	double x, y;
	// Width and Height of the graph in the plane
	double wid, hei;
} graph_t;


// Both return booleans indicating whether the given values are within bounds
// Convert from the location in the window to the graph coordinates
bool to_graph(graph_t gr, int tx, int ty, double *px, double *py);
// Convert from the graph coordinates to the location in the window
bool from_graph(graph_t gr, double px, double py, int *tx, int *ty);

// Zoom viewport in or out by scale keeping the center where it is
// If scale > 1 then vw will zoom out
// If scale < 1 then vw will zoom in
void zoom_graph(graph_t *gr, double scaleX, double scaleY);
// Set the width and height of the viewport while keeping the center where it is
void setdims_graph(graph_t *gr, double w, double h);

// x, y, w, h describe the position and size of the graph in the window
void draw_gridlines(graph_t gr);
// Draw a curve defined by func(x, y) == 0
void draw_curve(graph_t gr, double (*func)(void*, double, double), void *input);
// Draw function defined by func(x) = y
// If isx_out = 1 then func(y) = x
void draw_func(graph_t gr, double (*func)(void*, double), void *input, bool isx_out);
// Draw point at (x, y) using the provided character
// Returns whether the point fell within the bounds of the graph
bool draw_point(graph_t gr, double x, double y, const chtype ch);

#endif

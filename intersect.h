#ifndef _INTERSECT_H
#define _INTERSECT_H

#include <stdbool.h>

// Rectangle in which to search using a lattice of points with a certain number of rows and columns
struct bound_s{
	// Location of top left corner
	double x, y;
	double width, height;
	
	int rows, columns;
};

// Point within the (x, y) plane
typedef struct{
	double x, y;
} point_t;

// Represent an intersection between curves (Structured in Circular Linked List)
typedef struct inter_s{
	double x, y;  // Location of intersection
	
	// Store functions used to generate intersection
	double (*func1)(void*, double, double);
	double (*func2)(void*, double, double);
	void *param1, *param2;
	
	struct inter_s *prev, *next;  // Pointers to neighbors in circular linked list
} *inter_t;


/* Calculate the points where f1(x, y) == 0 and f2(x, y) == 0
 * Each call will return another intersection
 *   WARNING: This function cannot be run on multiple function pairs at once
 * 
 * Usage:
 *   struct bound_s rect = {-1, -1, 2, 2, 2, 2}; // Bound centered at (0, 0) with width of 2 and height of 2
 *   // With grid points at (-1, -1), (-1, 1), (1, -1), (1, 1)
 *   
 *   point_t pt1, pt2;
 *   // Half the initial bounding area of crossing 3 times
 *   pt1 = curve_inters(rect, 3, f1, prm1, f2, prm2);
 *   // Half the initial bounding area of crossing 4 times
 *   pt2 = curve_inters(rect, 4, NULL, NULL, NULL, NULL);  // rect need not be provided for later calls
 * 
 * Arguments:
 *   struct bound_s rect : Bounding area in which to search for crossings
 *   int depth : Number of times to halve the bounding area once a crossing is found
 *   double (*f1)(void*, double, double) : First function to call on (x, y)
 *   void *inp1 : Parameters to pass to f1 when evaluating (x, y) i.e. f1(inp1, x, y)
 *   double (*f2)(void*, double, double) : Second function to call on (x, y)
 *   void *inp2 : Parameters to pass to f2 when evaluating (x, y) i.e. f2(inp2, x, y)
 * 
 * Returns:
 *   point_t : Location of an intersection
 *   bool *success : Whether another point was found
 */
point_t curve_inters(
	struct bound_s rect,
	double (*f1)(void*, double, double), void *inp1,
	double (*f2)(void*, double, double), void *inp2,
	int depth, bool *success
);

/* Calculates intersections and inserts them into the provided circular linked list
 * after the element pointed to by inters.
 * 
 * Arguments:
 *   inter_t *inters : Pointer to circular linked list to place intersections in
 *   struct bound_s rect : Bounding area in which to search for crossings
 *   double (*f1)(void*, double, double) : First function to call on (x, y)
 *   void *inp1 : Parameters to pass to f1 when evaluating (x, y) i.e. f1(inp1, x, y)
 *   double (*f2)(void*, double, double) : Second function to call on (x, y)
 *   void *inp2 : Parameters to pass to f2 when evaluating (x, y) i.e. f2(inp2, x, y)
 *   
 *   int depth : Number of times to halve the bounding area once a crossing is found
 *   double prec : Distance in which new intersections will not be accepted
 *     EXAMPLE: (0, 1) is in inters and prec = 0.02 if (0, 1.019) is found then it will not be included
 *   
 * Returns:
 *   inter_t : Pointer into circular linked list
 */
inter_t append_inters(
	inter_t *inters,
	struct bound_s rect,
	double (*f1)(void*, double, double), void *inp1,
	double (*f2)(void*, double, double), void *inp2,
	int depth, double prec
);

/* Check if their is a point in inters that falls within dist of pt
 * 
 * Arguments:
 *   inter_t inters : List of intersections to search
 *   point_t pt : Point around which to check for intersections
 *   double dist : Distance between intersection and point
 * 
 * Returns:
 *   bool : Whether there is an intersection within dist of pt
 */
bool contains_inter(inter_t inters, point_t pt, double dist);

/* Remove intersection generated using func and inp from inters 
 * An intersection is removed if
 *   func1 == func and param1 == inp  or
 *   func2 == func and param2 == inp
 * 
 * Arguments:
 *   inter_t *inters : Pointer to a circular list of intersections
 *   double (*func)(void*, double, double) : Function pointer used to identify the intersection
 *   void *inp : Parameters used to identify the intersection
 * 
 * Returns:
 *   bool : Whether an intersection was removed from inters
 */
bool remove_inter(inter_t *inters, double (*func)(void*, double, double), void *inp);

/* Deallocate memory of a circular list of intersections that were allocated on the heap
 * 
 * Arguments:
 *   inter_t inters : Pointer to linked list to deallocate
 */
void free_inters(inter_t inters);

#endif

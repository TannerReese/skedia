#include <stdlib.h>
#include <math.h>

#include "intersect.h"

// Store current functions
static double (*fn1)(void*, double, double) = NULL;
static double (*fn2)(void*, double, double) = NULL;
// Store their parameters;
static void *prm1, *prm2;



// Traingular search area
struct triag_s{
	// Vertices of triangular region
	point_t a, b, c;
};

/* Calculate the triangle half the size and inverted to tr
 *       +
 *      / \
 *     #---#
 *    / \ / \
 *   +---#---+
 *  +  -->  tr
 *  #  -->  htr
 */
static struct triag_s invert_triag(struct triag_s tr){
	struct triag_s htr;
	
	htr.a.x = (tr.b.x + tr.c.x) / 2;
	htr.a.y = (tr.b.y + tr.c.y) / 2;
	
	htr.b.x = (tr.a.x + tr.c.x) / 2;
	htr.b.y = (tr.a.y + tr.c.y) / 2;
	
	htr.c.x = (tr.a.x + tr.b.x) / 2;
	htr.c.y = (tr.a.y + tr.b.y) / 2;
	
	return htr;
}

// Return value indicating if f1(x, y) <= 0 or f1(x, y) > 0 as well as f2(x, y) <= 0 or f2(x, y) > 0
static char check_point(point_t pt){
	char val = 0;
	
	// Store f2 info in second LSB
	val |= (fn1(prm1, pt.x, pt.y) <= 0) & 0x1;
	val <<= 1;
	// Store f1 info in LSB
	val |= (fn2(prm2, pt.x, pt.y) <= 0) & 0x1;
	
	return val;
}

// Checks if triangle contains both curves using check_point return value
#define check_triag(ach, bch, cch) (((ach) ^ (bch)) == 0b11 || ((bch) ^ (cch)) == 0b11 || ((cch) ^ (ach)) == 0b11)

// Calculate the precise location of crossing
// Uses static variables fn1 and fn2
/* Arguments:
 *   struct triag_s tr : Triangle in which to narrow down point
 *   char a_chk : Result of applying check_point to tr.a
 *   char b_chk : ''                                tr.b
 *   char c_chk : ''                                tr.c
 *      NOTE: a_chk, b_chk, c_chk are provided to reduce redundant calculations
 *   int depth : Number of halvings to perform on tr
 * 
 * Returns:
 *   point_t : Location of crossing
 *   bool *success : Whether a crossing was in fact present in tr
 */
static point_t isolate_inter(struct triag_s tr, char a_chk, char b_chk, char c_chk, int depth, bool *success){
	struct triag_s htr;
	
	// Store the result of check_point for each vertex in htr
	char ha_chk, hb_chk, hc_chk;
	// Indicate if the sub-triangles boarding vertex a, b, c, or center 'htr' contain both curves
	bool tA, tB, tC, tM;
	while(depth > 0){
		depth--;
		
		// Generate half-sized inverted triangle
		htr = invert_triag(tr);
		
		// Evaluate all vertices of htr
		ha_chk = check_point(htr.a);
		hb_chk = check_point(htr.b);
		hc_chk = check_point(htr.c);
		
		// Check if curves cross through each of the four sub-triangles
		tA = check_triag(a_chk, hb_chk, hc_chk);
		tB = check_triag(ha_chk, b_chk, hc_chk);
		tC = check_triag(ha_chk, hb_chk, c_chk);
		tM = check_triag(ha_chk, hb_chk, hc_chk);
		
		if(tA && !tB && !tC && !tM){  // Both Curves only pass through sub-triangle boarding vertex a
			tr.b = htr.b;
			tr.c = htr.c;
			b_chk = hb_chk;
			c_chk = hc_chk;
		}else if(!tA && tB && !tC && !tM){  // Both Curves only pass through sub-triangle boarding vertex b
			tr.a = htr.a;
			tr.c = htr.c;
			a_chk = ha_chk;
			c_chk = hc_chk;
		}else if(!tA && !tB && tC && !tM){  // Both Curves only pass through sub-triangle boarding vertex c
			tr.a = htr.a;
			tr.b = htr.b;
			a_chk = ha_chk;
			b_chk = hb_chk;
		}else if(!tA && !tB && !tC && tM){  // Both Curves only pass through center sub-triangle
			tr.a = htr.a;
			tr.b = htr.b;
			tr.c = htr.c;
			a_chk = ha_chk;
			b_chk = hb_chk;
			c_chk = hc_chk;
		}else if(!tA && !tB && !tC && !tM){  // No sub-triangle contains both curves
			*success = 0;
			point_t pt = {0, 0};
			return pt;
		}else{  // Multiple sub-triangles contain both curves
			point_t pt;
			struct triag_s ntr;  // Triangle to be passed to recursive call
			
			// Check for valid crossing in sub-triangle boarding a
			if(tA){
				ntr.a = tr.a;
				ntr.b = htr.b;
				ntr.c = htr.c;
				
				pt = isolate_inter(ntr, a_chk, hb_chk, hc_chk, depth, success);
				if(*success) return pt;
			}
			
			// Check for valid crossing in sub-triangle boarding b
			if(tB){
				ntr.a = htr.a;
				ntr.b = tr.b;
				ntr.c = htr.c;
				
				pt = isolate_inter(ntr, ha_chk, b_chk, hc_chk, depth, success);
				if(*success) return pt;
			}
			
			// Check for valid crossing in sub-triangle boarding c
			if(tC){
				ntr.a = htr.a;
				ntr.b = htr.b;
				ntr.c = tr.c;
				
				pt = isolate_inter(ntr, ha_chk, hb_chk, c_chk, depth, success);
				if(*success) return pt;
			}
			
			// Check for valid crossing in center sub-triangle
			if(tM){
				ntr.a = htr.a;
				ntr.b = htr.b;
				ntr.c = htr.c;
				
				pt = isolate_inter(ntr, ha_chk, hb_chk, hc_chk, depth, success);
				if(*success) return pt;
			}
			
			// None of the sub-triangles contained a crossing
			*success = 0;
			return pt;
		}
	}
	
	// Depth reached and so a valid crossing is found
	*success = 1;
	
	// Take center of tr as approximation of crossing
	point_t pt = {(tr.a.x + tr.b.x + tr.c.x) / 3, (tr.a.y + tr.b.y + tr.c.y) / 3};
	return pt;
}



point_t curve_inters(
	struct bound_s rect,
	double (*f1)(void*, double, double), void *inp1,
	double (*f2)(void*, double, double), void *inp2,
	int depth, bool *success
){
	// Store prior and current row of check points
	static char *priorRow, *currRow;
	/* Grid Example ; Center=(0,0) Width,Height=(2,2) Rows,Cols=(4,4)
	 *   '*' represents grid points in memory
	 *   '.' represents grid points out of memory
	 *   '?' represents unevaluated grid points
	 *   '*^' is the lattice point located at point_t loc
	 *   'X' is the cell currently being checked for intersections
	 *   
	 *   (-1, 1) .   .   .   .   . (1, 1)
	 *   
	 *           .   .   .   .   .
	 *   
	 * priorRow  *   *---*   *   *
	 *               | X |
	 * currRow   *   *---*^  ?   ?
	 *   
	 *  (-1, -1) ?   ?   ?   ?   ? (-1, 1)
	 */
	
	// Information for iterating through grid within search area
	static double cwid, chei;  // Distance between consecutive columns and rows, respectively
	static point_t loc;  // Position of current lattice point
	static int col, rowlen;  // The Column Index of current lattive point in currRow and the maximum column index (i.e. row length)
	static double minx, miny;  // A Lower Bound for the x and y values of loc
	static bool checking_upper, skip_lower;  // Whether the upper triangle is being checked and whether the lower triangle should be skipped
	
	// Check if curve_inters is being reset / started
	if(f1 && f2){
		// Clean up previous allocation if present
		if(priorRow) free(priorRow);
		if(currRow) free(currRow);
		
		// Set function pointers
		fn1 = f1;
		fn2 = f2;
		// Set function parameters
		prm1 = inp1;
		prm2 = inp2;
		
		// Allocate rowlen number of char's for currRow and priorRow
		rowlen = rect.columns + 1;
		priorRow = malloc(sizeof(char) * rowlen);
		currRow = malloc(sizeof(char) * rowlen);
		// Set loc to the top left corner
		loc.x = rect.x;
		loc.y = rect.y;
		// Calculate the cell width and height
		cwid = rect.width / rect.columns;
		chei = rect.height / rect.rows;
		// Set lower bounds
		minx = rect.x;
		miny = rect.y - rect.height - chei / 2;  // miny must be slightly lower than grid to ensure proper detection of end condition
		
		// Iterate through first row to calculate priorRow values
		for(col = 0; col < rowlen; col++){
			priorRow[col] = check_point(loc);
			loc.x += cwid;
		}
		
		// Move to first grid point in currRow
		loc.x = minx;
		loc.y -= chei;
		// Calculate first value of currRow
		currRow[0] = check_point(loc);
		
		// Move to next grid point after first
		col = 1;
		loc.x += cwid;
		
		checking_upper = 1;  // Indicate which triangle needs to be checked
		skip_lower = 0;  // Normal operation (not immediately after a continuation call)
	}else if(!fn1 || !fn2){  // When f1 and f2 weren't set but fn1 and fn2 don't have values
		*success = 0;  // Indicate no intersection was found
		point_t pt = {0, 0};
		return pt;
	}
	
	// Triangle to be searched
	struct triag_s tr;
	// Point found
	point_t pt;
	while(loc.y > miny){
		
		/* Triangle Division of Cell
		 *  |/   .    |/   .    |/   .    |
		 *  +---------+---------+---------+
		 *  |  Prior /| Upper  /|   Next /|
		 *  |   Cell  |      /  |   Cell  |
		 *  |         |    /    |   Not   |
		 *  | Checked |  /      | Checked |
		 *  |/   .    |/  Lower |/   ?    |
		 *  +---------+---------+---------+
		 *  |    ?   /|    ?   /|    ?   /|
		 */
		
		if(checking_upper){
			// Check current grid point
			currRow[col] = check_point(loc);
			
			checking_upper = 0;
			// Check upper triangle
			if(check_triag(currRow[col - 1], priorRow[col], priorRow[col - 1])){
				tr.a.x = loc.x - cwid;
				tr.a.y = loc.y;
				tr.b.x = loc.x;
				tr.b.y = loc.y + chei;
				tr.c.x = loc.x - cwid;
				tr.c.y = loc.y + chei;
				
				pt = isolate_inter(tr, currRow[col - 1], priorRow[col], priorRow[col - 1], depth, success);
				if(*success) return pt;
			}
		}else{
			// Check lower triangle
			if(!skip_lower && check_triag(priorRow[col], currRow[col], currRow[col - 1])){
				// Indicate that when the 
				skip_lower = 1;
				
				tr.a.x = loc.x;
				tr.a.y = loc.y + chei;
				tr.b.x = loc.x;
				tr.b.y = loc.y;
				tr.c.x = loc.x - cwid;
				tr.c.y = loc.y;
				
				pt = isolate_inter(tr, priorRow[col], currRow[col], currRow[col - 1], depth, success);
				if(*success) return pt;
			}
			
			// Move back into checking the upper triangle for the next iteration
			checking_upper = 1;
			// Switch back to regular operation where the lower triangle is not skipped
			skip_lower = 0;
			
			// Move to next 
			col++;
			loc.x += cwid;
			if(col >= rowlen){
				// Move column index to beginning
				col = 0;
				// Reset loc to beginning of currRow when it reaches the end 
				loc.x = minx;
				loc.y -= chei;
				
				// Move currRow to priorRow
				char *tmp;
				tmp = priorRow;
				priorRow = currRow;
				currRow = tmp;
				
				// Check first value in row
				currRow[col] = check_point(loc);
				// Move to next point
				col++;
				loc.x += cwid;
			}
		}
	}
	
	*success = 0;
	return pt;
}



bool contains_inter(inter_t inters, point_t pt, double dist){
	if(!inters) return 0;
	
	inter_t inr = inters;
	do{
		if(hypot(pt.x - inr->x, pt.y - inr->y) < dist) return 1;
		
		inr = inr->next;
	}while(inr != inters);
	
	return 0;
}

inter_t append_inters(
	inter_t *inters,
	struct bound_s rect,
	double (*f1)(void*, double, double), void *inp1,
	double (*f2)(void*, double, double), void *inp2,
	int depth, double prec
){
	bool success;
	point_t pt;
	inter_t new_inter;
	pt = curve_inters(rect, f1, inp1, f2, inp2, depth, &success);
	
	// Insert intersection points into inters while more are found
	while(success){
		// Check if new intersection overlaps with a prior one
		if(!contains_inter(*inters, pt, prec)){
			// Convert pt into an intersection
			new_inter = malloc(sizeof(struct inter_s));
			// Identify the location of the intersection
			new_inter->x = pt.x;
			new_inter->y = pt.y;
			// Identify the functions used to create the intersection
			new_inter->func1 = f1;
			new_inter->func2 = f2;
			new_inter->param1 = inp1;
			new_inter->param2 = inp2;
			
			// Insert new_inter into the list
			if(*inters){  // When inters has prior elements
				new_inter->prev = *inters;
				new_inter->next = (*inters)->next;
				(*inters)->next->prev = new_inter;
				(*inters)->next = new_inter;
			}else{  // When inters is empty
				new_inter->prev = new_inter;
				new_inter->next = new_inter;
			}
			*inters = new_inter;
		}
		
		// Find next point
		pt = curve_inters(rect, NULL, NULL, NULL, NULL, depth, &success);
	}
	
	return *inters;
}

bool remove_inter(inter_t *inters, double (*func)(void*, double, double), void *inp){
	if(!*inters) return 0;
	
	inter_t inr = *inters;
	do{
		// Check if intersection matches the given function and parameters
		if(inr->func1 == func && inr->param1 == inp || inr->func2 == func && inr->param2 == inp){
			// Check if inters needs to be changed
			if(inr == *inters) *inters = inr->next == inr->prev ? NULL /* When there is only one element */ : inr->prev;
			
			// Change pointers to remove inr from inters
			inr->prev->next = inr->next;
			inr->next->prev = inr->prev;
			free(inr);  // Deallocate memory for intersections
			return 1;
		}
		
		inr = inr->next;
	}while(inr != *inters);
	
	return 0;
}



void free_inters(inter_t inters){
	if(!inters) return;
	
	inter_t inr = inters, tmp;
	do{
		tmp = inr->next;
		free(inr);
		inr = tmp;
	}while(inr != inters);
}

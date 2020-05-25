#ifndef _EXPR_H
#define _EXPR_H

#include <stdbool.h>
#include <stddef.h>


struct expr_s;
typedef struct expr_s *expr_t;

// Allocate memory on heap for new expression
expr_t new_expr(void);
// Free the heap memory allocated for an expr
void free_expr(expr_t exp);
// Evaluate value of expression using given arguments in place of args
double eval_expr(expr_t exp, double *args);

// Replace expressions only dependent on constants by constants
expr_t constify_expr(expr_t exp);
// Check if exp has the same type and relevant parameters as target
bool expr_match(expr_t exp, expr_t target);
// Check if exp contains any expression with the same type and relevant parameters as target
bool expr_depends(expr_t exp, expr_t target);


// Builtin function of any arity
union expr_func_u{
	double (*one_arg)(double);
	double (*two_arg)(double, double);
	double (*n_arg)(double*);
};

// Constructors for certain expressions
// Any children that exp may have before the call will be deallocated
// Creates an expression with a constant value of c
expr_t const_expr(expr_t exp, double c);
// Creates an expression that will evaluate to the value of the arg_ind'th argument
expr_t arg_expr(expr_t exp, int arg_ind);
// Creates an expression that will evaluate to the value of *cache
expr_t cached_expr(expr_t exp, double *cache);

// Apply other expressions or functions
expr_t apply_expr(expr_t exp, expr_t var, int argc, expr_t *args);
expr_t apply_func(expr_t exp, union expr_func_u fn, bool use_n_arg, int argc, expr_t* args);

// Operations on expressions
// Note: res, a, and b will be manipulated by these functions
// If do_inv = 1 then res = a - b
// Otherwise then res = a + b
expr_t add_expr(expr_t res, expr_t a, expr_t b, bool do_inv);
expr_t negate_expr(expr_t exp);
// If do_inv = 1 then res = a / b
// Otherwise then res = a * b
expr_t mul_expr(expr_t res, expr_t a, expr_t b, bool do_inv);
// res = a ^ b
expr_t pow_expr(expr_t res, expr_t a, expr_t b);


#define EXPR_FUNCNAME_LEN 32
// An array of known functions to consult when parsing FUNC1, FUNC2, or FUNCN expr types
// Should be null terminated using {0}
typedef struct{
	// Name of function (case insensitive)
	char name[EXPR_FUNCNAME_LEN];
	// Number of arguments to function
	// If 0 then builtin is treated as constant
	int arity;
	// Whether to use (one_arg_f, two_arg_f) or n_arg_f
	bool use_n_arg;
	
	union{
		// Function pointer
		union expr_func_u func;
		
		// Constant for 0-arity entries
		double value;
	};
} expr_builtin_t;

extern expr_builtin_t expr_builtin_funcs[];



// Function used to obtain expression value for a name
// Returned expr should be of a constant, argument, variable, or function
typedef expr_t (*name_trans_f)(expr_t exp, const char *name, size_t n, void *input);
typedef enum {
	ERR_OK = 0,
	ERR_UNUSED_CHARACTER, ERR_UNRECOGNIZED_NAME,
	ERR_MISSING_VALUE, ERR_EMPTY_EXPRESSION, ERR_TOO_MANY_VALUES,
	ERR_BAD_ARITY,
	ERR_PARENTH_MISMATCH,
	ERR_PARSE_OVERFLOW, ERR_BAD_EXPRESSION
} parse_err_t;
// Allow for conversion from enum to string when printing error
extern const char *parse_errstr[];

expr_t parse_expr(const char *src, name_trans_f callback, const char **endptr, parse_err_t *err, void *trans_inp);

#endif

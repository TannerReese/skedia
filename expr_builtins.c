#include <math.h>

#include "expr.h"

// Provide definitions for functions not provided in math.h
static double sec(double x){
	return 1 / cos(x);
}

static double csc(double x){
	return 1 / sin(x);
}

static double cot(double x){
	return cos(x) / sin(x);
}

expr_builtin_t expr_builtin_funcs[] = {
//  Name, arity, use_n_arg, pointer / constant
	{"pi",    0, 0, {value: 3.14159265358979323846}},
	{"e",     0, 0, {value: 2.71828182845904523536}},
	{"sqrt",  1, 0, {{one_arg: sqrt}}},
	{"cbrt",  1, 0, {{one_arg: cbrt}}},
	
	{"exp",   1, 0, {{one_arg: exp}}},
	{"ln",    1, 0, {{one_arg: log}}},
	{"log10", 1, 0, {{one_arg: log10}}},
	
	{"sin",   1, 0, {{one_arg: sin}}},
	{"cos",   1, 0, {{one_arg: cos}}},
	{"tan",   1, 0, {{one_arg: tan}}},
	
	{"sec",   1, 0, {{one_arg: sec}}},
	{"csc",   1, 0, {{one_arg: csc}}},
	{"cot",   1, 0, {{one_arg: cot}}},
	
	{"sinh",  1, 0, {{one_arg: sinh}}},
	{"cosh",  1, 0, {{one_arg: cosh}}},
	{"tanh",  1, 0, {{one_arg: tanh}}},
	
	{"asin",  1, 0, {{one_arg: asin}}},
	{"acos",  1, 0, {{one_arg: acos}}},
	{"atan",  1, 0, {{one_arg: atan}}},
	{"atan2", 2, 0, {{two_arg: atan2}}},
	
	{"abs",   1, 0, {{one_arg: fabs}}},
	{"ceil",  1, 0, {{one_arg: ceil}}},
	{"floor", 1, 0, {{one_arg: floor}}},
	{0}
};

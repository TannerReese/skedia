#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <ctype.h>

#include "expr.h"


/* Represent the different types of expressions
 * CONST - numeric literals (e.g. 1, 2.5, -3) and constants (e.g. e, pi)
 * ARGS - arguments to expression. Represented by index (e.g. 0 -> first argument)
 * CACHED - evaluates to the value referenced by a double value
 * VAR - outside functions or variables defined by other expressions
 * FUNC1, FUNC2, FUNCN - builtin functions of arity 1, 2, or more, respectively. Represented by function pointer
 * ADD - sum and difference of expressions
 * MULT - product and quotient of expressions
 * POW - exponentiation of expressions
 */
enum expr_type{
	EXPR_CONST, EXPR_ARGS, EXPR_CACHED,
	EXPR_VAR, EXPR_FUNC1, EXPR_FUNC2, EXPR_FUNCN,
	EXPR_ADD, EXPR_MUL, EXPR_POW,
	
	// Not used outside of parsing
	// PARENTH: Represents open parenthesis during parsing
	// COMMA: Represents comma operator which joins values into argument list 
	EXPR_PARENTH, EXPR_COMMA
};


struct expr_s{
	enum expr_type type;
	
	expr_t next; // Used for linked list of children. Links to next sibling
	// Indicates whether to additively invert and/or multiplicatively invert the expression
	bool add_inv : 1, mul_inv : 1;
	
	union{
		// EXPR_CONST
		double constant;
		// EXPR_ARGS
		int arg_ind;
		// EXPR_CACHED
		double *cache;
		
		struct{
			union{
				// EXPR_VAR
				// Reference to expression to call as function
				// Uses children values as arguments to ref
				expr_t ref;
				
				// EXPR_FUNC1, EXPR_FUNC2, EXPR_FUNC3, EXPR_FUNCN
				union expr_func_u func;
			};
			
			// Used by VAR, FUNC1, FUNC2, FUNC3, FUNCN, ADD, MULT, POW
			// Linked list of child expressions used as input for the above
			int child_count;
			expr_t children;
		};
	};
};



// Frees memory used by children of expression only
void free_expr_no_self(expr_t exp){
	// Only free children if given type has children
	if(exp->type == EXPR_VAR
	|| exp->type == EXPR_FUNC1
	|| exp->type == EXPR_FUNC2
	|| exp->type == EXPR_FUNCN
	|| exp->type == EXPR_ADD
	|| exp->type == EXPR_MUL
	|| exp->type == EXPR_POW
	){
		expr_t child = exp->children, tmp;
		while(child){
			free_expr_no_self(child);
			tmp = child->next;
			free(child);
			child = tmp;
		}
	}
}



// Allocate memory on heap for new expression
expr_t new_expr(void){
	return malloc(sizeof(struct expr_s));
}

// Frees all memory used by expr_t including that of children expressions
// Excludes expr_t linked by ref in EXPR_VAR
void free_expr(expr_t exp){
	free_expr_no_self(exp);
	free(exp);
}

// Evaluate value of expression by evaluating children and using other expressions stored in variables
double eval_expr(expr_t exp, double *args){
	if(!exp) return 0;
	
	double result;
	switch(exp->type){
		// Used during parsing
		// But won't occur as types of actual nodes
		case EXPR_PARENTH:
		case EXPR_COMMA:
		break;
		
		case EXPR_CONST: result = exp->constant;
		break;
		case EXPR_ARGS: result = args[exp->arg_ind];
		break;
		case EXPR_CACHED: result = *(exp->cache);
		break;
		
		case EXPR_FUNC1:
			result = eval_expr(exp->children, args);
			result = exp->func.one_arg(result);
		break;
		case EXPR_FUNC2:
			result = eval_expr(exp->children, args);
			result = exp->func.two_arg(result, eval_expr(exp->children->next, args));
		break;
		
		case EXPR_ADD:
			result = 0;
			for(expr_t c = exp->children; c; c = c->next){
				result += eval_expr(c, args);
			}
		break;
		case EXPR_MUL:
			result = 1;
			for(expr_t c = exp->children; c; c = c->next){
				result *= eval_expr(c, args);
			}
		break;
		case EXPR_POW:
			result = eval_expr(exp->children, args);
			result = pow(result, eval_expr(exp->children->next, args));
		break;
		
		case EXPR_VAR:
		case EXPR_FUNCN:
			result = 0;
			double new_args[exp->child_count];
			for(expr_t c = exp->children; c; c = c->next){
				new_args[(int)result] = eval_expr(c, args);
				result++;
			}
			
			if(exp->type == EXPR_VAR){
				result = eval_expr(exp->ref, new_args);
			}else{
				result = exp->func.n_arg(new_args);
			}
		break;
	}
	
	if(exp->add_inv) result = -result;
	if(exp->mul_inv) result = 1 / result;
	return result;
}



expr_t constify_expr(expr_t exp){
	// Constants (e.g. 1, 2, 3.14) are already constant
	// Arguments (e.g. x, y, z) cannot be made constant
	if(exp->type == EXPR_CONST || exp->type == EXPR_ARGS || exp->type == EXPR_CACHED) return exp;
	
	bool is_const = 1;
	for(expr_t child = exp->children; child; child = child->next){
		// Check if child can be made constant
		if(constify_expr(child)->type != EXPR_CONST){
			is_const = 0;
		}
	}
	
	if(is_const){
		// Evaluate expression to obtain new constant value
		double evaled = eval_expr(exp, NULL);
		
		// Deallocate memory used for children nodes
		free_expr_no_self(exp);
		
		// Assign constant value and change type
		exp->constant = evaled;
		exp->type = EXPR_CONST;
	}
	return exp;
}

// Check if exp has the same type and relevant parameters as target
bool expr_match(expr_t exp, expr_t target){
	// Children are not consider in determining a match only the type and relevant parameters
	if(exp->type == target->type){
		switch(exp->type){
			case EXPR_CONST: return exp->constant == target->constant;
			case EXPR_ARGS: return exp->arg_ind == target->arg_ind;
			case EXPR_CACHED: return exp->cache == target->cache;
			
			// Check if exp and target refer to the same variable
			case EXPR_VAR: return exp->ref == target->ref;
			
			// Check if exp and target have the same function pointers
			case EXPR_FUNC1: return exp->func.one_arg == target->func.one_arg;
			case EXPR_FUNC2: return exp->func.two_arg == target->func.two_arg;
			case EXPR_FUNCN: return exp->func.n_arg == target->func.n_arg;
			
			case EXPR_ADD:
			case EXPR_MUL:
			case EXPR_POW:
			return 1;
			
			// Used during parsing
			// But won't occur as types of actual nodes
			case EXPR_PARENTH:
			case EXPR_COMMA:
			break;
		}
	}
	
	// If type doesn't match exprs don't match
	return 0;
}

// Check if exp contains any expression with the same type and relevant parameters as target
bool expr_depends(expr_t exp, expr_t target){
	if(expr_match(exp, target)){
		// Check if exp and target match
		return 1;
	}else if(exp->type == EXPR_CONST || exp->type == EXPR_ARGS || exp->type == EXPR_CACHED){
		// For any leaf nodes if type doesn't match 
		return 0;
	}
	
	// Check children for match
	for(expr_t c = exp->children; c; c = c->next){
		if(expr_depends(c, target)) return 1;
	}
	
	return 0;
}




// Constructors for certain expressions
expr_t const_expr(expr_t exp, double c){
	free_expr_no_self(exp);
	
	exp->type = EXPR_CONST;
	exp->next = NULL;
	exp->add_inv = 0;
	exp->mul_inv = 0;
	
	exp->constant = c;
	return exp;
}

expr_t arg_expr(expr_t exp, int arg_ind){
	free_expr_no_self(exp);
	
	exp->type = EXPR_ARGS;
	exp->next = NULL;
	exp->add_inv = 0;
	exp->mul_inv = 0;
	
	exp->arg_ind = arg_ind;
	return exp;
}

expr_t cached_expr(expr_t exp, double *cache){
	free_expr_no_self(exp);
	
	exp->type = EXPR_CACHED;
	exp->next = NULL;
	exp->add_inv = 0;
	exp->mul_inv = 0;
	
	exp->cache = cache;
	return exp;
}

// Apply other expressions or functions
expr_t apply_expr(expr_t exp, expr_t var, int argc, expr_t *args){
	free_expr_no_self(exp);
	
	exp->type = EXPR_VAR;
	exp->next = NULL;
	exp->add_inv = 0;
	exp->mul_inv = 0;
	
	exp->ref = var;
	exp->child_count = argc;
	if(argc > 0 && args){
		exp->children = args[0];
		for(int i = 1; i < argc; i++){
			args[i - 1]->next = args[i];
		}
		args[argc - 1]->next = NULL;
	}else{
		exp->children = NULL;
	}
	
	return exp;
}

expr_t apply_func(expr_t exp, union expr_func_u fn, bool use_n_arg, int argc, expr_t* args){
	exp->next = NULL;
	exp->add_inv = 0;
	exp->mul_inv = 0;
	
	if((argc == 1 || argc == 2) && !use_n_arg){
		exp->type = argc == 1 ? EXPR_FUNC1 : EXPR_FUNC2;
	}else{
		exp->type = EXPR_FUNCN;
	}
	
	exp->func = fn;
	exp->child_count = argc;
	if(argc > 0 && args){
		exp->children = args[0];
		for(int i = 1; i < argc; i++){
			args[i - 1]->next = args[i];
		}
		args[argc - 1]->next = NULL;
	}else{
		exp->children = NULL;
	}
	
	return exp;
}



// Operations on expressions
expr_t add_expr(expr_t res, expr_t a, expr_t b, bool do_inv){
	res->type = EXPR_ADD;
	res->children = a;
	a->next = b;
	
	b->add_inv = b->add_inv ^ do_inv;
	
	return res;
}

expr_t negate_expr(expr_t exp){
	exp->add_inv = !(exp->add_inv);
	return exp;
}

expr_t mul_expr(expr_t res, expr_t a, expr_t b, bool do_inv){
	res->type = EXPR_MUL;
	res->children = a;
	a->next = b;
	
	b->mul_inv = b->mul_inv ^ do_inv;
	
	return res;
}

expr_t pow_expr(expr_t res, expr_t a, expr_t b){
	res->type = EXPR_POW;
	res->children = a;
	a->next = b;
	return res;
}






// Redefinition and Reimplementation to avoid dependence on novel library functions
size_t expr_strnlen(const char *s, size_t max){
	size_t n;
	for(n = 0; n < max && *s; s++, n++);
	return n;
}





// Allow for conversion from enum to string when printing error
const char *parse_errstr[] = {
	"ERR_OK",
	"ERR_UNUSED_CHARACTER", "ERR_UNRECOGNIZED_NAME",
	"ERR_MISSING_VALUE", "ERR_EMPTY_EXPRESSION", "ERR_TOO_MANY_VALUES",
	"ERR_BAD_ARITY",
	"ERR_PARENTH_MISMATCH",
	"ERR_PARSE_OVERFLOW", "ERR_BAD_EXPRESSION"
};


enum token_type{
	NAME, NUMBER,
	OPEN_PARENTH, CLOSE_PARENTH,
	OPERATOR, COMMA, END
};

struct token_s{
	enum token_type type;
	
	union{
		double value;
		
		struct{
			const char *name;
			size_t length;
		};
	};
};

static struct token_s lex_expr(const char **str, parse_err_t *err){
	struct token_s tok;
	*err = ERR_OK;
	
	
	// Trim whitespace
	while(isspace(**str)){
		(*str)++;
	}
	
	// Indicate end of token sequence
	if(**str == '\0'){
		tok.type = END;
		return tok;
	}
	
	// Variable Token
	if(isalpha(**str)){
		tok.type = NAME;
		tok.name = *str;
		do{
			(*str)++;
		}while(isalnum(**str));
		
		tok.length = (size_t)(*str - tok.name);
		return tok;
	}
	
	// Numbers, Parentheses, Operators, & Commas
	char *endptr;
	switch(**str){
		// Parentheses
		case '(': tok.type = OPEN_PARENTH;
		break;
		case ')': tok.type = CLOSE_PARENTH;
		break;
		
		// Operators
		case '+':
		case '-':
		case '*':
		case '/':
		case '^':
			tok.type = OPERATOR;
		break;
		
		// Commas
		case ',':
			tok.type = COMMA;
		break;
		
		default:
			// Number Token
			tok.value = strtod(*str, &endptr);
			if(endptr != *str){
				tok.type = NUMBER;
				*str = endptr;
				return tok;
			}
			
			// Error
			*err = ERR_UNUSED_CHARACTER;
		return tok;
	}
	
	// Store character identifying operator
	tok.name = *str;
	tok.length = 1;
	
	// Move *str pointer past Parenthesis, Operator, or Comma
	(*str)++;
	return tok;
}



#define PARSE_STACK_SIZE 256

typedef struct{
	struct expr_s *head, *tail, *ptr;
} stack_t;

static struct expr_s pop_null(stack_t *s){
	struct expr_s exp = {0};
	// Check if stack is empty
	if(!(s->ptr)) return exp;
	
	exp = *(s->ptr);
	if(s->head == s->ptr){
	// If pointer at head set to NULL
		s->ptr = NULL;
	}else{
	// Otherwise move down
		(s->ptr)--;
	}
	return exp;
}

// Pop off last element as long as it isn't a null expression
static struct expr_s pop(stack_t *s){
	// Check if ptr is at a null expression, a block
	if(s->ptr->type == EXPR_PARENTH) return *(s->ptr);
	return pop_null(s);
}

#define peek_null(s) ((s)->ptr)

static struct expr_s *peek(stack_t *s){
	if(!(s->ptr)) return NULL;
	return s->ptr->type == EXPR_PARENTH ? NULL : s->ptr;
}

static struct expr_s *push(stack_t *s, struct expr_s exp){
	// Check if space available
	if(s->ptr == s->tail) return NULL;
	
	if(!(s->ptr)){
	// If no values stored put ptr at head
		s->ptr = s->head;
	}else{
	// Otherwise move up
		(s->ptr)++;
	}
	
	*(s->ptr) = exp;
	return s->ptr;
}

// Applies operator node `op` to the values on the value stack
// Pops elements off the value stack and combines them according to the operator
static parse_err_t apply_op(stack_t *s, struct expr_s op){
	struct expr_s tmp, tmp2;
	expr_t tmp_p;
	int cnt;
	switch(op.type){
		case EXPR_VAR:
		case EXPR_FUNC1:
		case EXPR_FUNC2:
		case EXPR_FUNCN:
			cnt = 0; // Count number of elements in list of arguments
			for(tmp_p = peek(s); tmp_p; tmp_p = tmp_p->next){
				cnt++;
			}
			
			// Check if there were any arguments
			if(cnt == 0){
				return ERR_BAD_EXPRESSION;
			// Check if arity of function matches number of arguments
			}else if(cnt != op.child_count){
				return ERR_BAD_ARITY;
			}
			
			op.children = malloc(sizeof(struct expr_s));
			*(op.children) = pop(s);
			
			push(s, op);
		break;
		
		// Accumulate terms for function call
		// Tries to add top element from stack to second to top element
		case EXPR_COMMA:
			if(!peek(s)) return ERR_MISSING_VALUE;
			tmp2 = pop(s);
			
			tmp_p = peek(s);
			if(!tmp_p) return ERR_MISSING_VALUE;
			
			// Seek to end of siblings list for first argument
			for(; tmp_p->next; tmp_p = tmp_p->next){}
			// Append tmp2's list to tmp's
			tmp_p->next = malloc(sizeof(struct expr_s));
			*(tmp_p->next) = tmp2;
		break;
		
		// Add or Multiply top two elements
		case EXPR_ADD:
		case EXPR_MUL:
			if(!peek(s)) return ERR_MISSING_VALUE;
			tmp2 = pop(s);
			
			if(!peek(s)){
				if(op.type == EXPR_ADD && op.add_inv){
					// If only one argument is present and operator is '-' assume unary usage
					tmp2.add_inv = 1;				
					push(s, tmp2);
					break;
				}else{
					push(s, tmp2);
					return ERR_MISSING_VALUE;
				}
			}
			
			tmp = pop(s);
			
			// Count number of arguments to sum
			cnt = 0;
			// Include the first element
			cnt++;
			if(tmp.type == op.type){
				if(!(tmp.children)) return ERR_BAD_EXPRESSION;
				
				// Include children of tmp in sum / product
				op.children = tmp.children;
				
				// Move tmp_p to end of children of tmp
				// Use tmp_p to store tail of op's children
				for(tmp_p = tmp.children; tmp_p->next; tmp_p = tmp_p->next){
					cnt++;
				}
			}else{
				// Create space on heap for tmp and make first child
				op.children = malloc(sizeof(struct expr_s));
				*(op.children) = tmp;
				
				// Use tmp to store tail of op's children
				tmp_p = op.children;
			}
			
			// Include the first element
			if(tmp2.type == op.type){
				if(!(tmp2.children)) return ERR_BAD_EXPRESSION;
				
				// Include children of tmp2 in sum / product
				tmp_p->next = tmp2.children;
				
				for(tmp_p = tmp2.children; tmp_p; tmp_p = tmp_p->next){
					// Invert tmp2's children if operator is '-' or '/'
					if(op.type == EXPR_ADD)
						tmp_p->add_inv = tmp_p->add_inv ^ op.add_inv;
					else
						tmp_p->mul_inv = tmp_p->mul_inv ^ op.mul_inv;
					
					// Count arguments from tmp2
					cnt++;
				}
			}else{
				cnt++;
				
				// Invert tmp2 if operator is '-' or '/'
				if(op.type == EXPR_ADD)
					tmp2.add_inv = tmp2.add_inv ^ op.add_inv;
				else
					tmp2.mul_inv = tmp2.mul_inv ^ op.mul_inv;
				
				// Create space on heap for tmp2 and store at tail of sum
				tmp_p->next = malloc(sizeof(struct expr_s));
				*(tmp_p->next) = tmp2;
			}
			
			op.add_inv = 0;
			op.mul_inv = 0;
			op.child_count = cnt;
			push(s, op);
		break;
		case EXPR_POW:
			if(!peek(s)) return ERR_MISSING_VALUE;
			tmp2 = pop(s);
			
			if(!peek(s)){
				push(s, tmp2);
				return ERR_MISSING_VALUE;
			}
			tmp = pop(s);
			
			// Allocate space on heap for the base and exponent
			op.children = malloc(sizeof(struct expr_s));
			*(op.children) = tmp;
			op.children->next = malloc(sizeof(struct expr_s));
			*(op.children->next) = tmp2;
			
			op.add_inv = 0;
			op.mul_inv = 0;
			op.child_count = 2;
			push(s, op);
		break;
		
		// Following types can never occur as the type of an operator
		case EXPR_CONST:
		case EXPR_ARGS:
		case EXPR_CACHED:
		case EXPR_PARENTH:
		break;
	}
	
	return ERR_OK;
}



static int get_prec(expr_t op){
	// If end of operators return block
	if(!op) return -1;
	
	switch(op->type){
		case EXPR_COMMA: return 1;
		case EXPR_ADD: return 2;
		case EXPR_MUL: return 3;
		case EXPR_POW: return 4;
		default: return -1; // Negative precedence used to indicate block like parentheses or function calls
	}
}

static bool left_associate(expr_t op){
	if(!op) return 0;
	
	switch(op->type){
		// 'a - b - c' => '(a - b) - c'
		case EXPR_ADD:
		case EXPR_MUL: return 1;
		// 'a ^ b ^ c' => 'a ^ (b ^ c)'
		case EXPR_POW:
		default: return 0;
	}
}

expr_t parse_expr(const char *src, name_trans_f callback, const char **endptr, parse_err_t *err, void *trans_inp){
	// Use endptr to track parse location of string if non-null
	if(endptr) *endptr = src;
	else endptr = &src;
	
	if(err) *err = ERR_OK;
	else{
		parse_err_t err_s = ERR_OK;
		err = &err_s;
	}
	
	// Initialize value and operator stack
	stack_t vals, ops;
	struct expr_s val_stack[PARSE_STACK_SIZE], op_stack[PARSE_STACK_SIZE];
	vals.head = val_stack;
	vals.tail = val_stack + PARSE_STACK_SIZE;
	vals.ptr = NULL;
	ops.head = op_stack;
	ops.tail = op_stack + PARSE_STACK_SIZE;
	ops.ptr = NULL;
	
	struct expr_s tmp;
	int prec1, prec2;
	for(struct token_s tok = lex_expr(endptr, err);
		tok.type != END && *err == ERR_OK;
		tok = lex_expr(endptr, err)
	){
		// Ensure tmp is fully cleared
		tmp.type = EXPR_CONST;
		tmp.next = NULL;
		tmp.add_inv = 0;
		tmp.mul_inv = 0;
		tmp.constant = 0;
		tmp.ref = NULL;
		tmp.child_count = 0;
		tmp.children = NULL;
		
		switch(tok.type){
			case NAME:
				// Set type to something that is not used
				// Represents unfound name
				tmp.type = EXPR_PARENTH;
				
				// Check builtin functions
				for(int i = 0; expr_builtin_funcs[i].name[0] != '\0'; i++){
					if(expr_strnlen(expr_builtin_funcs[i].name, EXPR_FUNCNAME_LEN) == tok.length
					&& strncasecmp(tok.name, expr_builtin_funcs[i].name, tok.length) == 0
					){
						tmp.child_count = expr_builtin_funcs[i].arity;
							
						// Assign type to function 
						if(tmp.child_count == 0){
							tmp.type = EXPR_CONST;
						}else if((tmp.child_count == 1 || tmp.child_count == 2)
						&& !(expr_builtin_funcs[i].use_n_arg)
						){
							tmp.type = tmp.child_count == 1 ? EXPR_FUNC1 : EXPR_FUNC2;
						}else{
							tmp.type = EXPR_FUNCN;
						}
						
						if(tmp.child_count > 0){
							// If function with arguments provide pointer
							tmp.func = expr_builtin_funcs[i].func;
							tmp.children = NULL;
						}else{
							// If constant set constant
							tmp.constant = expr_builtin_funcs[i].value;
						}
						break;
					}
				}
				
				// Check name translation if no builtin function matches
				if(tmp.type == EXPR_PARENTH){
					if(!callback(&tmp, tok.name, tok.length, trans_inp)){
						*err = ERR_UNRECOGNIZED_NAME;
						break;
					}
				}
				
				// Place name into appropriate stack
				if(tmp.type == EXPR_CONST
				|| tmp.type == EXPR_ARGS
				|| tmp.type == EXPR_CACHED
				|| ((tmp.type == EXPR_VAR || tmp.type == EXPR_FUNCN)
				   && tmp.child_count == 0)
				){
					if(!push(&vals, tmp)) *err = ERR_PARSE_OVERFLOW;
				}else{
					if(!push(&ops, tmp)) *err = ERR_PARSE_OVERFLOW;
				}
			break;
			case NUMBER:
				tmp.type = EXPR_CONST;
				tmp.constant = tok.value;
				
				if(!push(&vals, tmp)){
					*err = ERR_PARSE_OVERFLOW;
					break;
				}
			break;
			
			case OPEN_PARENTH:
				tmp.type = EXPR_PARENTH;
				
				// Place null expression as block on both stacks
				if(!push(&vals, tmp) || !push(&ops, tmp)){
					*err = ERR_PARSE_OVERFLOW;
					break;
				}
			break;
			case CLOSE_PARENTH:
				// Apply all operators until last open parenthesis
				for(prec2 = get_prec(peek(&ops)); prec2 >= 0; prec2 = get_prec(peek(&ops))){
					*err = apply_op(&vals, pop(&ops));
					if(*err != ERR_OK) break;
				}
				if(*err != ERR_OK) break;
				
				// Check if open parenthesis present
				if(peek_null(&ops)->type != EXPR_PARENTH){
					*err = ERR_PARENTH_MISMATCH;
					break;
				}
				pop_null(&ops); // Remove open parenthesis block
				
				// Esnure that there is a value on vals stack
				if(peek_null(&vals)->type == EXPR_PARENTH){
					*err = ERR_EMPTY_EXPRESSION;
					break;
				}
				
				tmp = pop(&vals);
				
				// Ensure that there is not more than one value before open parenthesis
				if(peek_null(&vals)->type != EXPR_PARENTH){
					*err = ERR_TOO_MANY_VALUES;
					break;
				}
				pop_null(&vals);
				
				// Put value of parenthetical phrase back on stack
				push(&vals, tmp);
				
				// If parenthetical phrase contained the arguments for a function then apply the function
				if(peek(&ops) &&
				  (peek(&ops)->type == EXPR_VAR
				|| peek(&ops)->type == EXPR_FUNC1
				|| peek(&ops)->type == EXPR_FUNC2
				|| peek(&ops)->type == EXPR_FUNCN
				)){
					*err = apply_op(&vals, pop(&ops));
				}
			break;
			
			case OPERATOR:
			case COMMA:
				tmp.child_count = 0;
				tmp.children = NULL;
				switch(*(tok.name)){
					case '-': tmp.add_inv = 1;
					case '+': tmp.type = EXPR_ADD;
					break;
					case '/': tmp.mul_inv = 1;
					case '*': tmp.type = EXPR_MUL;
					break;
					case '^': tmp.type = EXPR_POW;
					break;
					case ',': tmp.type = EXPR_COMMA;
					break;
				}
				
				// Apply operators of higher (or equal) precedence from ops stack onto vals before pushing tmp to the ops stack
				// Equal precedence operators only removed if left_associative
				prec1 = get_prec(&tmp);
				prec2 = get_prec(peek(&ops));
				while( prec2 >= 0
				&&   ( prec2 > prec1
				     ||  (prec2 == prec1 && left_associate(peek(&ops)))
				     )
				){
					*err = apply_op(&vals, pop(&ops));
					if(*err != ERR_OK) break;
					
					prec2 = get_prec(peek(&ops));
				}
				if(*err != ERR_OK) break;
				
				if(!push(&ops, tmp)){
					*err = ERR_PARSE_OVERFLOW;
				}
			break;
			
			// Token type filtered out by surrounding for loop
			case END:
			break;
		}
		
		// Break if error occurs during switch
		if(*err != ERR_OK){
			break;
		}
	}
	
	expr_t s;
	
	if(*err == ERR_OK){
		// Apply all remaining operators on ops stack
		for(prec2 = get_prec(peek(&ops)); prec2 >= 0; prec2 = get_prec(peek(&ops))){
			*err = apply_op(&vals, pop(&ops));
			if(*err != ERR_OK) break;
		}
		
		// Check if there is a remaining open parenth
		if(*err == ERR_OK && peek_null(&ops)) *err = ERR_PARENTH_MISMATCH;
	}
	
	// Clean up ops stack
	if(ops.ptr){
		for(s = ops.head; s <= ops.ptr; s++) free_expr_no_self(s);
	}
	
	// Check that there is exactly one value left on the vals stack
	if(*err == ERR_OK){
		if(!peek_null(&vals)) *err = ERR_EMPTY_EXPRESSION;
		else if(peek_null(&vals)->type == EXPR_PARENTH) *err = ERR_PARENTH_MISMATCH;
		else tmp = pop(&vals);
		
		if(*err == ERR_OK && peek_null(&vals)){
			push(&vals, tmp);
			*err = ERR_TOO_MANY_VALUES;
		}
	}
	
	// Clean up vals stack
	// If error occurred this will clean up any loose ends
	if(vals.ptr){
		for(s = vals.head; s <= vals.ptr; s++) free_expr_no_self(s);
	}
	
	if(*err == ERR_OK){
		// Allocate heap memory for tmp
		s = malloc(sizeof(struct expr_s));
		*s = tmp;
		return s;
	}else{
		// Return nothing if error occurred
		return NULL;
	}
}

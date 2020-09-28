#include <stdio.h>
#include <string.h>

#include "mpc.h"

enum { LVAL_LONG, LVAL_ERR, LVAL_DOUBLE };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM, LERR_BAD_TYPE };

typedef struct {
  int type;
  union {
    long l;
    double d;
    int err;
  } value;
} lval;

lval lval_long(long x) {
  lval v;
  v.type = LVAL_LONG;
  v.value.l = x;
  return v;
}

lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.value.err = x;
  return v;
}

lval lval_double(double x) {
  lval v;
  v.type = LVAL_DOUBLE;
  v.value.d = x;
  return v;
}

void lval_print(lval v) {
  switch (v.type) {
    case LVAL_LONG: printf("%li", v.value.l); break;
    case LVAL_ERR:
      if (v.value.err == LERR_DIV_ZERO) {
        printf("Error: Division By Zero!");
      }
      if (v.value.err == LERR_BAD_OP)   {
        printf("Error: Invalid Operator!");
      }
      if (v.value.err == LERR_BAD_NUM)  {
        printf("Error: Invalid Number!");
      }
      if (v.value.err == LERR_BAD_TYPE) {
	printf("Error: Mismatched TYpes!");
      }
    break;
  case LVAL_DOUBLE: printf("%f", v.value.d); break;
  }
}

void lval_println(lval v) { lval_print(v); putchar('\n'); }

lval eval_op2(lval x, char* op, lval y) {
  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }

  if (x.type != y.type)
    return lval_err(LERR_BAD_TYPE);
  if (x.type == LVAL_LONG) {
    if (strcmp(op, "+") == 0 || strcmp(op, "add") == 0) { return lval_long(x.value.l + y.value.l); }
    if (strcmp(op, "-") == 0 || strcmp(op, "sub") == 0) { return lval_long(x.value.l - y.value.l); }
    if (strcmp(op, "*") == 0 || strcmp(op, "mul") == 0) { return lval_long(x.value.l * y.value.l); }
    if (strcmp(op, "/") == 0 || strcmp(op, "div") == 0) {
      return y.value.l == 0
	? lval_err(LERR_DIV_ZERO)
	: lval_long(x.value.l / y.value.l);
    }
    if (strcmp(op, "%") == 0 || strcmp(op, "mod") == 0) {
      return y.value.l == 0
	? lval_err(LERR_DIV_ZERO)
	: lval_long(x.value.l % y.value.l);
    }
    if (strcmp(op, "^") == 0 || strcmp(op, "pow") == 0) { return lval_long(pow(x.value.l, y.value.l)); }
  }
  if (x.type == LVAL_DOUBLE) {
    if (strcmp(op, "+") == 0 || strcmp(op, "add") == 0) { return lval_double(x.value.d + y.value.d); }
    if (strcmp(op, "-") == 0 || strcmp(op, "sub") == 0) { return lval_double(x.value.d - y.value.d); }
    if (strcmp(op, "*") == 0 || strcmp(op, "mul") == 0) { return lval_double(x.value.d * y.value.d); }
    if (strcmp(op, "/") == 0 || strcmp(op, "div") == 0) {
      return y.value.d == 0.0
	? lval_err(LERR_DIV_ZERO)
	: lval_double(x.value.d / y.value.d);
    }
    if (strcmp(op, "%") == 0 || strcmp(op, "mod") == 0) {
      return y.value.d == 0.0
	? lval_err(LERR_DIV_ZERO)
	: lval_double(fmod(x.value.d, y.value.d));
    }
    if (strcmp(op, "^") == 0 || strcmp(op, "pow") == 0) { return lval_double(pow(x.value.d, y.value.d)); }
  }
  return lval_err(LERR_BAD_OP);
}

lval eval_op1(lval x, char* op) {
  if (x.type == LVAL_LONG)
    if (strcmp(op, "-") == 0 || strcmp(op, "neg") == 0) { return lval_long(-x.value.l); }
  if (x.type == LVAL_DOUBLE)
    if (strcmp(op, "-") == 0 || strcmp(op, "neg") == 0) { return lval_double(-x.value.d); }
  return lval_err(LERR_BAD_OP);
} 

lval eval(mpc_ast_t* t) {

  if (strstr(t->tag, "long")) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_long(x) : lval_err(LERR_BAD_NUM);
  }
  if (strstr(t->tag, "double")) {
    errno = 0;
    double x = strtod(t->contents, NULL);
    return errno != ERANGE ? lval_double(x) : lval_err(LERR_BAD_NUM);
  }
  
  char* op = t->children[1]->contents;
  lval x = eval(t->children[2]);

  int i = 3;
  if (!strstr(t->children[i]->tag, "expr"))
    x = eval_op1(x, op);
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op2(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

int main(int argc, char **argv) {
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Double   = mpc_new("double");
  mpc_parser_t* Long     = mpc_new("long");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lisp64   = mpc_new("lisp64");
  
  mpca_lang(MPCA_LANG_DEFAULT,
	    "                                                     \
      double   : /-?[0-9]+\\.[0-9]+/ ;				\
      long     : /-?[0-9]+/ ;						\
      number   : <double> | <long> ;					\
      operator : '+' | '-' | '*' | '/' | '%' | '^' | \"add\" | \"sub\" | \"mul\" | \"div\" | \"mod\" | \"pow\" | \"neg\" ; \
      expr     : <number> | '(' <operator> <expr>+ ')' ;	  \
      lisp64   : /^/ <operator> <expr>+ /$/ ;			  \
    ",
	    Long, Double, Number, Operator, Expr, Lisp64);

  FILE *f = fopen("test.lisp", "r");
  if (f == NULL) {
    printf("file failure\n");
    return 1;
  }
  
  mpc_result_t r;
  if (mpc_parse_file("test.lisp", f, Lisp64, &r)) { 
    lval result = eval(r.output);
    lval_println(result);
    mpc_ast_delete(r.output);
  } else {
    mpc_err_print(r.error);
    mpc_err_delete(r.error);
  }

  fclose(f);
  mpc_cleanup(6, Long, Double,  Number, Operator, Expr, Lisp64);
  return 0;
}

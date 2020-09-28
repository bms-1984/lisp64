/*
    Lisp64 is a custom Lisp implementation.
    Copyright (C) 2020 Ben M. Sutter

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>

#include "mpc.h"

enum { LVAL_LONG, LVAL_ERR, LVAL_DOUBLE, LVAL_SYM, LVAL_SEXP };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM, LERR_BAD_TYPE };

typedef struct lval {
  int type;
  int count;
  union {
    long l;
    double d;
    char *err;
    char *sym;
    struct lval **cell;
  } value;
} lval;

void lval_print(lval *v);
lval *lval_eval(lval *v);
lval *lval_eval_sexp(lval *v);
lval *builtin_op(lval *a, char *op);

lval *lval_long(long x) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_LONG;
  v->value.l = x;
  return v;
}

lval *lval_err(char *x) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->value.err = malloc(strlen(x) + 1);
  strcpy(v->value.err, x);
  return v;
}

lval *lval_double(double x) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_DOUBLE;
  v->value.d = x;
  return v;
}

lval *lval_sym(char *x) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->value.sym = malloc(strlen(x) + 1);
  strcpy(v->value.sym, x);
  return v;
}

lval *lval_sexp(void) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_SEXP;
  v->count = 0;
  v->value.cell = NULL;
  return v;
}

void lval_del(lval *v) {
  switch (v->type) {
  case LVAL_LONG: break;
  case LVAL_DOUBLE: break;
  case LVAL_ERR: free(v->value.err); break;
  case LVAL_SYM: free(v->value.sym); break;
  case LVAL_SEXP:
    for (int i = 0; i < v->count; i++) {
      lval_del(v->value.cell[i]);
    }
    free(v->value.cell);
    break;
  }
  free(v);
}

void lval_expr_print(lval *v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {
    lval_print(v->value.cell[i]);
    if (i != (v->count-1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print(lval *v) {
  switch (v->type) {
  case LVAL_LONG:   printf("%li", v->value.l); break;
  case LVAL_DOUBLE: printf("%f", v->value.d); break;
  case LVAL_ERR:   printf("Error: %s", v->value.err); break;
  case LVAL_SYM:   printf("%s", v->value.sym); break;
  case LVAL_SEXP: lval_expr_print(v, '(', ')'); break;
  }
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval *lval_add(lval *v, lval *x) {
  v->count++;
  v->value.cell = realloc(v->value.cell, sizeof(lval*) * v->count);
  v->value.cell[v->count-1] = x;
  return v;
}

lval *lval_read_long(mpc_ast_t *t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ?
    lval_long(x) : lval_err("invalid number");
}

lval *lval_read_double(mpc_ast_t *t) {
  errno = 0;
  double x = strtod(t->contents, NULL);
  return errno != ERANGE ?
    lval_double(x) : lval_err("invalid number");
}

lval *lval_read(mpc_ast_t *t) {
  if (strstr(t->tag, "double")) { return lval_read_double(t); }
  if (strstr(t->tag, "long")) { return lval_read_long(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }
  lval *x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexp(); }
  if (strstr(t->tag, "sexp"))  { x = lval_sexp(); }
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }
  return x;
}

lval *lval_pop(lval *v, int i) {
  lval *x = v->value.cell[i];
  memmove(&v->value.cell[i], &v->value.cell[i+1],
    sizeof(lval*) * (v->count-i-1));
  v->count--;
  v->value.cell = realloc(v->value.cell, sizeof(lval*) * v->count);
  return x;
}

lval *lval_take(lval *v, int i) {
  lval *x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval *lval_eval_sexp(lval *v) {
  for (int i = 0; i < v->count; i++) {
    v->value.cell[i] = lval_eval(v->value.cell[i]);
  }
  for (int i = 0; i < v->count; i++) {
    if (v->value.cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
  }
  if (v->count == 0) { return v; }
  if (v->count == 1) { return lval_take(v, 0); }
  lval *f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f); lval_del(v);
    return lval_err("S-expression Does not start with symbol!");
  }
  lval* result = builtin_op(v, f->value.sym);
  lval_del(f);
  return result;
}

lval *lval_eval(lval *v) {
   if (v->type == LVAL_SEXP) { return lval_eval_sexp(v); }
   return v;
}

lval *builtin_op(lval *a, char *op) {
  for (int i = 0; i < a->count; i++) {
    if (!(a->value.cell[i]->type == LVAL_LONG || a->value.cell[i]->type == LVAL_DOUBLE)) {
      lval_del(a);
      return lval_err("Cannot operate on non-number!");
    }
  }
  lval *x = lval_pop(a, 0);
  if (x->type == LVAL_LONG) {
    if ((strcmp(op, "-") == 0 || strcmp(op, "neg") == 0) && a->count == 0) {
      x->value.l = -x->value.l;
    }
    while (a->count > 0) {
      lval* y = lval_pop(a, 0);
      if (strcmp(op, "+") == 0 || strcmp(op, "add") == 0) { x->value.l += y->value.l; }
      if (strcmp(op, "-") == 0 || strcmp(op, "sub") == 0) { x->value.l -= y->value.l; }
      if (strcmp(op, "*") == 0 || strcmp(op, "mul") == 0) { x->value.l *= y->value.l; }
      if (strcmp(op, "/") == 0 || strcmp(op, "div") == 0) {
	if (y->value.l == 0) {
	  lval_del(x);
	  lval_del(y);
	  x = lval_err("Division By Zero!"); break;
	}
	x->value.l /= y->value.l;
      }
      if (strcmp(op, "%") == 0 || strcmp(op, "mod") == 0 || strcmp(op, "rem") == 0) {
	if (y->value.l == 0) {
	  lval_del(x);
	  lval_del(y);
	  x = lval_err("Division By Zero!"); break;
	}
	x->value.l %= y->value.l;
      }
      if (strcmp(op, "^") == 0 || strcmp(op, "pow") == 0) { x->value.l = pow(x->value.l, y->value.l); }
      lval_del(y);
    }
  }
  if (x->type == LVAL_DOUBLE) {
    if ((strcmp(op, "-") == 0 || strcmp(op, "neg") == 0) && a->count == 0) {
      x->value.d = -x->value.d;
    }
    while (a->count > 0) {
      lval* y = lval_pop(a, 0);
      if (strcmp(op, "+") == 0 || strcmp(op, "add") == 0) { x->value.d += y->value.d; }
      if (strcmp(op, "-") == 0 || strcmp(op, "sub") == 0) { x->value.d -= y->value.d; }
      if (strcmp(op, "*") == 0 || strcmp(op, "mul") == 0) { x->value.d *= y->value.d; }
      if (strcmp(op, "/") == 0 || strcmp(op, "div") == 0) {
	if (y->value.d == 0) {
	  lval_del(x);
	  lval_del(y);
	  x = lval_err("Division By Zero!"); break;
	}
	x->value.d /= y->value.d;
      }
      if (strcmp(op, "%") == 0 || strcmp(op, "mod") == 0 || strcmp(op, "rem") == 0) {
	if (y->value.d == 0) {
	  lval_del(x);
	  lval_del(y);
	  x = lval_err("Division By Zero!"); break;
	}
	x->value.d = fmod(x->value.d, y->value.d);
      }
      if (strcmp(op, "^") == 0 || strcmp(op, "pow") == 0) { x->value.d = pow(x->value.d, y->value.d); }
      lval_del(y);
    }
  }

  lval_del(a);
  return x;
}

int main(int argc, char **argv) {
  mpc_parser_t *Number   = mpc_new("number");
  mpc_parser_t *Double   = mpc_new("double");
  mpc_parser_t *Long     = mpc_new("long");
  mpc_parser_t *Symbol   = mpc_new("symbol");
  mpc_parser_t *Expr     = mpc_new("expr");
  mpc_parser_t *Sexp     = mpc_new("sexp");
  mpc_parser_t *Lisp64   = mpc_new("lisp64");
  
  mpca_lang(MPCA_LANG_DEFAULT,
	    "                                                     \
      double   : /-?[0-9]+\\.[0-9]+/ ;					\
      long     : /-?[0-9]+/ ;						\
      number   : <double> | <long> ;					\
      symbol   : '+' | '-' | '*' | '/' | '%' | '^' | \"add\" | \"sub\" | \"mul\" | \"div\" | \"mod\" | \"pow\" | \"neg\" | \"rem\" ; \
      sexp     : '(' <expr>* ')' ;					\
      expr     : <number> | <symbol> | <sexp> ;				\
      lisp64   : /^/ <expr>* /$/ ;					\
    ",
	    Double, Long, Number, Symbol, Sexp, Expr, Lisp64);

  if (argc > 1) {
    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
      perror("file failure\n");
      return 1;
    }
    mpc_result_t r;
    if (mpc_parse_file("test.lisp", f, Lisp64, &r)) { 
      lval* x = lval_eval(lval_read(r.output));
      lval_println(x);
      lval_del(x);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    } 
    fclose(f);
  } else {
    char *buffer;
    size_t bufsize = 50;
    buffer = (char *)malloc(bufsize * sizeof(char));
    if(buffer == NULL)
      {
	perror("unable to allocate buffer");
	exit(1);
      }
    printf("lisp64 v0.1\n");
    printf("lisp64 Copyright (C) 2020 Ben M. Sutter\nThis program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.\nThis is free software, and you are welcome to redistribute it\nunder certain conditions; type `show c' for details.\n");
    while (1) {
      printf("> ");
      getline(&buffer, &bufsize, stdin);
      if (strstr(buffer, ";quit")) break;
      mpc_result_t r;
      if (mpc_parse("repl", buffer, Lisp64, &r)) { 
	lval* x = lval_eval(lval_read(r.output));
	lval_println(x);
	lval_del(x);
	mpc_ast_delete(r.output);
      } else {
	mpc_err_print(r.error);
	mpc_err_delete(r.error);
      }
    }
  }
  
  mpc_cleanup(7, Long, Double,  Number, Symbol, Expr, Sexp, Lisp64);
  return 0;
}

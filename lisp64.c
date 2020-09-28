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
#define LASSERT(args, cond, fmt, ...) \
  if (!(cond)) { lval* err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err; }

#define LASSERT_TYPE(func, args, index, expect) \
  LASSERT(args, args->value.cell[index]->type == expect, \
    "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
    func, index, ltype_name(args->value.cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num) \
  LASSERT(args, args->count == num, \
    "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
    func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
  LASSERT(args, args->value.cell[index]->count != 0, \
    "Function '%s' passed {} for argument %i.", func, index);


struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

enum { LVAL_LONG, LVAL_ERR, LVAL_DOUBLE, LVAL_SYM, LVAL_SEXP, LVAL_QEXP, LVAL_FUN};

typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval {
  int type;
  int count;
  union {
    long l;
    double d;
    char *err;
    char *sym;
    lval **cell;
    lbuiltin fun;
  } value;
};

struct lenv {
  int count;
  char **syms;
  lval **vals;
};

void lval_print(lval *v);
lval *lval_eval(lenv *e, lval *v);
lval *lval_eval_sexp(lenv *e, lval *v);
lval *builtin_op(lenv *e, lval *a, char *op);
void lval_del(lval *v);
lval *lval_copy(lval *v);
lval *lval_err(char *fmt, ...);

lenv *lenv_new(void) {
  lenv *e = malloc(sizeof(lenv));
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  return e;
}

void lenv_del(lenv *e) {
  for (int i = 0; i < e->count; i++) {
    free(e->syms[i]);
    lval_del(e->vals[i]);
  }
  free(e->syms);
  free(e->vals);
  free(e);
}

lval *lenv_get(lenv *e, lval *k) {
  for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], k->value.sym) == 0) {
      return lval_copy(e->vals[i]);
    }
  }
  return lval_err("Unbound Symbol '%s'", k->value.sym);
}

void lenv_put(lenv *e, lval *k, lval *v) {
   for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], k->value.sym) == 0) {
      lval_del(e->vals[i]);
      e->vals[i] = lval_copy(v);
      return;
    }
  }
  e->count++;
  e->vals = realloc(e->vals, sizeof(lval*) * e->count);
  e->syms = realloc(e->syms, sizeof(char*) * e->count);
  e->vals[e->count-1] = lval_copy(v);
  e->syms[e->count-1] = malloc(strlen(k->value.sym)+1);
  strcpy(e->syms[e->count-1], k->value.sym);
}

lval *lval_long(long x) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_LONG;
  v->value.l = x;
  return v;
}

lval *lval_err(char *fmt, ...) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  va_list va;
  va_start(va, fmt);
  v->value.err = malloc(512);
  vsnprintf(v->value.err, 511, fmt, va);
  v->value.err = realloc(v->value.err, strlen(v->value.err)+1);
  va_end(va);
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

lval *lval_qexp(void) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_QEXP;
  v->count = 0;
  v->value.cell = NULL;
  return v;
}

lval *lval_fun(lbuiltin x) {
  lval *v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->value.fun = x;
  return v;
}

void lval_del(lval *v) {
  switch (v->type) {
  case LVAL_FUN:
  case LVAL_LONG:
  case LVAL_DOUBLE: break;
  case LVAL_ERR: free(v->value.err); break;
  case LVAL_SYM: free(v->value.sym); break;
  case LVAL_QEXP:
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
  case LVAL_QEXP: lval_expr_print(v, '{', '}'); break; 
  case LVAL_FUN:   printf("<function>"); break;
  }
}

void lval_println(lval *v) { lval_print(v); putchar('\n'); }

lval *lval_copy(lval *v) {
  lval *x = malloc(sizeof(lval));
  x->type = v->type;

  switch (v->type) {
  case LVAL_FUN: x->value.fun = v->value.fun; break;
  case LVAL_LONG: x->value.l = v->value.l; break;
  case LVAL_DOUBLE: x->value.d = v->value.d; break;
  case LVAL_ERR:
    x->value.err = malloc(strlen(v->value.err) + 1);
    strcpy(x->value.err, v->value.err); break;
  case LVAL_SYM:
    x->value.sym = malloc(strlen(v->value.sym) + 1);
    strcpy(x->value.sym, v->value.sym); break;
  case LVAL_SEXP:
  case LVAL_QEXP:
    x->count = v->count;
    x->value.cell = malloc(sizeof(lval*) * x->count);
    for (int i = 0; i < x->count; i++) {
      x->value.cell[i] = lval_copy(v->value.cell[i]);
    }
    break;
  }
  
  return x;
}

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
  if (strstr(t->tag, "qexp"))  { x = lval_qexp(); }
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
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

char *ltype_name(int t) {
  switch(t) {
  case LVAL_FUN: return "Function";
  case LVAL_LONG: return "Long";
  case LVAL_DOUBLE: return "Double";
  case LVAL_ERR: return "Error";
  case LVAL_SYM: return "Symbol";
  case LVAL_SEXP: return "S-Expression";
  case LVAL_QEXP: return "Q-Expression";
  default: return "Unknown";
  }
}

lval *builtin_head(lenv *e, lval *a) {
  LASSERT_NUM("head", a, 1);
  LASSERT_TYPE("head", a, 0, LVAL_QEXP);
  LASSERT_NOT_EMPTY("head", a, 0);
  lval *v = lval_take(a, 0);
  while (v->count > 1) { lval_del(lval_pop(v, 1)); }
  return v;
}

lval *builtin_tail(lenv *e, lval *a) {
  LASSERT_NUM("tail", a, 1);
  LASSERT_TYPE("tail", a, 0, LVAL_QEXP);
  LASSERT_NOT_EMPTY("tail", a, 0);
  lval *v = lval_take(a, 0);
  lval_del(lval_pop(v, 0));
  return v;
}

lval *builtin_list(lenv *e, lval *a) {
  a->type = LVAL_QEXP;
  return a;
}

lval *builtin_eval(lenv *e, lval *a) {
  LASSERT_NUM("eval", a, 1);
  LASSERT_TYPE("eval", a, 0, LVAL_QEXP);
  lval *x = lval_take(a, 0);
  x->type = LVAL_SEXP;
  return lval_eval(e, x);
}

lval *lval_join(lval *x, lval *y) {
  while (y->count) {
    x = lval_add(x, lval_pop(y, 0));
  }
  lval_del(y);
  return x;
}

lval *builtin_join(lenv *e, lval *a) {
  for (int i = 0; i < a->count; i++) {
    LASSERT_TYPE("join", a, i, LVAL_QEXP);
  }
  lval *x = lval_pop(a, 0);
  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }
  lval_del(a);
  return x;
}

lval *builtin_define(lenv *e, lval *a) {
  LASSERT_TYPE("define", a, 0, LVAL_QEXP);
  lval *syms = a->value.cell[0];
  for (int i = 0; i < syms->count; i++) {
    LASSERT(a, syms->value.cell[i]->type == LVAL_SYM, "Function 'def' cannot define non-symbol");
  }
  LASSERT(a, syms->count == a->count-1, "Function 'def' cannot define incorrect " "number of values to symbols");
  for (int i = 0; i < syms->count; i++) {
    lenv_put(e, syms->value.cell[i], a->value.cell[i+1]);
  }
  lval_del(a);
  return lval_sexp();
}

lval *lval_eval_sexp(lenv *e, lval *v) {
  for (int i = 0; i < v->count; i++) {
    v->value.cell[i] = lval_eval(e, v->value.cell[i]);
  }
  for (int i = 0; i < v->count; i++) {
    if (v->value.cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
  }
  if (v->count == 0) { return v; }
  if (v->count == 1) { return lval_take(v, 0); }
  lval *f = lval_pop(v, 0);
  if (f->type != LVAL_FUN) {
    lval_del(v); lval_del(f);
    return lval_err("first element is not a function");
  }
  lval *result = f->value.fun(e, v);
  lval_del(f);
  return result;
}

lval *lval_eval(lenv *e, lval *v) {
  if (v->type == LVAL_SYM) {
    lval *x = lenv_get(e, v);
    lval_del(v);
    return x;
  }
  if (v->type == LVAL_SEXP) { return lval_eval_sexp(e, v); }
  return v;
}

lval *builtin_add(lenv *e, lval *a) {
  return builtin_op(e, a, "+");
}

lval *builtin_sub(lenv *e, lval *a) {
  return builtin_op(e, a, "-");
}

lval *builtin_mul(lenv *e, lval *a) {
  return builtin_op(e, a, "*");
}

lval *builtin_div(lenv *e, lval *a) {
  return builtin_op(e, a, "/");
}

lval *builtin_mod(lenv *e, lval *a) {
  return builtin_op(e, a, "%");
}

lval *builtin_pow(lenv *e, lval *a) {
  return builtin_op(e, a, "^");
}

void lenv_add_builtin(lenv *e, char *name, lbuiltin func) {
  lval *k = lval_sym(name);
  lval *v = lval_fun(func);
  lenv_put(e, k, v);
  lval_del(k); lval_del(v);
}

void lenv_add_builtins(lenv *e) {
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);
  lenv_add_builtin(e, "define", builtin_define);
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
  lenv_add_builtin(e, "%", builtin_mod);
  lenv_add_builtin(e, "^", builtin_pow);
}

lval *builtin_op(lenv *e, lval *a, char *op) {
  for (int i = 0; i < a->count; i++) {
    if (!(a->value.cell[i]->type == LVAL_LONG || a->value.cell[i]->type == LVAL_DOUBLE)) {
      lval_del(a);
      return lval_err("Cannot operate on non-number!");
    }
  }
  lval *x = lval_pop(a, 0);
  if (x->type == LVAL_LONG) {
    if (strcmp(op, "-") == 0 && a->count == 0) {
      x->value.l = -x->value.l;
    }
    while (a->count > 0) {
      lval* y = lval_pop(a, 0);
      if (strcmp(op, "+") == 0) { x->value.l += y->value.l; }
      if (strcmp(op, "-") == 0) { x->value.l -= y->value.l; }
      if (strcmp(op, "*") == 0) { x->value.l *= y->value.l; }
      if (strcmp(op, "/") == 0) {
	if (y->value.l == 0) {
	  lval_del(x);
	  lval_del(y);
	  x = lval_err("Division By Zero!"); break;
	}
	x->value.l /= y->value.l;
      }
      if (strcmp(op, "%") == 0) {
	if (y->value.l == 0) {
	  lval_del(x);
	  lval_del(y);
	  x = lval_err("Division By Zero!"); break;
	}
	x->value.l %= y->value.l;
      }
      if (strcmp(op, "^") == 0) { x->value.l = pow(x->value.l, y->value.l); }
      lval_del(y);
    }
  }
  if (x->type == LVAL_DOUBLE) {
    if (strcmp(op, "-") == 0 && a->count == 0) {
      x->value.d = -x->value.d;
    }
    while (a->count > 0) {
      lval* y = lval_pop(a, 0);
      if (strcmp(op, "+") == 0) { x->value.d += y->value.d; }
      if (strcmp(op, "-") == 0) { x->value.d -= y->value.d; }
      if (strcmp(op, "*") == 0) { x->value.d *= y->value.d; }
      if (strcmp(op, "/") == 0) {
	if (y->value.d == 0) {
	  lval_del(x);
	  lval_del(y);
	  x = lval_err("Division By Zero!"); break;
	}
	x->value.d /= y->value.d;
      }
      if (strcmp(op, "%") == 0) {
	if (y->value.d == 0) {
	  lval_del(x);
	  lval_del(y);
	  x = lval_err("Division By Zero!"); break;
	}
	x->value.d = fmod(x->value.d, y->value.d);
      }
      if (strcmp(op, "^") == 0) { x->value.d = pow(x->value.d, y->value.d); }
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
  mpc_parser_t *Qexp     = mpc_new("qexp");
  mpc_parser_t *Lisp64   = mpc_new("lisp64");
  
  mpca_lang(MPCA_LANG_DEFAULT,
	    "                                                     \
      double   : /-?[0-9]+\\.[0-9]+/ ;					\
      long     : /-?[0-9]+/ ;						\
      number   : <double> | <long> ;					\
      symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&\\^%]+/ ;			\
      sexp     : '(' <expr>* ')' ;					\
      qexp     : '{' <expr>* '}' ;					\
      expr     : <number> | <symbol> | <sexp> | <qexp> ;		\
      lisp64   : /^/ <expr>* /$/ ;					\
    ",
	    Double, Long, Number, Symbol, Sexp, Qexp, Expr, Lisp64);
  lenv* e = lenv_new();
  lenv_add_builtins(e);
  
  if (argc > 1) {
    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
      perror("file failure\n");
      return 1;
    }
    mpc_result_t r;
    if (mpc_parse_file("test.lisp", f, Lisp64, &r)) { 
      lval *x = lval_eval(e, lval_read(r.output));
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
	lval* x = lval_eval(e, lval_read(r.output));
	lval_println(x);
	lval_del(x);
	mpc_ast_delete(r.output);
      } else {
	mpc_err_print(r.error);
	mpc_err_delete(r.error);
      }
    }
  }
  
  mpc_cleanup(8, Long, Double, Number, Symbol, Expr, Sexp, Qexp, Lisp64);
  return 0;
}

/*
Main value object used throughout leesp
Includes functions to create and use lval instances
requires useage of an environment
*/

#include <stdio.h>
#include <stdlib.h>

#include "constructors.h"
#include "edit.h"
#include "print.h"

/* forward declarations */
void lenv_put(lenv* e, lval* k, lval* v);
lval* lenv_get(lenv* e, lval* k);

lval* builtin_eval(lenv* e, lval* a);
lval* builtin_list(lenv* e, lval* a);

char* ltype_name(int t) {
  switch(t) {
    case LVAL_ERR: return "Error";
    case LVAL_NUM: return "Number";
    case LVAL_SYM: return "Symbol";
    case LVAL_FUN: return "Function";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_QEXPR: return "Q-Expression";
    default: return "Unknown";
  }
}

lval* lval_read_num(mpc_ast_t* t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* t) {
  /* if symbol or number return the conversion to that type */
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

  /* if root (>) or sexpr or qexpr then create an empty list */
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr")) { x = lval_sexpr(); }
  if (strstr(t->tag, "qexpr")) { x = lval_qexpr(); }

  /* fill this list with any valid expression contained within */
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->tag, "regex") == 0) { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;
}

lval* lval_call(lenv* e, lval* f, lval* a) {
  if (f->builtin) { return f->builtin(e, a); }

  int given = a->count;
  int total = f->formals->count;

  while (a->count) {
    if (f->formals->count == 0) {
      lval_del(a);
      return lval_err(
        "Function passed too many arguments. Got %i, expected %i",
        given,
        total
      );
    }

    lval* sym = lval_pop(f->formals, 0);
    if (strcmp(sym->sym, "&") == 0) {
      if (f->formals->count != 1) {
        // ensure '&' is followed by another symbol
        lval_del(a);
        return lval_err("Function format invalid. Symbol '&' not followed by single symbol");
      }

      // bind all of the remaining arguments to the next formal
      lval* nsym = lval_pop(f->formals, 0);
      lenv_put(f->env, nsym, builtin_list(e, a));
      lval_del(sym);
      lval_del(nsym);
      break;
    }
    lval* val = lval_pop(a, 0);
    lenv_put(f->env, sym, val);
    lval_del(sym);
    lval_del(val);
  }

  // argument list is now bound, so this can be cleaned up
  lval_del(a);

  if (f->formals->count > 0 && strcmp(f->formals->cell[0]->sym, "&") == 0) {
    // if '&' remains in formals list
    if (f->formals->count != 2) {
      return lval_err("Function format invalid. Symbol '&' not followed by single symbol");
    }

    lval_del(lval_pop(f->formals, 0)); // pop and delete '&' symbol
    lval* sym = lval_pop(f->formals, 0);
    lval* val = lval_qexpr();
    lenv_put(f->env, sym, val);
    lval_del(sym);
    lval_del(val);
  }

  if (f->formals->count == 0) {
    // if all formals have been evaluated, evaluate and return
    f->env->par = e;
    return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
  } else {
    // otherwise return partially evaluated function
    return lval_copy(f);
  }
}

lval* lval_eval_sexpr(lenv* e, lval* v);

lval* lval_eval(lenv* e, lval* v) {
  if (v->type == LVAL_SYM) {
    lval* x = lenv_get(e, v);
    lval_del(v);
    return x;
  }
  /* evaluate S-Expressions */
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
  /* all other lval types remain the same */
  return v;
}

lval* lval_eval_sexpr(lenv* e, lval* v) {
  /* evaluate children */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(e, v->cell[i]);
  }

  /* error checking */
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
  }

  /* empty expression */
  if (v->count == 0) { return v; }

  /* single expression */
  if (v->count == 1) { return lval_take(v, 0); }

  /* ensure first element is a function after evaluation */
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_FUN) {
    lval* err = lval_err(
      "S-Expression starts with incorrect type. Got %s, expected %s.",
      ltype_name(f->type),
      ltype_name(LVAL_FUN)
    );
    lval_del(v);
    lval_del(f);
    return err;
  }

  /* If so call the function to get the result */
  lval* result = lval_call(e, f, v);
  lval_del(f);
  return result;
}

int lvals_are_equal(lval* x, lval* y) {
  if (x->type != y->type) { return 0; }

  switch (x->type) {
    case LVAL_NUM: return (x->num == y->num);
    case LVAL_ERR: return (strcmp(x->err, y->err) == 0);
    case LVAL_SYM: return (strcmp(x->sym, y->sym) == 0);
    case LVAL_FUN:
      if (x->builtin || y->builtin) {
        return x->builtin == y->builtin;
      } else {
        return lvals_are_equal(x->formals, y->formals) && lvals_are_equal(x->body, y->body);
      }
    case LVAL_QEXPR:
    case LVAL_SEXPR:
      if (x->count != y->count) { return 0; }
      for (int i = 0; i < x->count; i++) {
        if (!lvals_are_equal(x->cell[i], y->cell[i])) { return 0; }
      }
      return 1;
    break;
  }
  return 0;
}

/*
Definitions of leesp's built in functions
*/

#include "assertions.h"
#include "arithmetic.h"
#include "comparison.h"
#include "list.h"

lval* builtin_lambda(lenv* e, lval* a) {
  LASSERT_NUM("\\", a, 2);
  LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
  LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

  for (int i = 0; i < a->cell[0]->count; i++) {
    // first Q expression must only contain symbols
    LASSERT(
      a,
      (a->cell[0]->cell[i]->type == LVAL_SYM),
      "Cannot define non-symbol. Got %s, expected %s.",
      ltype_name(a->cell[0]->cell[i]->type),
      ltype_name(LVAL_SYM)
    );
  }

  lval* formals = lval_pop(a, 0);
  lval* body = lval_pop(a, 0);
  lval_del(a);

  return lval_lambda(formals, body);
}

lval* builtin_var(lenv* e, lval* a, char* func) {
  LASSERT_TYPE(func, a, 0, LVAL_QEXPR);

  /* first arg is a symbol list */
  lval* syms = a->cell[0];
  for (int i = 0; i < syms->count; i++) {
    LASSERT(
      a,
      (syms->cell[i]->type == LVAL_SYM),
      "Function '%s' cannot define non-symbol. Got %s, expected %s.",
      func,
      ltype_name(syms->cell[i]->type),
      ltype_name(LVAL_SYM)
    );
  }

  LASSERT(
    a,
    syms->count == a->count - 1,
    "Function '%s' cannot define incorrect number of values to symbols. Received %i symbols but %i values",
    func,
    syms->count,
    a->count - 1
  );

  /* assign copies of values to symbols */
  for (int i = 0; i < syms->count; i++) {
    /* if 'def' define in global, if 'put' define locally */
    if (strcmp(func, "def") == 0) {
      lenv_def(e, syms->cell[i], a->cell[i + 1]);
    } else if (strcmp(func, "=") == 0) {
      lenv_put(e, syms->cell[i], a->cell[i + 1]);
    }
  }

  lval_del(a);
  return lval_sexpr();
}

lval* builtin_def(lenv* e, lval* a) {
  return builtin_var(e, a, "def");
}

lval* builtin_put(lenv* e, lval* a) {
  return builtin_var(e, a, "=");
}

lval* builtin_if(lenv* e, lval* a) {
  LASSERT_NUM("if", a, 3);
  LASSERT_TYPE("if", a, 0, LVAL_NUM);
  LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
  LASSERT_TYPE("if", a, 2, LVAL_QEXPR);

  lval* x;
  a->cell[1]->type = LVAL_SEXPR;
  a->cell[2]->type = LVAL_SEXPR;

  if (a->cell[0]->num) {
    x = lval_eval(e, lval_pop(a, 1));
  } else {
    x = lval_eval(e, lval_pop(a, 2));
  }

  lval_del(a);
  return x;
}

/*
Definitions of leesp's built in functions
*/

#define LASSERT(args, cond, fmt, ...) \
  if (!(cond)) { \
    lval* err = lval_err(fmt, ##__VA_ARGS__); \
    lval_del(args); \
    return err; \
  }

lval* builtin_op(lenv* e, lval* a, char* op) {
  /* ensure all arguments are numbers */
  for (int i = 0; i < a->count; i++) {
    LASSERT(
      a,
      a->cell[i]->type == LVAL_NUM,
      "function %s passed an incorrect type for argument %i. Got %s, expected %s",
      op,
      i,
      ltype_name(a->cell[i]->type),
      ltype_name(LVAL_NUM)
    );
  }

  /* pop the first element */
  lval* x = lval_pop(a, 0);

  /* if no arguments and sub then perform unary negation */
  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->num = -x->num;
  }

  while (a->count > 0) {
    lval* y = lval_pop(a, 0);

    if (strcmp(op, "+") == 0) { x->num += y->num; }
    if (strcmp(op, "-") == 0) { x->num -= y->num; }
    if (strcmp(op, "*") == 0) { x->num *= y->num; }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(x);
        lval_del(y);
        x = lval_err("Division by zero!");
        break;
      }
      x->num /= y->num;
    }

    lval_del(y);
  }

  lval_del(a);
  return x;
}

lval* builtin_add(lenv* e, lval* a) {
  return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a) {
  return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a) {
  return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a) {
  return builtin_op(e, a, "/");
}

lval* builtin_head(lenv* e, lval* a) {
  /* takes a Q-Expression and returns a Q-Expression of only the first element */
  LASSERT(a, a->count == 1, "Function 'head' passed too many arguments. Got %i, expected %i.", a->count, 1);
  LASSERT(
    a,
    a->cell[0]->type == LVAL_QEXPR,
    "Function 'head' passed incorrect type for argument 0. Got %s, expected %s.",
    ltype_name(a->cell[0]->type),
    ltype_name(LVAL_QEXPR)
  );
  LASSERT(a, a->cell[0]->count != 0, "Function 'head' passed an empty expression of {}!");

  lval* v = lval_take(a, 0);
  while (v->count > 1) {
    lval_del(lval_pop(v, 1));
  }
  return v;
}

lval* builtin_tail(lenv* e, lval* a) {
  /* takes a Q-Expression and returns a Q-Expression with the first element removed */
  LASSERT(a, a->count == 1, "Function 'tail' passed too many arguments. Got %i, expected %i.", a->count, 1);
  LASSERT(
    a,
    a->cell[0]->type == LVAL_QEXPR,
    "Function 'tail' passed incorrect type for argument 0. Got %s, expected %s.",
    ltype_name(a->cell[0]->type),
    ltype_name(LVAL_QEXPR)
  );
  LASSERT(a, a->cell[0]->count != 0, "Function 'tail' passed an empty expression of {}!");

  lval* v = lval_take(a, 0);
  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_list(lenv* e, lval* a) {
  /* takes one or more arguments and returns a new Q-Expression containing the arguments */
  a->type = LVAL_QEXPR;
  return a;
}

lval* builtin_eval(lenv* e, lval* a) {
  /* takes a Q-Expression and evaluates it as if it were a S-Expression */
  LASSERT(a, a->count == 1, "Function 'eval' passed too many arguments. Got %i, expected %i.", a->count, 1);
  LASSERT(
    a,
    a->cell[0]->type == LVAL_QEXPR,
    "Function 'eval' passed incorrect type for argument 0. Got %s, expected %s.",
    ltype_name(a->cell[0]->type),
    ltype_name(LVAL_QEXPR)
  );

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* a) {
  /* takes one or more Q-Expressions and returns a Q-Expression of them joined together */
  for (int i = 0; i < a->count; i++) {
    LASSERT(
      a,
      a->cell[i]->type == LVAL_QEXPR,
      "function 'join' passed an incorrect type for argument %i. Got %s, expected %s",
      i,
      ltype_name(a->cell[i]->type),
      ltype_name(LVAL_QEXPR)
    );
  }

  lval* x = lval_pop(a, 0);

  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);
  return x;
}

lval* builtin_def(lenv* e, lval* a) {
  LASSERT(
    a,
    a->cell[0]->type == LVAL_QEXPR,
    "Function 'def' passed incorrect type for argument 0. Got %s, expected %s.",
    ltype_name(a->cell[0]->type),
    ltype_name(LVAL_QEXPR)
  );

  /* first arg is a symbol list */
  lval* syms = a->cell[0];
  for (int i = 0; i < syms->count; i++) {
    LASSERT(
      a,
      syms->cell[i]->type == LVAL_SYM,
      "function 'def' passed an incorrect type for argument %i. Got %s, expected %s",
      i,
      ltype_name(syms->cell[i]->type),
      ltype_name(LVAL_SYM)
    );
  }

  LASSERT(
    a,
    syms->count == a->count - 1,
    "Function 'def' cannot define incorrect number of values to symbols. Received %i symbols but %i values",
    syms->count,
    a->count - 1
  );

  /* assign copies of values to symbols */
  for (int i = 0; i < syms->count; i++) {
    lenv_put(e, syms->cell[i], a->cell[i + 1]);
  }

  lval_del(a);
  return lval_sexpr();
}

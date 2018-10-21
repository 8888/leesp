int lvals_are_equal(lval* x, lval* y);

lval* builtin_comparison(lenv* e, lval* a, char* op) {
  LASSERT_NUM(op, a, 2);
  LASSERT_TYPE(op, a, 0, LVAL_NUM);
  LASSERT_TYPE(op, a, 1, LVAL_NUM);

  int r = 0;
  if (strcmp(op, ">") == 0) {
    r = (a->cell[0]->num > a->cell[1]->num);
  } else if (strcmp(op, "<") == 0) {
    r = (a->cell[0]->num < a->cell[1]->num);
  } else if (strcmp(op, ">=") == 0) {
    r = (a->cell[0]->num >= a->cell[1]->num);
  } else if (strcmp(op, "<=") == 0) {
    r = (a->cell[0]->num <= a->cell[1]->num);
  }
  lval_del(a);
  return lval_num(r);
}

lval* builtin_greater_than(lenv* e, lval* a) {
  return builtin_comparison(e, a, ">");
}

lval* builtin_less_than(lenv* e, lval* a) {
  return builtin_comparison(e, a, "<");
}

lval* builtin_greater_than_equal(lenv* e, lval* a) {
  return builtin_comparison(e, a, ">=");
}

lval* builtin_less_than_equal(lenv* e, lval* a) {
  return builtin_comparison(e, a, "<=");
}

lval* builtin_equality(lenv* e, lval* a, char* op) {
  LASSERT_NUM(op, a, 2);
  int r = 0;
  if (strcmp(op, "==") == 0) {
    r = lvals_are_equal(a->cell[0], a->cell[1]);
  } else if (strcmp(op, "!=") == 0) {
    r = !lvals_are_equal(a->cell[0], a->cell[1]);
  }
  lval_del(a);
  return lval_num(r);
}

lval* builtin_equal_to(lenv* e, lval* a) {
  return builtin_equality(e, a, "==");
}

lval* builtin_not_equal(lenv* e, lval* a) {
  return builtin_equality(e, a, "!=");
}

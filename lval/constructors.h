lenv* lenv_new(void);

/* enum of possible lval types */
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR };

lval* lval_num(long x) {
  /* construct a pointer to a new Number lval */
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval* lval_err(char* fmt, ...) {
  /* construct a pointer to a new Error lval */
  int error_size = 512;
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(error_size);

  va_list va;
  va_start(va, fmt);
  vsnprintf(v->err, error_size - 1, fmt, va);
  v->err = realloc(v->err, strlen(v->err)+1);
  va_end(va);

  return v;
}

lval* lval_sym(char* s) {
  /* construct a pointer to a new Symbol lval */
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval* lval_fun(lbuiltin func) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->builtin = func;
  return v;
}

lval* lval_sexpr(void) {
  /* construct a pointer to a new S-Expression lval */
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_qexpr(void) {
  /* construct a pointer to a new Q-Expression  */
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_lambda(lval* formals, lval* body) {
  /* construct a pointer to a new lambda function lval */
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->builtin = NULL;
  v->env = lenv_new();
  v->formals = formals;
  v->body = body;
  return v;
}

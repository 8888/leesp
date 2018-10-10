#include <stdio.h>
#include <stdlib.h>

#include "mpc/mpc.h"

#include "lval.h"
#include "lenv.h"

// compile if compiling on windows
#ifdef _WIN32
  #include <string.h>

  static char buffer[2048];

  // fake the readline function
  char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* cpy = malloc(strlen(buffer) +1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy) -1] = '\0';
    return cpy;
  }

  // fake the add_history function
  void add_history(char* unused) {}

// otherwise include the editline headers
#elif __APPLE__
  #include <editline/readline.h>
#else
  // linux/unix/posix
  #include <editline/readline.h>
  #include <editline/history.h>
#endif

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

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
  lval* k = lval_sym(name);
  lval* v = lval_fun(func);
  lenv_put(e, k, v);
  lval_del(k);
  lval_del(v);
}

void lenv_add_builtins(lenv* e) {
  /* list functions */
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);
  lenv_add_builtin(e, "def", builtin_def);

  /* math functions */
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
}

int main(int argc, char** argv) {
  /* create some parsers */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr = mpc_new("sexpr");
  mpc_parser_t* Qexpr = mpc_new("qexpr");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Leesp = mpc_new("leesp");

  /* define them with the following language */
  mpca_lang(MPCA_LANG_DEFAULT,
    " \
      number: /-?[0-9]+/ ; \
      symbol: /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ; \
      sexpr: '(' <expr>* ')' ; \
      qexpr: '{' <expr>* '}' ; \
      expr: <number> | <symbol>  | <sexpr> | <qexpr> ; \
      leesp: /^/ <expr>* /$/ ; \
    ",
    Number, Symbol, Sexpr, Qexpr, Expr, Leesp
  );

  puts("Leesp version 0.0.0.0.1");
  puts("Press ctrl+c to exit\n");

  lenv* e = lenv_new();
  lenv_add_builtins(e);

  while (1) {
    char* input = readline("leesp> ");
    add_history(input);

    /* attempt to parse the user input */
    mpc_result_t r;
    // mpc_parse returns 1 on success and 0 on failure
    if (mpc_parse("<stdin>", input, Leesp, &r)) {
      lval* x = lval_eval(e, lval_read(r.output));
      lval_print_ln(x);
      lval_del(x);

      mpc_ast_delete(r.output);
    } else {
      /* otherwise print the error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  lenv_del(e);

  /* undefine and delete our parsers */
  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Leesp);

  return 0;
}

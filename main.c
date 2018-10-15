#include <stdio.h>
#include <stdlib.h>

#include "mpc/mpc.h"

#include "lval.h"
#include "lenv.h"
#include "builtin.h"

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
  lenv_add_builtin(e, "=", builtin_put);

  /* math functions */
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);

  lenv_add_builtin(e, "\\", builtin_lambda);
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

  puts("Leesp version 0.0.12");
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

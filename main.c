#include <stdio.h>
#include <stdlib.h>

#include "include/mpc/mpc.h"

/* parser foward declarations */
mpc_parser_t* Number;
mpc_parser_t* Symbol;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Leesp;

#include "shared/structs.h"
#include "lval/lval.h"
#include "lenv/lenv.h"
#include "builtin_functions/builtin.h"

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

  /* comparison functions */
  lenv_add_builtin(e, ">", builtin_greater_than);
  lenv_add_builtin(e, "<", builtin_less_than);
  lenv_add_builtin(e, ">=", builtin_greater_than_equal);
  lenv_add_builtin(e, "<=", builtin_less_than_equal);

  /* equality functions */
  lenv_add_builtin(e, "==", builtin_equal_to);
  lenv_add_builtin(e, "!=", builtin_not_equal);

  lenv_add_builtin(e, "\\", builtin_lambda);
  lenv_add_builtin(e, "if", builtin_if);

  /* string functions */
  lenv_add_builtin(e, "load", builtin_load);
  lenv_add_builtin(e, "error", builtin_error);
  lenv_add_builtin(e, "print", builtin_print);
}

void load_standard_library(lenv* e) {
  lval* args = lval_add(lval_sexpr(), lval_str("./library/standard.leesp"));
  lval* result = builtin_load(e, args);
  if (result->type == LVAL_ERR) { lval_print_ln(result); }
  lval_del(result);
}

int main(int argc, char** argv) {
  /* create some parsers */
  Number = mpc_new("number");
  Symbol = mpc_new("symbol");
  String = mpc_new("string");
  Comment = mpc_new("comment");
  Sexpr = mpc_new("sexpr");
  Qexpr = mpc_new("qexpr");
  Expr = mpc_new("expr");
  Leesp = mpc_new("leesp");

  /* define them with the following language */
  mpca_lang(MPCA_LANG_DEFAULT,
    " \
      number: /-?[0-9]+/ ; \
      symbol: /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ; \
      string: /\"(\\\\.|[^\"])*\"/ ; \
      comment: /;[^\\r\\n]*/ ; \
      sexpr: '(' <expr>* ')' ; \
      qexpr: '{' <expr>* '}' ; \
      expr: <number> | <symbol> | <string> | <comment> | <sexpr> | <qexpr> ; \
      leesp: /^/ <expr>* /$/ ; \
    ",
    Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Leesp
  );

  lenv* e = lenv_new();
  lenv_add_builtins(e);
  load_standard_library(e);

  if (argc == 1) {
    puts("Leesp version 0.15.0");
    puts("Press ctrl+c to exit\n");

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
  } else {
    for (int i = 1; i < argc; i++) {
      // i = 1 because first argument is always the program
      lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));
      lval* result = builtin_load(e, args);
      if (result->type == LVAL_ERR) { lval_print_ln(result); }
      lval_del(result);
    }
  }

  lenv_del(e);

  /* undefine and delete our parsers */
  mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Leesp);

  return 0;
}

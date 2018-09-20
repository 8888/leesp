#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"

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

long eval_op(long x, char* op, long y) {
  if (strcmp(op, "+") == 0) { return x + y; }
  if (strcmp(op, "-") == 0) { return x - y; }
  if (strcmp(op, "*") == 0) { return x * y; }
  if (strcmp(op, "/") == 0) { return x / y; }
  return 0;
}

long eval(mpc_ast_t* t) {
  /* if tagged as a number, return it directly */
  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  char* op = t->children[1]->contents; // the operator is always the second child
  long x = eval(t->children[2]); // store the third child in x

  /* iterate the remaining children and combine */
  int i = 3; // starting from 4th child
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

int main(int argc, char** argv) {
  /* create some parsers */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Leesp = mpc_new("leesp");

  /* define them with the following language */
  mpca_lang(MPCA_LANG_DEFAULT,
    " \
      number: /-?[0-9]+/ ; \
      operator: '+' | '-' | '*' | '/' ; \
      expr: <number> | '(' <operator> <expr>+ ')' ; \
      leesp: /^/ <operator> <expr>+ /$/ ; \
    ",
    Number, Operator, Expr, Leesp
  );

  puts("Leesp version 0.0.0.0.1");
  puts("Press ctrl+c to exit\n");

  while (1) {
    char* input = readline("leesp> ");
    add_history(input);

    /* attempt to parse the user input */
    mpc_result_t r;
    // mpc_parse returns 1 on success and 0 on failure
    if (mpc_parse("<stdin>", input, Leesp, &r)) {
      long result = eval(r.output);
      printf("%li\n", result);
      mpc_ast_delete(r.output);
    } else {
      /* otherwise print the error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  /* undefine and delete our parsers */
  mpc_cleanup(4, Number, Operator, Expr, Leesp);

  return 0;
}
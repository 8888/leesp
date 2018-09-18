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

int main(int argc, char** argv) {
  puts("Leesp version 0.0.0.0.1");
  puts("Press ctrl+c to exit\n");

  while (1) {
    char* input = readline("leesp> ");
    add_history(input);

    printf("You said %s\n", input);
    free(input);
  }

  return 0;
}
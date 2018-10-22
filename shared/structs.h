struct lval;
typedef struct lval lval;
struct lenv;
typedef struct lenv lenv;
typedef lval*(*lbuiltin)(lenv*, lval*);

struct lenv {
  lenv* par;
  int count;
  char** syms;
  lval** vals;
};

struct lval {
  int type;

  /* basic */
  long num;
  char* err;
  char* sym;
  char* str;

  /* function */
  lbuiltin builtin;
  lenv* env;
  lval* formals;
  lval* body;

  /* expression */
  int count;
  lval** cell;
};

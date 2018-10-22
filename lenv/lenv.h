/*
Create an environment to store relationships between symbols and values
This is a naive approach of two equal length lists
*/

#include "edit.h"

lenv* lenv_new(void) {
  lenv* e = malloc(sizeof(lenv));
  e->par = NULL;
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  return e;
}

void lenv_def(lenv* e, lval* k, lval* v) {
  /* define in the top-most global env */
  while (e->par) { e = e->par; }
  lenv_put(e, k, v);
}

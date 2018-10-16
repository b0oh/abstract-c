#pragma once

#include <sys/queue.h>

typedef struct Sexp {
  enum {
        SEXP_SYMBOL,
        SEXP_LIST
  } type;

  TAILQ_ENTRY(Sexp) entry;
} Sexp;

typedef struct SexpSymbol {
  Sexp super;

  const char name[];
} SexpSymbol;

typedef struct SexpList {
  Sexp super;

  TAILQ_HEAD(SexpListHead, Sexp) head;
} SexpList;

Sexp* sexp_read(const char* input);
void sexp_free(const Sexp* sexp);

Sexp* sexp_clone(const Sexp* sexp);

SexpSymbol* sexp_symbol_new(const char* name);
SexpList* sexp_list_new();
void sexp_list_add(SexpList* sexp, Sexp* inner);

void sexp_print(const Sexp* sexp);

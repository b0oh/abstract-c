#pragma once

typedef struct Term {
  enum {
        TERM_VAR,
        TERM_ABS,
        TERM_APP
  } type;
} Term;

typedef struct TermVar {
  Term super;

  const char name[];
} TermVar;

typedef struct TermAbs {
  Term super;

  Term* body;
  const char name[];
} TermAbs;

typedef struct TermApp {
  Term super;

  Term* abs;
  Term* arg;
} TermApp;


Term* term_var_new(const char* name);
Term* term_app_new(Term* abs, Term* arg);
Term* term_abs_new(const char* name, Term* body);

Term* term_clone(const Term* term);

void term_free(const Term* term);

Term* term_beta(const Term* term, const char* name, const Term* subst);

Term* term_reduce_once(const Term* term);
Term* term_full_beta(const Term* term);

void term_print(const Term* term);

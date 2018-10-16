#include "term.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VAR(term) ((const TermVar*) (term))
#define ABS(term) ((const TermAbs*) (term))
#define APP(term) ((const TermApp*) (term))

Term* term_var_new(const char* name) {
  size_t name_lenght = strlen(name) + 1;
  TermVar* term = malloc(sizeof(TermVar) + name_lenght);
  term->super.type = TERM_VAR;
  memcpy((void*) term->name, name, name_lenght);
  return &term->super;
}

Term* term_abs_new(const char* name, Term* body) {
  size_t name_lenght = strlen(name) + 1;
  TermAbs* term = malloc(sizeof(TermAbs) + name_lenght);
  term->super.type = TERM_ABS;
  memcpy((void*) term->name, name, name_lenght);
  term->body = body;

  return &term->super;

}

Term* term_app_new(Term* abs, Term* arg) {
  TermApp* term = malloc(sizeof(TermApp));
  term->super.type = TERM_APP;
  term->abs = abs;
  term->arg = arg;
  return &term->super;
}

Term* term_clone(const Term* term) {
  switch (term->type) {
  case TERM_VAR:
    return term_var_new(VAR(term)->name);
  case TERM_ABS:
    return term_abs_new(ABS(term)->name, ABS(term)->body);
  case TERM_APP:
    return term_app_new(APP(term)->abs, APP(term)->arg);
  }
}

static void term_var_free(TermVar* term) {
  free(term);
}

static void term_abs_free(TermAbs* term) {
  term_free(term->body);
  free(term);
}

static void term_app_free(TermApp* term) {
  term_free(term->abs);
  term_free(term->arg);
  free(term);
}

void term_free(const Term* term) {
  switch (term->type) {
  case TERM_VAR:
    term_var_free((TermVar*) term);
    break;
  case TERM_ABS:
    term_abs_free((TermAbs*) term);
    break;
  case TERM_APP:
    term_app_free((TermApp*) term);
    break;
  }
}

Term* term_beta(const Term* term, const char* name, const Term* subst) {
  switch (term->type) {

  case TERM_VAR:
    if (strcmp(((const TermVar*) term)->name, name) == 0) {
      return term_clone(subst);
    } else {
      return term_clone(term);
    }
    break;

  case TERM_ABS: {
    const TermAbs* term_abs = (const TermAbs*) term;
    if (strcmp(term_abs->name, name) == 0) {
      return term_clone(term);
    } else {
      return term_abs_new(term_abs->name, term_beta(term_abs->body, name, subst));
    }
    break;
  }

  case TERM_APP: {
    const TermApp* term_app = (const TermApp*) term;
    Term* beta_abs = term_beta(term_app->abs, name, subst);
    Term* beta_arg = term_beta(term_app->arg, name, subst);
    return term_app_new(beta_abs, beta_arg);
  }
  }
}

Term* term_reduce_once(const Term* term) {
  switch (term->type) {
  case TERM_VAR:
    return term_clone(term);
  case TERM_ABS:
    return term_abs_new(ABS(term)->name, term_reduce_once(ABS(term)->body));
  case TERM_APP:
    if (APP(term)->abs->type == TERM_ABS) {
      const TermAbs* abs = ABS(APP(term)->abs);
      return term_beta(abs->body, abs->name, APP(term)->arg);
    } else {
      return term_app_new(term_reduce_once(APP(term)->abs),
                          term_reduce_once(APP(term)->arg));
    }
  }
}

static bool term_is_normal(const Term* term) {
  switch (term->type) {
  case TERM_VAR:
    return true;
  case TERM_ABS:
    return term_is_normal(ABS(term)->body);
  case TERM_APP:
    if (APP(term)->abs->type == TERM_ABS) {
      return false;
    }
    return term_is_normal(APP(term)->abs) &&
      term_is_normal(APP(term)->arg);
  }
}

Term* term_full_beta(const Term* term) {
  Term* new_term = term_clone(term);;

  do {
    printf("full beta iteration: ");
    term_print(term);
    printf("\n");
    new_term = term_reduce_once(new_term);
  } while (!term_is_normal(new_term));

  return new_term;
}

void term_print(const Term* term) {
  switch (term->type) {
  case TERM_VAR: {
    printf("%s", ((TermVar*) term)->name);
    break;
  }

  case TERM_ABS: {
    const TermAbs* abs = (TermAbs*) term;
    printf("(%s -> ", abs->name);
    term_print(abs->body);
    printf(")");
    break;
  }

  case TERM_APP: {
    const TermApp* app = (TermApp*) term;
    printf("(");
    term_print(app->abs);
    printf(" ");
    term_print(app->arg);
    printf(")");
    break;
  }

  default:
    break;
  }
}

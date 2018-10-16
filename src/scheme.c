#include "scheme.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sexp.h"
#include "term.h"

#define fail(message)                           \
  assert(0 && message);                         \
  abort();                                      \

#define ENSURE_TYPE(VAR, TYPE)                  \
  if ((VAR)->type != TYPE) {                    \
    fail("type of " #VAR " is not " #TYPE);     \
  }                                             \

#define CHECK_NAME(VAR, NAME)                   \
  if (strcmp((VAR)->name, NAME) != 0) {         \
    return NULL;                                \
  }                                             \

#define DECLARE_SEXP(VAR, TYPE, SEXP)           \
  const TYPE* VAR = (const TYPE*) (SEXP);       \
  if (!VAR) {                                   \
    fail("missing " #VAR);                      \
  }                                             \

#define DECLARE_SEXP_SYMBOL(VAR, SEXP)          \
  ENSURE_TYPE((SEXP), SEXP_SYMBOL);             \
  DECLARE_SEXP(VAR, SexpSymbol, (SEXP));        \

#define DECLARE_SEXP_LIST(VAR, SEXP)            \
  DECLARE_SEXP(VAR, SexpList, SEXP);            \
  ENSURE_TYPE(&VAR->super, SEXP_LIST);          \

#define ENSURE_NULL(VAR)                        \
  if (VAR) {                                    \
    fail("too many elements after " #VAR);      \
  }                                             \

static Term* extract_sexp_symbol(const SexpSymbol* sexp) {
  return term_var_new(sexp->name);
}

static inline const SexpSymbol* check_symbol(const Sexp* sexp, const char* name) {
  if (sexp->type != SEXP_SYMBOL) {
    return NULL;
  }

  const SexpSymbol* symbol = (const SexpSymbol*) sexp;
  if (strcmp(symbol->name, name) != 0) {
    return NULL;
  }

  return symbol;
}

static Term* extract_lambda_chain(const Sexp* arg, Term* body) {
  if (arg->type != SEXP_SYMBOL) {
    fail("lambda argument is not symbol");
  }

  const SexpSymbol* arg_symbol = (const SexpSymbol*) arg;

  const Sexp* rest = TAILQ_NEXT(arg, entry);
  if (rest) {
    body = extract_lambda_chain(rest, body);
  }

  return term_abs_new(arg_symbol->name, body);
}

Term* try_extract_lambda(const Sexp* sexp) {
  /* (lambda (qwe) arg) */

  const SexpSymbol* symbol = check_symbol(sexp, "lambda");
  if (!symbol) {
    return NULL;
  }

  const Sexp* args = TAILQ_NEXT(&symbol->super, entry);
  if (!args) {
    fail("missing lambda args");
  }

  if (args->type != SEXP_LIST) {
    fail("lambda args are not list");
  }

  const SexpList* args_list = (const SexpList*) args;
  const Sexp* first = TAILQ_FIRST(&args_list->head);
  if (!first) {
    fail("empty lambda arg list");
  }

  const Sexp* body = TAILQ_NEXT(args, entry);
  if (!body) {
    fail("missing lambda body");
  }

  if (TAILQ_NEXT(body, entry)) {
    fail("extra lambda list arguments");
  }

  Term* body_term = scheme_extract(body);
  return extract_lambda_chain(first, body_term);
}

static Term* try_extract_let(const Sexp* sexp) {
  /* (let ((a 1) (b 2)) body) */

  /* (lambda (b) ((lambda (a) body) 1)) 2 */

  DECLARE_SEXP_SYMBOL(symbol, sexp);
  CHECK_NAME(symbol, "let");

  DECLARE_SEXP_LIST(bindings, TAILQ_NEXT(&symbol->super, entry));

  DECLARE_SEXP(body, Sexp, TAILQ_NEXT(&bindings->super, entry));

  ENSURE_NULL(TAILQ_NEXT(body, entry));

  printf("let\n");

  Term* term = scheme_extract(body);

  const Sexp* binding = TAILQ_FIRST(&bindings->head);
  while (binding) {

    DECLARE_SEXP_LIST(binding_list, binding);

    DECLARE_SEXP_SYMBOL(name, TAILQ_FIRST(&binding_list->head));
    DECLARE_SEXP(value, Sexp, TAILQ_NEXT(&name->super, entry));
    ENSURE_NULL(TAILQ_NEXT(value, entry));

    term = term_abs_new(name->name, term);
    term = term_app_new(term, scheme_extract(value));

    binding = TAILQ_NEXT(binding, entry);
  }

  return term;
}

static Term* try_extract_let_star(const Sexp* sexp) {
  /* (let* ((a 1) (b 2)) body) */
  /* (let ((a 1)) (let ((b 2)) body)) */

  DECLARE_SEXP_SYMBOL(symbol, sexp);
  CHECK_NAME(symbol, "let*");

  DECLARE_SEXP_LIST(bindings, TAILQ_NEXT(&symbol->super, entry));

  DECLARE_SEXP(body, Sexp, TAILQ_NEXT(&bindings->super, entry));

  ENSURE_NULL(TAILQ_NEXT(body, entry));

  printf("let*\n");

  Sexp* nested_lets = sexp_clone(body);

  const Sexp* binding = TAILQ_LAST(&bindings->head, SexpListHead);
  while (binding) {

    DECLARE_SEXP_LIST(binding_list, binding);

    DECLARE_SEXP_SYMBOL(name, TAILQ_FIRST(&binding_list->head));
    DECLARE_SEXP(value, Sexp, TAILQ_NEXT(&name->super, entry));
    ENSURE_NULL(TAILQ_NEXT(value, entry));

    SexpList* inner_lets = sexp_list_new();
    sexp_list_add(inner_lets, &sexp_symbol_new("let")->super);

    SexpList* inner_bindings = sexp_list_new();

    SexpList* inner_binding = sexp_list_new();
    sexp_list_add(inner_binding, &sexp_symbol_new(name->name)->super);
    sexp_list_add(inner_binding, sexp_clone(value));

    sexp_list_add(inner_bindings, &inner_binding->super);

    sexp_list_add(inner_lets, &inner_bindings->super);
    sexp_list_add(inner_lets, nested_lets);
    nested_lets = &inner_lets->super;

    binding = TAILQ_PREV(binding, SexpListHead, entry);
  }

  return scheme_extract(nested_lets);
}

static Term* extract_app_chain(const Sexp* arg, Term* acc) {
  if (!arg) {
    return acc;
  }

  Term* term = term_app_new(acc, scheme_extract(arg));
  const Sexp* rest = TAILQ_NEXT(arg, entry);
  return extract_app_chain(rest, term);
}

static Term* extract_app(const Sexp* abs) {
  const Sexp* arg = TAILQ_NEXT(abs, entry);
  if (!arg) {
    fail("missing app arg");
  }

  return extract_app_chain(arg, scheme_extract(abs));
}

static Term* extract_sexp_list(const SexpList* sexp) {
  printf("list: ");
  sexp_print(&sexp->super);
  printf("\n");

  const Sexp* symbol = TAILQ_FIRST(&sexp->head);
  if (!symbol) {
    fail("empty list");
  }

  if (symbol->type == SEXP_SYMBOL) {
    Term* term = try_extract_lambda(symbol);
    if (term) {
      return term;
    }

    term = try_extract_let(symbol);
    if (term) {
      return term;
    }

    term = try_extract_let_star(symbol);
    if (term) {
      return term;
    }
  }

  return extract_app(symbol);
}

Term* scheme_extract(const Sexp* sexp) {
  switch (sexp->type) {
  case SEXP_SYMBOL:
    return extract_sexp_symbol((const SexpSymbol*) sexp);

  case SEXP_LIST:
    return extract_sexp_list((const SexpList*) sexp);
  }
}

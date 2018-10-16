#include "sexp.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SexpSymbol* sexp_symbol_new_with_lenght(const char* name, size_t lenght) {
  SexpSymbol* sexp = malloc(sizeof(SexpSymbol) + lenght + 1);
  char* sexp_name = (char*) sexp->name;
  memcpy(sexp_name, name, lenght);
  sexp_name[lenght] = 0;
  sexp->super.type = SEXP_SYMBOL;
  return sexp;
}

SexpSymbol* sexp_symbol_new(const char* name) {
  return sexp_symbol_new_with_lenght(name, strlen(name));
}

void sexp_free(const Sexp* sexp) {
  switch (sexp->type) {
  case SEXP_SYMBOL:
    free((void*) sexp);
    break;
  case SEXP_LIST: {
    const SexpList* sexp_list = (const SexpList*) sexp;
    Sexp *element, *tmp;
    TAILQ_FOREACH_SAFE(element, &sexp_list->head, entry, tmp) {
      sexp_free(element);
    }
    free((void*) sexp);
    break;
  }
  }
}

static bool is_special(char byte) {
  return index("+*^/#,\\-_?!|>", byte) != NULL;
}

static bool is_symbol_char(char byte) {
  return isalpha(byte) || isdigit(byte) || is_special(byte);
}

typedef struct SexpReader {
  const char* input;
} SexpReader;

static SexpSymbol* sexp_symbol_read(SexpReader* reader) {
  const char* end = reader->input;
  while (*end && is_symbol_char(*end)) {
    ++end;
  }

  if (reader->input == end) {
    return NULL;
  }

  const char* begin = reader->input;
  reader->input = end;
  return sexp_symbol_new_with_lenght(begin, end - begin);
}

SexpList* sexp_list_new() {
  /* printf("sexp_list_new\n"); */
  SexpList* sexp = malloc(sizeof(SexpList));
  sexp->super.type = SEXP_LIST;
  TAILQ_INIT(&sexp->head);
  return sexp;
}

void sexp_list_add(SexpList* sexp, Sexp* inner) {
  /* printf("sexp_list_add %p %p\n", (void*) sexp, (void*) inner); */
  TAILQ_INSERT_TAIL(&sexp->head, inner, entry);
}

static void skip_white(SexpReader* reader) {
  while (*reader->input && index(" \n", *reader->input) != NULL) {
      ++reader->input;
  }
}

static void skip_comment(SexpReader* reader) {
  if (*reader->input == ';') {
    while (*reader->input && *reader->input != '\n') {
      ++reader->input;
    }
    if (*reader->input) {
      /* skip \n */
      ++reader->input;
    }
  }
}

static Sexp* sexp_read_inner(SexpReader* reader);

static SexpList* sexp_list_read(SexpReader* reader) {
  SexpList* sexp = sexp_list_new();
  if (!sexp) {
    return NULL;
  }

  assert(reader->input[0] == '(');
  ++reader->input;

  skip_white(reader);
  skip_comment(reader);

  while (reader->input[0] != ')') {
    Sexp* inner = sexp_read_inner(reader);
    if (!inner) {
      sexp_free(&sexp->super);
      return NULL;
    }
    sexp_list_add(sexp, inner);
    skip_white(reader);
    skip_comment(reader);
  }

  /* skip ) */
  ++reader->input;

  return sexp;
}

static Sexp* sexp_read_inner(SexpReader* reader) {

  while (true) {
    skip_white(reader);
    skip_comment(reader);
    switch (*reader->input) {
    case '(':
      return &sexp_list_read(reader)->super;
    default:
      return &sexp_symbol_read(reader)->super;
    }
  }

  return NULL;
}

Sexp* sexp_read(const char* input) {
  return sexp_read_inner(&(SexpReader) { .input = input });
}

Sexp* sexp_clone(const Sexp* sexp) {
  switch (sexp->type) {
  case SEXP_SYMBOL:
    return &sexp_symbol_new(((const SexpSymbol*) sexp)->name)->super;
  case SEXP_LIST: {
    const SexpList* sexp_list = (const SexpList*) sexp;
    SexpList* new_list = sexp_list_new();
    Sexp* element;
    TAILQ_FOREACH(element, &sexp_list->head, entry) {
      sexp_list_add(new_list, sexp_clone(element));
    }
    return &new_list->super;
  }
  }
}

void sexp_print(const Sexp* sexp) {
  switch (sexp->type) {
  case SEXP_SYMBOL:
    printf("%s", ((const SexpSymbol*) sexp)->name);
    break;
  case SEXP_LIST: {
    printf("(");
    const Sexp* element;
    TAILQ_FOREACH(element, &((const SexpList*) sexp)->head, entry) {
      sexp_print(element);
      printf(" ");
    }
    printf(")");
    break;
  }
  }
}

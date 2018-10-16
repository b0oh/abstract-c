#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "scheme.h"
#include "sexp.h"
#include "term.h"

static char* get_content(const char* file_name) {
  FILE* file = fopen(file_name, "r");
  assert(file);

  size_t size = 1024, position = 0;
  char* buffer = malloc(size);
  assert(buffer);

  do {
    size_t to_read = size - position;
    size_t bytes = fread(buffer + position, 1, to_read, file);
    if (bytes < to_read) {
      break;
    }

    position += bytes;

    size *= 2;
    char* tmp = realloc(buffer, size);
    assert(tmp);
    buffer = tmp;
  } while (true);

  return buffer;
}

int main(int argc, const char** argv) {
  (void) argc;

  const char* file_name = argv[1];
  char* content = get_content(file_name);
  const Sexp* sexp = sexp_read(content);
  const Term* term = scheme_extract(sexp);

  const Term* beta = term_full_beta(term);

  term_print(term);
  printf("\n");
  term_print(beta);
  printf("\n");

  sexp_free(sexp);
  term_free(term);
  free(content);

  return 0;
}

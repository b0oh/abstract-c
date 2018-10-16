#pragma once

typedef struct Sexp Sexp;
typedef struct Term Term;

Term* scheme_extract(const Sexp* sexp);

#include "print.h"

typedef union yylv {
  int ival;
  double dval;
  char *sval;
} YYSTYPE;

extern YYSTYPE yylval;
extern printer_t thePrinter;

#include "printdefy.h"


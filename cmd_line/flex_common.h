#ifndef __FLEX_COMMON__
#define __FLEX_COMMON__

#include <stdio.h>
#include "CConfig.h"
#include "stddebug.h"
#include "CInterpreter.h"

typedef void * yyscan_t;

struct pcdata
{
	pCControlAPI pControlAPI;
	pCConfig pConfig;
	EMode nMode;
	int _lineNum;
	int _columnNum;
	yyscan_t scaninfo;
};

int yyerror(struct pcdata *pp, const char *s);
void yyset_in(FILE *in, yyscan_t yyscanner);
void yyset_out(FILE *out, yyscan_t yyscanner);
FILE *yyget_out(yyscan_t yyscanner);
int yylex_init_extra(void *, yyscan_t *);
union YYSTYPE;
int yylex(YYSTYPE *, yyscan_t);
void *yyget_extra(yyscan_t);

#endif /* __FLEX_COMMON__ */

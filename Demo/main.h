
#ifndef _main_h
#define _main_h


void tassert_fail(const char *expression, const char *pfile, unsigned lineno);

#define tassert(expr)	((expr)	? ((void) 0) : tassert_fail (#expr, __FILE__, __LINE__))



#endif

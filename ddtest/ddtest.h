// 
// ddtest.h : Data Dependence Test pass
// 

#ifndef _DDTEST_H
#define _DDTEST_H

#include "ProgramUnit.h"

void ddtest(ProgramUnit & pgm);

void print_dd(bool forArray, bool flow, bool output, bool anti);

#endif

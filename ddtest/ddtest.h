// 
// ddtest.h : Data Dependence Test pass
// 

#ifndef _DDTEST_H
#define _DDTEST_H

#include "ProgramUnit.h"

void ddtest(ProgramUnit & pgm);

void print_dds(bool flow, bool output, bool anti);
void print_dda(bool flow, bool output, bool anti);

#endif

// 
// subexpr.h : constant propagation pass
// 

#ifndef _SUBEXPR_H
#define _SUBEXPR_H

#include "ProgramUnit.h"


class BasicBlock;

void subexpr_elimination(ProgramUnit & pgm,
                         List<BasicBlock> * pgm_basic_blocks);

#endif

// 
// ssa.h : SSA pass
// 

#ifndef _SSA_H
#define _SSA_H

#include "ProgramUnit.h"
#include "SSAWorkSpace.h"
#include "../bblock/BasicBlock.h"

void transform_loops(ProgramUnit& pgm);
void ssa(ProgramUnit & pgm, List<BasicBlock>* basicBlocks);
void dessa(ProgramUnit & pgm);

#endif

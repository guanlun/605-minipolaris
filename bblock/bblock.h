
#ifndef _MINI_POLARIS_bblock_h
#define _MINI_POLARIS_bblock_h

// 
// bblock.h : basic block pass

#include "BasicBlock.h"		// BasicBlock class defined

List<BasicBlock>  * find_basic_blocks(ProgramUnit & pgm);
void		    summarize_basic_blocks(ProgramUnit & pgm,
					   List<BasicBlock> * bbl,
					   ostream &o);

#endif

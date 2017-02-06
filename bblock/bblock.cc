// 
// bblock.cc : basic block routines
//

#include <stdlib.h>
#include <fstream>
#include <strstream>

#include "ProgramUnit.h"
#include "Program.h"
#include "StmtList.h"
#include "Collection/List.h"
#include  "Statement/Statement.h"
#include  "bblock/bblock.h"

// it is necessary to declare these template classes whenever you create
// a new class from a template so that the inherited member functions will
// be created
template class TypedCollection<BasicBlock>;
template class List<BasicBlock>;
template class Iterator<BasicBlock>;
template class Assign<BasicBlock>;
template class RefSet<BasicBlock>;

List<BasicBlock>	  * find_basic_blocks(ProgramUnit & pgm)
{
    String bb_name;
    int bb_number; // The index of basic block
    String pgm_name; // The program unit's name

    List<BasicBlock>* basicBlocks = new List<BasicBlock>();

    // return NULL if pgm.pu_class() == BLOCK_DATA_PU_TYPE or UNKNOWN_PU_TYPE

    //  you will put in here the bulk of the work
    //  be sure to understand the difference between prev_ref() and pred()
    //  and between next_ref() and succ().  Try running polaris with the
    //  switch p_scan set to 2 and examining how these fields are used
    //  with the various statements.

    // somewhere inside, to create the name of the Basic Block
    {
        strstream o;  // notice o is inside a local scope and so it
        // is reinitialized each time

        o << pgm_name << "#" << bb_number++ << '\000';
        char *name = o.str();
        bb_name = name;
        delete name;
    }

    BasicBlock* bb = new BasicBlock();

    basicBlocks->ins_last(bb);

    // (Hint : use the same technique to build the comment
    // you will insert before the first statement of each block,
    // since it requires type String.

    //
    //   :-)
    //


    return basicBlocks;
}


// suggestions for print format added for convenience

void		    summarize_basic_blocks(ProgramUnit& pgm, List<BasicBlock> * bbl, ostream &o)
{
    // this exits the program upon an unexpected condition, bbl == 0 (NULL)
    p_assert(bbl, "summarize_basic_blocks() : null bbl");

    o << "BASIC BLOCK SUMMARY FOR ";

    // TODO
    // print PROGRAM/SUBROUTINE/FUNCTION depending upon type ...
    //    and if not the right type, unconditionally exit
    // p_abort("summarize_basic_blocks() : invalid PU type");

    // print the name of the program unit

    o << "=============================================\n";

    // change X to the number of basic blocks in pgm
    o << "   " << bbl->entries() << " basic blocks total.\n\n";


    //  output every basic block 
    //

    o << "\n";
}

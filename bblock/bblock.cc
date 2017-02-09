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

bool is_end_of_bb(const Statement& currStmt)
{
    if (currStmt.succ().entries() != 1) {
        return true;
    }

    const Statement& onlySuccStmt = currStmt.succ()._element(0);
    const Statement* nextStmt = currStmt.next_ref();

    return (&onlySuccStmt != nextStmt);
}

bool is_begin_of_bb(const Statement& currStmt)
{
    return (currStmt.pred().entries() != 1);
}

List<BasicBlock>	  * find_basic_blocks(ProgramUnit & pgm)
{
    PU_TYPE pgmClass = pgm.pu_class();

    if ((pgmClass == BLOCK_DATA_PU_TYPE) || (pgmClass == UNDEFINED_PU_TYPE)) {
        return NULL;
    }

    String bb_name;
    int bb_number = 0; // The index of basic block
    String pgm_name = pgm.routine_name_ref(); // The program unit's name

    List<BasicBlock>* basicBlocks = new List<BasicBlock>();

    BasicBlock* bb = NULL;

    //  you will put in here the bulk of the work
    //  be sure to understand the difference between prev_ref() and pred()
    //  and between next_ref() and succ().  Try running polaris with the
    //  switch p_scan set to 2 and examining how these fields are used
    //  with the various statements.

    StmtList& statements = pgm.stmts();

    for (Iterator<Statement> iter = statements;
        iter.valid();
        ++iter) {

        Statement& currStmt = iter.current();

        if (is_begin_of_bb(currStmt) || (bb == NULL)) {
            if (bb != NULL) {
                basicBlocks->ins_last(bb);

                bb = NULL;
            }

            strstream o;

            o << pgm_name << "#" << bb_number++ << '\000';
            char *name = o.str();
            bb_name = name;
            delete name;

            bb = new BasicBlock(bb_name);
        }

        bb->add_statement(currStmt);
        currStmt.work_stack().push(new BasicBlockWork(1, bb));

        if (is_end_of_bb(currStmt)) {
            basicBlocks->ins_last(bb);

            bb = NULL;
        }
    }

    for (Iterator<BasicBlock> iter = basicBlocks;
        iter.valid();
        ++iter) {
        BasicBlock& currBlk = iter.current();

        currBlk.insert_bb_comment();
        currBlk.build_pred_succ_relation();
    }

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

    // print PROGRAM/SUBROUTINE/FUNCTION depending upon type ...
    //    and if not the right type, unconditionally exit
    // p_abort("summarize_basic_blocks() : invalid PU type");
    char* programType;

    switch (pgm.pu_class()) {
        case PROGRAM_PU_TYPE:
            programType = "PROGRAM";
            break;
        case SUBROUTINE_PU_TYPE:
            programType = "SUBROUTINE";
            break;
        case FUNCTION_PU_TYPE:
            programType = "FUNCTION";
            break;
        default:
            p_abort("summarize_basic_blocks() : invalid PU type");
            break;
    }

    o << programType << ": ";

    // print the name of the program unit
    o << pgm.routine_name_ref() << "\n";

    o << "=============================================\n";

    // change X to the number of basic blocks in pgm
    o << "   " << bbl->entries() << " basic blocks total.\n\n";

    //  output every basic block 
    //

    for (Iterator<BasicBlock> iter = bbl;
        iter.valid();
        ++iter) {
        BasicBlock& bb = iter.current();

        bb.print(cout);

        o << "\n";
    }
}

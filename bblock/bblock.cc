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
    String bb_name;
    int bb_number; // The index of basic block
    String pgm_name; // The program unit's name

    List<BasicBlock>* basicBlocks = new List<BasicBlock>();

    BasicBlock* bb = NULL;

    // return NULL if pgm.pu_class() == BLOCK_DATA_PU_TYPE or UNKNOWN_PU_TYPE

    //  you will put in here the bulk of the work
    //  be sure to understand the difference between prev_ref() and pred()
    //  and between next_ref() and succ().  Try running polaris with the
    //  switch p_scan set to 2 and examining how these fields are used
    //  with the various statements.

    StmtList& statements = pgm.stmts();
    cout << "----------------------------------------------------------" << endl;
    for (Iterator<Statement> iter = statements;
        iter.valid();
        ++iter) {

        Statement& currStmt = iter.current();

        if (is_begin_of_bb(currStmt)) {
            if (bb != NULL) {
                basicBlocks->ins_last(bb);
                cout << "end: " << bb << *(currStmt.prev_ref()) << endl;
            }

            bb = new BasicBlock();
            cout << "begin: " << bb << currStmt << endl;
        } else if (bb == NULL) {
            cout << "begin: " << bb << currStmt << endl;
            bb = new BasicBlock();
        }

        bb->addStatement(currStmt);

        if (is_end_of_bb(currStmt)) {
            cout << "end: " << bb << currStmt << endl;
            basicBlocks->ins_last(bb);

            bb = NULL;
        }

        /*
        if (currStmt.pred().entries() != 1) {
            Statement* prevStmt = currStmt.prev_ref();
            if (prevStmt != NULL) {
                cout << "end of current bb" << endl;
                cout << *(currStmt.prev_ref()) << endl;
                cout << "-------------------------------" << endl;
            }

            cout << "begining of new bb" << endl;
            cout << currStmt << endl;
            cout << "-------------------------------" << endl;
        // } else {
        //     const Statement& onlyPredStmt = currStmt.pred()._element(0);
        //     const Statement* prevStmt = currStmt.prev_ref();

        //     if (&onlyPredStmt != prevStmt) {
        //         cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
        //     }
        }

        if (currStmt.succ().entries() != 1) {
            cout << "end of current bb - more than one way" << endl;
            cout << currStmt << endl;
            cout << "-------------------------------" << endl;

        } else {
            const Statement& onlySuccStmt = currStmt.succ()._element(0);

            const Statement* nextStmt = currStmt.next_ref();

            if (&onlySuccStmt != nextStmt) {
                cout << "end of current bb - diff" << endl;
                cout << currStmt << endl;
                cout << "-------------------------------" << endl;
            }
        }
        */

    }

    cout << "----------------------------------------------------------" << endl;

    // somewhere inside, to create the name of the Basic Block
    {
        strstream o;  // notice o is inside a local scope and so it
        // is reinitialized each time

        o << pgm_name << "#" << bb_number++ << '\000';
        char *name = o.str();
        bb_name = name;
        delete name;
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

    for (Iterator<BasicBlock> iter = bbl;
        iter.valid();
        ++iter) {
        BasicBlock& bb = iter.current();

        cout << bb.tag << endl;
    }

    o << "\n";
}

#ifndef _MINI_POLARIS_basicblock_h
#define _MINI_POLARIS_basicblock_h



//
// BasicBlock.h :  BasicBlock class type
//

#include <stdlib.h>
#include <fstream>
#include <strstream>

#include "ProgramUnit.h"
#include "Program.h"
#include "StmtList.h"
#include "Collection/List.h"
#include  "Statement/Statement.h"


using namespace std;

class BasicBlock : public Listable
{
    // be sure to define the following virtual functions yourself, since
    // they are inherited from Listable :

    // Duplicate and return a pointer to the new item
    // (MUST be implemented correctly by subclasses
    // for List copy functions to work).
    INLINE Listable *listable_clone() const;

    // print the contents of the Basic Block (used in summary)
    INLINE void print(ostream &o) const;


    // put whatever you need to store the information and be able to access it

private:
    Statement* startStmt;
    Statement* endStmt;

    List<Statement&>* preds;
    List<Statement&>* succs;
};

INLINE Listable *
BasicBlock::listable_clone() const
{
    
}

// one example of a member function :
INLINE void
BasicBlock::print(ostream &o) const
{
    // change X1 to the basic block name
    String X1;
    o << "    Basic Block " << X1 << " :\n";
    // change X2 to the number of statements in block
    int X2;
    o << "      " << X2 << " statements.\n";
    o << "      starts : ";

    // change X3 to a variable containing the first statment in the block
    Statement* X3;
    int indent = 0;
    X3->write(o, indent);
    // change X4 to number of statements
    int X4;
    if (X4 > 1)
    {
        o << "      ends   : ";
        // print last statement
    }
    // change X5 to number of predecessors
    int X5;
    o << "\n      " << X5 << " predecessors : \n";
    int seen = 0;
    // change X6 to number of predecessors
    int X6;
    if (X6 > 0)
    {
        o << "        ";
        // for every predecessor :
        {
            if (seen)
                o << ", ";
            else
                seen++;
            // print name of predecessor
        }
        o << "\n";
    }

    // do the same for successors

}

// you will need many more functions here (or you can put some in BasicBlock.cc
// for example, making sure to mention it in the Makefile in CPPSRCS...)



#endif

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
#include  "StringElem.h"

using namespace std;

class BasicBlock;

class BasicBlockWork : public WorkSpace
{
public:
    BasicBlockWork(BasicBlockWork & other) : _basicBlock(other._basicBlock), WorkSpace(other) { }

    BasicBlockWork(int tag, BasicBlock * BasicBlock) : _basicBlock(BasicBlock), WorkSpace(tag)  { }

    INLINE BasicBlock * get_basic_block() const { return _basicBlock; }

    INLINE Listable *listable_clone() const { return new BasicBlockWork((BasicBlockWork &) *this); }
    // Needed for Listable class.

    INLINE void print(ostream &o) const { o << "BasicBlockWork : [ " << this->_basicBlock << " ]"; }
    // Needed for Listable class.

    INLINE int structures_OK() const { return 1; };

private:
    BasicBlock * _basicBlock;
};

class BasicBlock : public Listable
{
public:
    BasicBlock(const String& name);

    BasicBlock(const BasicBlock& other);

    // be sure to define the following virtual functions yourself, since
    // they are inherited from Listable :

    // Duplicate and return a pointer to the new item
    // (MUST be implemented correctly by subclasses
    // for List copy functions to work).
    INLINE Listable *listable_clone() const;

    // print the contents of the Basic Block (used in summary)
    INLINE void print(ostream &o) const;

    INLINE void add_statement(Statement& statement) { this->stmts.ins_last(statement); }

    void insert_bb_comment();

    void build_pred_succ_relation();

    string name;

private:
    // put whatever you need to store the information and be able to access it

    RefList<Statement&> stmts;

    RefList<BasicBlock&> predecessors;
    RefList<BasicBlock&> successors;
};

BasicBlock::BasicBlock(const String& name)
{
    this->name = name;
}

BasicBlock::BasicBlock(const BasicBlock& other)
: name(other.name), stmts(other.stmts), predecessors(other.predecessors), successors(other.successors), Listable(other)
{
}

INLINE Listable *
BasicBlock::listable_clone() const
{
    return new BasicBlock(*this);
}

void 
BasicBlock::insert_bb_comment()
{
	Statement* firstStmt = &(this->stmts[0]);

	strstream o;
	o << "c     ----------- BASIC BLOCK: " << this->name << '\000';

	firstStmt->pre_directives().ins_last(new StringElem(o.str()));
}

void
BasicBlock::build_pred_succ_relation()
{
    int stmtCount = this->stmts.entries();

    Statement* firstStmt = &(this->stmts[0]);
    Statement* lastStmt = &(this->stmts[stmtCount - 1]);

    for (Iterator<Statement> iter = firstStmt->pred();
        iter.valid();
        ++iter) {
        Statement& predStatement = iter.current();

        BasicBlockWork* bbWork =
            (BasicBlockWork *) predStatement.work_stack().top_ref(0);

        BasicBlock* pred = bbWork->get_basic_block();

        this->predecessors.ins_last(*pred);
    }

    for (Iterator<Statement> iter = lastStmt->succ();
        iter.valid();
        ++iter) {
        Statement& succStatement = iter.current();

        BasicBlockWork* bbWork =
            (BasicBlockWork *) succStatement.work_stack().top_ref(0);

        BasicBlock* succ = bbWork->get_basic_block();

        this->successors.ins_last(*succ);
    }
}

// one example of a member function :
INLINE void
BasicBlock::print(ostream &o) const
{
    o << "    Basic Block " << this->name << " :\n";

    int stmtCount = this->stmts.entries();
    // change X2 to the number of statements in block
    o << "      " << stmtCount << " statements.\n";
    o << "      starts : ";

    Statement* firstStmt = &(this->stmts[0]);
    Statement* lastStmt = &(this->stmts[stmtCount - 1]);

    int indent = 0;
    firstStmt->write(o, indent);

    if (stmtCount > 1)
    {
        o << "      ends   : ";
        // print last statement

        lastStmt->write(o, indent);
    }

    int predCount = this->predecessors.entries();
    o << "\n      " << predCount << " predecessors : \n";
    int seen = 0;
    if (predCount > 0)
    {
        o << "        ";

        for (int bbIdx = 0; bbIdx < predCount; bbIdx++) {
            if (seen)
                o << ", ";
            else
                seen++;

            o << this->predecessors[bbIdx].name;

        }
        o << "\n";
    }

    // do the same for successors
    int succCount = this->successors.entries();
    o << "\n      " << succCount << " successors : \n";
    seen = 0;
    if (succCount > 0)
    {
        o << "        ";

        for (int bbIdx = 0; bbIdx < succCount; bbIdx++) {
            if (seen)
                o << ", ";
            else
                seen++;

            o << this->successors[bbIdx].name;

        }
        o << "\n";
    }
}

// you will need many more functions here (or you can put some in BasicBlock.cc
// for example, making sure to mention it in the Makefile in CPPSRCS...)

#endif

// 
// ssa.cc : SSA transformations
// 

#include "ssa.h"
#include "StmtList.h"
#include "Collection/List.h"
#include "Statement/Statement.h"

int PASS_TAG = 3;

void perStmtOperation(StmtList& stmts, void (*forEachFunction)(Statement&)) {
	for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
		Statement& stmt = stmtIter.current();
		forEachFunction(stmt);
	}
}

void createWorkSpace(Statement& stmt) {
	cout << "creating workspace" << endl;
	stmt.work_stack().push(new SSAWorkSpace(PASS_TAG));
}

void computeDominance(ProgramUnit& pgm) {
	StmtList& stmts = pgm.stmts();
	perStmtOperation(stmts, createWorkSpace);
}

void ssa(ProgramUnit & pgm)
{
	computeDominance(pgm);
}

void dessa(ProgramUnit & pgm)
{

}

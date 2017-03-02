// 
// ssa.cc : SSA transformations
// 

#include "ssa.h"
#include "StmtList.h"
#include "Collection/List.h"
#include "Statement/Statement.h"

#include <queue>

int PASS_TAG = 3;

void per_stmt_operation(StmtList& stmts, void (*forEachFunction)(Statement&)) {
	for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
		Statement& stmt = stmtIter.current();
		forEachFunction(stmt);
	}
}

void create_workspace(Statement& stmt) {
	cout << "creating workspace" << endl;
	stmt.work_stack().push(new SSAWorkSpace(PASS_TAG));
}

template <class T>
void set_intersect(set<T>& s1, set<T>& s2) {

}

template <class T>
bool set_equal(set<T>& s1, set<T>& s2) {
	if (s1.size() != s2.size()) {
		return false;
	}

	for (typename set<T>::iterator it = s1.begin(); it != s1.end(); ++it) {
		if (s2.count(*it) == 0) {
			return false;
		}
	}

	return true;
}

void compute_dominance(ProgramUnit& pgm) {
	StmtList& stmts = pgm.stmts();
	per_stmt_operation(stmts, create_workspace);

	for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
		Statement& stmt = stmtIter.current();
		SSAWorkSpace* ssaWS = (SSAWorkSpace*)stmt.work_stack().top_ref(PASS_TAG);

		for (Iterator<Statement> domIter = stmts; domIter.valid(); ++domIter) {
			Statement& initDominator = domIter.current();
			ssaWS->dominators.insert(&initDominator);
		}
	}

	queue<Statement*> workList;
	workList.push(&stmts[0]);

	while (!workList.empty()) {
		Statement* currNode = workList.back();
		set<Statement*> currDominators = ((SSAWorkSpace*)currNode->work_stack().top_ref(PASS_TAG))->dominators;
		workList.pop();

		cout << "work list is not empty" << endl;

		set<Statement*> newDominators;

		int predIdx = 0;
		for (Iterator<Statement> predIter = currNode->pred(); predIter.valid(); ++predIter) {
			Statement& predStmt = predIter.current();
			set<Statement*>& predDominators = ((SSAWorkSpace*)predStmt.work_stack().top_ref(PASS_TAG))->dominators;

			if (predIdx == 0) {
				newDominators = predDominators;
			} else {
				set_intersect(newDominators, predDominators);
			}
			predIdx++;
		}

		newDominators.insert(currNode);

		if (!set_equal(newDominators, currDominators)) {
			((SSAWorkSpace*)currNode->work_stack().top_ref(PASS_TAG))->dominators = newDominators;

			for (Iterator<Statement> succIter = currNode->succ(); succIter.valid(); ++succIter) {
				Statement& succStmt = succIter.current();

				workList.push(&succStmt);
			}
		}
	}
}

void ssa(ProgramUnit & pgm)
{
	compute_dominance(pgm);
}

void dessa(ProgramUnit & pgm)
{

}

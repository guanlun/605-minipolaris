#include "subexpr.h"

int PASS_TAG = 4;

template <class T>
void set_intersect(set<T>& s1, set<T>& s2) {
    for (typename set<T>::iterator it = s1.begin(); it != s1.end(); ++it) {
        if (s2.count(*it) == 0) {
            s1.erase(it);
        }
    }
}

SubExprWorkspace* get_workspace(Statement& stmt) {
	WorkSpace* ws = stmt.work_stack().top_ref(PASS_TAG);

	if (ws == NULL) {
		ws = new SubExprWorkspace(PASS_TAG);
		stmt.work_stack().push(ws);
	}

	return (SubExprWorkspace*) ws;
}

void binarify_operations(ProgramUnit& pgm) {
	StmtList& stmts = pgm.stmts();

	for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
		Statement& stmt = stmtIter.current();


	}
}

void calculate_in_out_sets(ProgramUnit& pgm) {
	StmtList& stmts = pgm.stmts();

	bool changed = false;

	// Init the out sets
	bool isEntryStmt = true;

	for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
		Statement& stmt = stmtIter.current();

		SubExprWorkspace* workspace = get_workspace(stmt);

		if (isEntryStmt) {
			isEntryStmt = false;
		} else {
			for (Iterator<Statement> outSetStmtIter = stmts; outSetStmtIter.valid(); ++outSetStmtIter) {
				Statement& outSetStmt = outSetStmtIter.current();

				workspace->outSet.insert(&outSetStmt);
			}
		}
	}

	for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
		Statement& stmt = stmtIter.current();
		SubExprWorkspace* workspace = get_workspace(stmt);

		int predIdx = 0;

		for (Iterator<Statement> predIter = stmt.pred(); predIter.valid(); ++predIter) {
			Statement& predStmt = predIter.current();

			SubExprWorkspace* predWS = get_workspace(predStmt);

			if (predIdx == 0) {
				workspace->inSet = predWS->outSet;
			} else {
				set_intersect(workspace->inSet, predWS->outSet);
			}

			predIdx++;
		}

		cout << stmt << endl;
		for (Mutator<Expression> exprIter = stmt.iterate_expressions(); exprIter.valid(); ++exprIter) {
			Expression& expr = exprIter.current();

			cout << expr << endl;
		}
	}
}

void subexpr_elimination(ProgramUnit& pgm,
                         List<BasicBlock> * pgm_basic_blocks) {

	binarify_operations(pgm);

	calculate_in_out_sets(pgm);
}



// 
// ssa.cc : SSA transformations
// 

#include "ssa.h"
#include "StmtList.h"
#include "Collection/List.h"
#include "Statement/Statement.h"

#include <queue>
#include <vector>
#include <map>

int PASS_TAG = 3;

void per_stmt_operation(Iterator<Statement> stmts, void (*forEachFunction)(Statement&)) {
	for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
		Statement& stmt = stmtIter.current();
		forEachFunction(stmt);
	}
}

void create_workspace(Statement& stmt) {
	stmt.work_stack().push(new SSAWorkSpace(PASS_TAG));
}

inline SSAWorkSpace* get_ssa_ws(Statement& stmt) {
	return (SSAWorkSpace*)stmt.work_stack().top_ref(PASS_TAG);
}

template <class T>
void set_intersect(set<T>& s1, set<T>& s2) {
	for (typename set<T>::iterator it = s1.begin(); it != s1.end(); ++it) {
		if (s2.count(*it) == 0) {
			s1.erase(it);
		}
	}
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

void link_dominants(Statement& stmt) {
	SSAWorkSpace* ws = get_ssa_ws(stmt);
	for (set<Statement*>::iterator it = ws->dominators.begin();
		it != ws->dominators.end();
		++it) {
		Statement* dominatorStmt = *it;

		SSAWorkSpace* dominatorWorkSpace = get_ssa_ws(*dominatorStmt);
		dominatorWorkSpace->dominants.insert(&stmt);
	}
}

void find_immediate_dominator(Statement& stmt) {
	SSAWorkSpace* ws = get_ssa_ws(stmt);
	Statement* runner = &stmt;

	while (runner != NULL) {
		const RefSet<Statement>& predStmts = runner->pred();
		if (predStmts.empty()) {
			break;
		}

		Iterator<Statement> firstPredIter = predStmts;
		runner = &firstPredIter.current();

		if (ws->dominators.count(runner) > 0) {
			break;
		}
	}

	ws->immediateDominator = runner;
}

void find_dominance_frontier(Statement& stmt) {
	SSAWorkSpace* ws = get_ssa_ws(stmt);
	const RefSet<Statement>& predStmts = stmt.pred();

	if (predStmts.entries() >= 2) {
		// Only a merge-point could be a dominance frontier of other nodes
		for (Iterator<Statement> predIter = predStmts; predIter.valid(); ++predIter) {
			Statement& predStmt = predIter.current();

			Statement* runner = &predStmt;

			while (runner != ws->immediateDominator) {
				SSAWorkSpace* runnerWS = get_ssa_ws(*runner);
				runnerWS->dominanceFrontiers.insert(&stmt);

				runner = runnerWS->immediateDominator;
			}
		}
	}
}

void find_phi_insertion_points(Statement& stmt) {
	// TODO: maybe change this to out_refs?
	Expression& assignedExpr = stmt.lhs();
	Symbol& assignedSymbol = assignedExpr.symbol();

	SSAWorkSpace* ws = get_ssa_ws(stmt);

	for (set<Statement*>::iterator dfIter = ws->dominanceFrontiers.begin();
		dfIter != ws->dominanceFrontiers.end();
		++dfIter) {
		Statement* dfStmt = *dfIter;
		SSAWorkSpace* dfWorkSpace = get_ssa_ws(*dfStmt);

		dfWorkSpace->phiSymbols.insert(&assignedSymbol);
	}
}

void insert_phi_stmts(Statement& stmt) {
	SSAWorkSpace* ws = get_ssa_ws(stmt);

	for (set<Symbol*>::iterator phiIter = ws->phiSymbols.begin();
		phiIter != ws->phiSymbols.end();
		++phiIter) {

	}
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
		Statement* currNode = workList.front();
		set<Statement*> currDominators = ((SSAWorkSpace*)currNode->work_stack().top_ref(PASS_TAG))->dominators;
		workList.pop();

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

	per_stmt_operation(stmts, link_dominants);
	per_stmt_operation(stmts, find_immediate_dominator);
	per_stmt_operation(stmts, find_dominance_frontier);
}

void generate_phi_stmts(ProgramUnit& pgm) {
	StmtList& stmts = pgm.stmts();

//	per_stmt_operation(stmts.stmts_of_type(ASSIGNMENT_STMT), find_phi_insertion_points);

	for (DictionaryIter<Symbol> symIter = pgm.symtab().iterator(); symIter.valid(); ++symIter) {
		Symbol& sym = symIter.current();
		set<Statement*> workList;

		for (Iterator<Statement> stmtIter = stmts.stmts_of_type(ASSIGNMENT_STMT); stmtIter.valid(); ++stmtIter) {
			Statement& stmt = stmtIter.current();

			Symbol& assignee = stmt.lhs().symbol();

			if (&assignee == &sym) {
				workList.insert(&stmt);
			}
		}

		while (!workList.empty()) {
			Statement* currNode = *workList.begin();
			workList.erase(currNode);

			set<Statement*> dfNodes = ((SSAWorkSpace*)currNode->work_stack().top_ref(PASS_TAG))->dominanceFrontiers;

			for (set<Statement*>::iterator dfIter = dfNodes.begin(); dfIter != dfNodes.end(); ++dfIter) {
				Statement* dfNode = *dfIter;

				SSAWorkSpace* dfNodeWS = get_ssa_ws(*dfNode);

				dfNodeWS->phiSymbols.insert(&sym);
				workList.insert(dfNode);

//				cout << sym.tag_ref() << " is added to " << dfNode->tag() << endl;
			}
		}
	}

	per_stmt_operation(stmts, insert_phi_stmts);
}

void variable_renaming_helper(Statement* stmt, map<Symbol*, vector<int> >& variableNumLookup, set<Statement*>& visited) {
	if (visited.count(stmt) > 0) {
		return;
	}

	cout << "visiting: " << stmt->tag() << " " << stmt << endl;

	SSAWorkSpace* ws = get_ssa_ws(*stmt);

	for (Iterator<Expression> inRefIter = stmt->in_refs(); inRefIter.valid(); ++inRefIter) {
		Expression& inRefExpr = inRefIter.current();

		if (inRefExpr.op() != ID_OP) {
			continue;
		}

		Symbol& inRefSymbol = inRefExpr.symbol();
		map<Symbol*, vector<int> >::iterator vnIter = variableNumLookup.find(&inRefSymbol);
	}

	// Look at the new definitions and rename any redefined variables
	for (Iterator<Expression> outRefIter = stmt->out_refs(); outRefIter.valid(); ++outRefIter) {
		Expression& outRefExpr = outRefIter.current();

		if (outRefExpr.op() != ID_OP) {
			continue;
		}

		Symbol& outRefSymbol = outRefExpr.symbol();
		map<Symbol*, vector<int> >::iterator vnIter = variableNumLookup.find(&outRefSymbol);

		vector<int>& numStack = vnIter->second;
		numStack.push_back(numStack.back() + 1);
	}

	visited.insert(stmt);

	// Recursively visit all the dominants
	for (set<Statement*>::iterator dominantIter = ws->dominants.begin();
		dominantIter != ws->dominants.end();
		++dominantIter) {

		Statement* dominantStmt = *dominantIter;
		if (dominantStmt == stmt) {
			continue;
		}

		variable_renaming_helper(dominantStmt, variableNumLookup, visited);
	}
}

void rename_variables(ProgramUnit& pgm) {
	StmtList& assignmentStmts = pgm.stmts();

	set<Statement*> visitedStmts;
	map<Symbol*, vector<int> > variableNumLookup;

	for (DictionaryIter<Symbol> symIter = pgm.symtab().iterator(); symIter.valid(); ++symIter) {
		Symbol& symbol = symIter.current();

		vector<int> numStack;
		numStack.push_back(1);

		variableNumLookup.insert(pair<Symbol*, vector<int> >(&symbol, numStack));
	}

	variable_renaming_helper(&assignmentStmts[0], variableNumLookup, visitedStmts);
}

void ssa(ProgramUnit & pgm)
{
	compute_dominance(pgm);
	generate_phi_stmts(pgm);
	rename_variables(pgm);
}

void dessa(ProgramUnit & pgm)
{

}

// 
// ssa.cc : SSA transformations
// 

#include "ssa.h"
#include "StmtList.h"
#include "Collection/List.h"
#include "Statement/Statement.h"
#include "Statement/AssignmentStmt.h"
#include "Expression/FunctionCallExpr.h"
#include "Expression/IDExpr.h"
#include "Expression/expr_funcs.h"

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
	Statement* idomStmt = ws->immediateDominator;

	if (!idomStmt) {
		return;
	}

	SSAWorkSpace* idomWS = get_ssa_ws(*idomStmt);

	idomWS->dominants.insert(&stmt);

	/*
	for (set<Statement*>::iterator it = ws->dominators.begin();
		it != ws->dominators.end();
		++it) {
		Statement* dominatorStmt = *it;

		SSAWorkSpace* dominatorWorkSpace = get_ssa_ws(*dominatorStmt);
		dominatorWorkSpace->dominants.insert(&stmt);
	}
	*/
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
//	Expression& assignedExpr = stmt.lhs();
	Iterator<Expression> outIter = stmt.out_refs();
	Expression& outExpr = outIter.current();
	Symbol& assignedSymbol = outExpr.symbol();

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

//		new_function("PHI", make_type(REAL_TYPE, 8), pgm);
//		FunctionCallExpr* phiExpr = new FunctionCallExpr();
//		Statement* phiFunctionStmt = new
	}
}

void compute_dominance(ProgramUnit& pgm, List<BasicBlock>* basicBlocks) {
	for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
		BasicBlock& bb = bbIter.current();

		for (Iterator<BasicBlock> domIter = *basicBlocks; domIter.valid(); ++domIter) {
			BasicBlock& initBB = domIter.current();

			bb.dominators.insert(&initBB);
		}
	}

	queue<BasicBlock*> workList;
	workList.push(basicBlocks->first_ref());

	while (!workList.empty()) {
		BasicBlock* currNode = workList.front();
		workList.pop();

		set<BasicBlock*> newDominators;

		for (int predIdx = 0; predIdx < currNode->predecessors.entries(); predIdx++) {
			BasicBlock& predBB = currNode->predecessors[predIdx];

			if (predIdx == 0) {
				newDominators = predBB.dominators;
			} else {
				set_intersect(newDominators, predBB.dominators);
			}

			predIdx++;
		}

		newDominators.insert(currNode);

		if (!set_equal(newDominators, currNode->dominators)) {
			currNode->dominators = newDominators;

			for (int succIdx = 0; succIdx < currNode->successors.entries(); succIdx++) {
				BasicBlock& succBB = currNode->successors[succIdx];

				workList.push(&succBB);
			}
		}
	}

	for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
		BasicBlock& bb = bbIter.current();

		BasicBlock* runner = &bb;
		while (runner != NULL) {
			RefList<BasicBlock&>& predBBs = runner->predecessors;
			if (predBBs.entries() == 0) {
				break;
			}

			runner = &predBBs[0];

			if (bb.dominators.count(runner) > 0) {
				break;
			}
		}

		bb.immediateDominator = runner;
	}

	for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
		BasicBlock& bb = bbIter.current();

		RefList<BasicBlock&>& predBBs = bb.predecessors;

		if (predBBs.entries() >= 2) {
			// Only a merge-point could be a dominance frontier of other nodes

			for (int predIdx = 0; predIdx < predBBs.entries(); predIdx++) {
				BasicBlock& predBB = predBBs[predIdx];

				BasicBlock* runner = &predBB;

				while (runner != bb.immediateDominator) {
					runner->dominanceFrontiers.insert(&bb);
					runner = runner->immediateDominator;
				}
			}
		}
	}

	for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
		BasicBlock& bb = bbIter.current();

		if (bb.immediateDominator) {
			bb.immediateDominator->dominants.insert(&bb);
		}
	}

	for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
		BasicBlock& bb = bbIter.current();

		cout << bb << endl;
	}
}

void generate_phi_stmts(ProgramUnit& pgm) {
	StmtList& stmts = pgm.stmts();

//	per_stmt_operation(stmts.stmts_of_type(ASSIGNMENT_STMT), find_phi_insertion_points);

	for (DictionaryIter<Symbol> symIter = pgm.symtab().iterator(); symIter.valid(); ++symIter) {
		Symbol& sym = symIter.current();
		set<Statement*> workList;
		set<Statement*> added;

		for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
			Statement& stmt = stmtIter.current();

			Iterator<Expression> outIter = stmt.out_refs();
			if (!outIter.valid()) {
				continue;
			}

			Expression& outExpr = outIter.current();
			Symbol& assignee = outExpr.symbol();

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

				if (added.count(dfNode) == 0) {
					workList.insert(dfNode);
					added.insert(dfNode);
				}

				cout << sym.tag_ref() << " is added to " << dfNode->tag() << endl;
			}
		}
	}

	Expression* phiFunc = new_function("PHI", make_type(INTEGER_TYPE, 4), pgm);

//	per_stmt_operation(stmts, insert_phi_stmts);
	for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
		cout << "still iterating" << endl;
		Statement& stmt = stmtIter.current();
		cout << stmt << endl;

		SSAWorkSpace* ws = get_ssa_ws(stmt);

		for (set<Symbol*>::iterator phiIter = ws->phiSymbols.begin();
			phiIter != ws->phiSymbols.end();
			++phiIter) {
			Symbol* phiSymbol = *phiIter;

			Expression* args = comma();
			Expression* phiFuncExpr = function_call(phiFunc->clone(), args);
			Expression* assignedExpr = new IDExpr(phiSymbol->type(), *phiSymbol);

			Statement* phiStmt = new AssignmentStmt(stmts.new_tag(), assignedExpr, phiFuncExpr);
			create_workspace(*phiStmt);

			stmts.ins_after(phiStmt, &stmt);
		}
	}
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

void ssa(ProgramUnit & pgm, List<BasicBlock>* basicBlocks)
{
	compute_dominance(pgm, basicBlocks);
//	generate_phi_stmts(pgm);
//	rename_variables(pgm);
}

void dessa(ProgramUnit & pgm)
{

}

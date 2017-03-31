#include <map>
#include <vector>
#include <sstream>
#include "Statement/AssignmentStmt.h"
#include "Expression/BinaryExpr.h"
#include "Expression/NonBinaryExpr.h"

#include "subexpr.h"

int PASS_TAG = 4;

int variableNameNum = 1;

map<Expression*, Statement*> exprStmtLookup;

const char* new_temp_variable_name() {
	stringstream ss;

	ss << "_t" << variableNameNum++;

	return ss.str().c_str();
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
void set_union(set<T>& s1, set<T>& s2) {
	for (typename set<T>::iterator it = s2.begin(); it != s2.end(); ++it) {
		if (s1.count(*it) == 0) {
			s1.insert(it);
		}
	}
}

bool expr_eq(Expression& e1, Expression& e2) {
	// TODO: different cases
	return (e1 == e2);
}

SubExprWorkspace* get_workspace(Statement& stmt) {
	WorkSpace* ws = stmt.work_stack().top_ref(PASS_TAG);

	if (ws == NULL) {
		ws = new SubExprWorkspace(PASS_TAG);
		stmt.work_stack().push(ws);
	}

	return (SubExprWorkspace*) ws;
}

bool is_targeted_unary_expr(Expression& expr) {
	switch (expr.op()) {
	case NOT_OP:
		return true;
	default:
		return false;
	}
}

bool is_targeted_binary_expr(Expression& expr) {
	switch (expr.op()) {
	case EQ_OP:
	case NE_OP:
	case LT_OP:
	case LE_OP:
	case GT_OP:
	case GE_OP:
	case SUB_OP:
	case DIV_OP:
	case INTDIV_OP:
	case RATDIV_OP:
	case EXP_OP:
	case CONCAT_OP:
	case ADD_OP:
	case MULT_OP:
	case OR_OP:
	case AND_OP:
	case EQV_OP:
	case NEQV_OP:
		return true;
	default:
		return false;
	}
}

Expression* replace_expression(Expression* expr, Expression* oldExpr, Expression* newExpr) {
	if (*expr == *oldExpr) {
		return newExpr->clone();
	} else {
		for (Mutator<Expression> exprMut = expr->arg_list(); exprMut.valid(); ++exprMut) {
			exprMut.assign() = replace_expression(exprMut.pull(), oldExpr, newExpr);
		}
	}

	return expr;
}

void replace_expression_in_stmt(Statement* stmt, Expression* oldExpr, Expression* newExpr) {
	for (Mutator<Expression> exprIter = stmt->iterate_expressions(); exprIter.valid(); ++exprIter) {
		Expression& expr = exprIter.current();
		exprIter.assign() = replace_expression(&expr, oldExpr, newExpr);
	}

	stmt->build_refs();
}

Expression* binary_conversion_helper(Expression& expr, vector<Statement*>& intermediateStmts, ProgramUnit& pgm) {
	bool allSingle = true;

	vector<Expression*> newExprs;

	for (Iterator<Expression> argIter = expr.arg_list(); argIter.valid(); ++argIter) {
		Expression& argExpr = argIter.current();

		if (argExpr.op() != ID_OP) {
			// TODO: should handle other cases (array, etc.)

			allSingle = false;

//			cout << "arg: " << argExpr << " is not id" << endl;

			Expression* intermediateExpr = binary_conversion_helper(argExpr, intermediateStmts, pgm);
			newExprs.push_back(intermediateExpr);

		} else {
			newExprs.push_back(&argExpr);
		}
	}

//	cout << "--------------------------------" << endl;
//	cout << "For expr " << expr << endl;
//	for (vector<Expression*>::iterator it = newExprs.begin(); it != newExprs.end(); ++it) {
//		cout << **it << " ";
//	}
//	cout << endl << "--------------------------------" << endl;

	Expression* preComputedExpr = new_variable(new_temp_variable_name(), expr.type(), pgm);

	OP_TYPE op = expr.op();

	Expression* op1 = newExprs[0];
	Expression* op2 = newExprs[1];

	// TODO
	Expression* newExpr = add(op1->clone(), op2->clone());

//	Expression* newExpr = new NonBinaryExpr(
//		ADD_OP,
//		expr_type(ADD_OP, op1->type(), op2->type()),
//		op1->clone(),
//		op2->clone()
//	);

	AssignmentStmt* intermediateStmt = new AssignmentStmt(pgm.stmts().new_tag(), preComputedExpr, newExpr);

	intermediateStmts.push_back(intermediateStmt);

	return preComputedExpr;
}

void convert_to_binary_operation(Statement& stmt, ProgramUnit& pgm) {
	cout << stmt << endl;

	StmtList& stmts = pgm.stmts();

	for (Iterator<Expression> exprIter = stmt.iterate_expressions(); exprIter.valid(); ++exprIter) {
		Expression& expr = exprIter.current();

		if (is_targeted_binary_expr(expr)) {
			vector<Statement*> intermediateStmts;

			binary_conversion_helper(expr, intermediateStmts, pgm);

			for (vector<Statement*>::iterator intermediateIter = intermediateStmts.begin();
				intermediateIter != intermediateStmts.end();
				++intermediateIter) {
				Statement* intermediateStmt = *intermediateIter;

				cout << "inserting..." << endl;
				cout << *intermediateStmt << endl;

				stmts.ins_before(intermediateStmt, &stmt);
			}
		}
	}
}

void binarify_operations(ProgramUnit& pgm) {
	StmtList& stmts = pgm.stmts();

	for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
		Statement& stmt = stmtIter.current();

		convert_to_binary_operation(stmt, pgm);
	}
}

set<Expression*>& get_targeted_exprs(StmtList& stmts) {
	set<Expression*> exprs;

	for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
		Statement& stmt = stmtIter.current();

		for (Iterator<Expression> exprIter = stmt.iterate_expressions(); exprIter.valid(); ++exprIter) {
			Expression& expr = exprIter.current();

			// Here we assume all the expressions have been converted to binary operations.
			if (is_targeted_binary_expr(expr)) {
				exprs.insert(&expr);
				exprStmtLookup[&expr] = &stmt;

				SubExprWorkspace* workspace = get_workspace(stmt);

				workspace->targetExpr = &expr;
			}
		}
	}

//	for (map<Expression*, Statement*>::iterator it = exprStmtLookup.begin(); it != exprStmtLookup.end(); ++it) {
//		cout << "entry: " << *it->first << ": " << it->second->tag() << endl;
//	}

	return exprs;
}

void calculate_in_out_sets(ProgramUnit& pgm) {
	StmtList& stmts = pgm.stmts();

	bool changed = false;

	// Init the out sets
	bool isEntryStmt = true;

	set<Expression*> allTargetedExprs = get_targeted_exprs(stmts);

	for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
		Statement& stmt = stmtIter.current();

		SubExprWorkspace* workspace = get_workspace(stmt);

		if (isEntryStmt) {
			isEntryStmt = false;
		} else {
			for (Iterator<Statement> outSetStmtIter = stmts; outSetStmtIter.valid(); ++outSetStmtIter) {
				Statement& outSetStmt = outSetStmtIter.current();

				workspace->outSet = allTargetedExprs;
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

		set<Expression*> resultSet = workspace->inSet;

		// Calculate gen set
		for (Iterator<Expression> exprIter = stmt.iterate_expressions(); exprIter.valid(); ++exprIter) {
			Expression& expr = exprIter.current();

			// Here we assume all the expressions have been converted to binary operations.
			if (is_targeted_binary_expr(expr)) {
				resultSet.insert(&expr);
			}
		}

//		for (Iterator<Expression> outRefIter = stmt.out_refs(); outRefIter.valid(); ++outRefIter) {
//			Expression& outRefExpr = outRefIter.current();
//
//			cout << "out ref: " << outRefExpr << endl;
//
//			for (set<Expression*>::iterator currSetIter = resultSet.begin(); currSetIter != resultSet.end(); ++currSetIter) {
//				Expression* currSetExpr = *currSetIter;
//
//				cout << currSetExpr->arg_list() << endl;
//
//				// Check whether the definition would kill any expressions by comparing it to each operand
//				for (Iterator<Expression> argIter = currSetExpr->arg_list(); argIter.valid(); ++argIter) {
//					Expression& argExpr = argIter.current();
//
//					if (*currSetExpr == argExpr) {
//						cout << "found same!!!!!!!!!!!!!! " << argExpr << endl;
//					}
//				}
//			}
//		}

		workspace->outSet = resultSet;
	}
}

void eliminate_common_subexpr_in_stmt(Statement& defStmt, ProgramUnit& pgm) {
	StmtList& stmts = pgm.stmts();

	SubExprWorkspace* defStmtWorkspace = get_workspace(defStmt);

	Expression* targetExpr = defStmtWorkspace->targetExpr;

	if (targetExpr == NULL) {
		// Current statement does not have a target expression for elmination
		return;
	}

	vector<Statement*> stmtsWithCommonSubExpr;

	for (Iterator<Statement> useIter = stmts; useIter.valid(); ++useIter) {
		Statement& useStmt = useIter.current();

		if (&defStmt == &useStmt) {
			continue;
		}

		SubExprWorkspace* useStmtWorkspace = get_workspace(useStmt);

		// Check if the target expression in the two statements match
		Expression* candidateExpr = useStmtWorkspace->targetExpr;
		if ((candidateExpr == NULL) || !expr_eq(*targetExpr, *candidateExpr)) {
			continue;
		}

		// Check if the target expression defined previously is available here
		set<Expression*>& availExprSet = useStmtWorkspace->inSet;
		if (availExprSet.find(targetExpr) == availExprSet.end()) {
			continue;
		}
		stmtsWithCommonSubExpr.push_back(&useStmt);
	}

	if (!stmtsWithCommonSubExpr.empty()) {
		Expression* preComputedExpr = new_variable(new_temp_variable_name(), targetExpr->type(), pgm);
		Expression* rhs = targetExpr->clone();

		Statement* preComputedStmt = new AssignmentStmt(stmts.new_tag(), preComputedExpr, rhs);
		stmts.ins_before(preComputedStmt, &defStmt);
		replace_expression_in_stmt(&defStmt, defStmtWorkspace->targetExpr, preComputedExpr);

		for (vector<Statement*>::iterator stmtIter = stmtsWithCommonSubExpr.begin();
			stmtIter != stmtsWithCommonSubExpr.end();
			++stmtIter) {
			Statement* stmtWithCommonSubExpr = *stmtIter;
			SubExprWorkspace* ws = get_workspace(*stmtWithCommonSubExpr);

			replace_expression_in_stmt(stmtWithCommonSubExpr, ws->targetExpr, preComputedExpr);

			ws->targetExpr = NULL;
		}
	}
}

void eliminate_common_subexpr(ProgramUnit& pgm) {
	StmtList& stmts = pgm.stmts();

	for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
		Statement& stmt = stmtIter.current();

		eliminate_common_subexpr_in_stmt(stmt, pgm);
	}
}

void subexpr_elimination(ProgramUnit& pgm,
                         List<BasicBlock> * pgm_basic_blocks) {

	binarify_operations(pgm);

	calculate_in_out_sets(pgm);

	eliminate_common_subexpr(pgm);
}

#include <map>
#include <vector>
#include <sstream>
#include "Statement/AssignmentStmt.h"
#include "Expression/UnaryExpr.h"
#include "Expression/BinaryExpr.h"
#include "Expression/NonBinaryExpr.h"
#include "Symbol/VariableSymbol.h"

#include "subexpr.h"

int PASS_TAG = 4;

int variableNameNum = 1;

map<string, set<Statement*>* > exprStmtLookup;

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

bool expr_eq(Expression& e1, Expression& e2) {
	// TODO: different cases
	return (e1 == e2);
}

SubExprWorkspace* get_workspace(Statement& stmt) {
	SubExprWorkspace* ws = (SubExprWorkspace*)stmt.work_stack().top_ref(6);

	if (ws == NULL) {
		ws = new SubExprWorkspace(6);
		stmt.work_stack().push(ws);
	} else {
//		cout << "workspace for " << stmt.tag() << " already exists" << endl;
//		cout << ws << "   " << stmt.work_stack().entries() << endl;
	}

	return ws;
}

bool is_targeted_expr(Expression& expr) {
	// TODO: more unary operations
	switch (expr.op()) {
	case NOT_OP:
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

Expression* create_intermediate_expr(OP_TYPE op, Expression* op1, Expression* op2 = NULL) {
	switch (op) {
	case NOT_OP:
		return new UnaryExpr(
			op,
			op1->type(),
			op1->clone()
		);
	case CONCAT_OP:
	case COLON_OP:
	case ADD_OP:
	case MULT_OP:
	case OR_OP:
	case AND_OP:
	case EQV_OP:
	case NEQV_OP:
		return new NonBinaryExpr(
			op,
			expr_type(op, op1->type(), op2->type()),
			op1->clone(),
			op2->clone()
		);

	default:
		return new BinaryExpr(
			op,
			expr_type(op, op1->type(), op2->type()),
			op1->clone(),
			op2->clone()
		);
	}
}

Expression* binary_conversion_helper(
		Expression& expr,
		vector<Statement*>& intermediateStmts,
		ProgramUnit& pgm,
		bool isTopLevel = false
	) {
	int argCount = expr.arg_list().entries();

	vector<Expression*> newExprs;

	bool hasNestedExpr = false;

	for (Iterator<Expression> argIter = expr.arg_list(); argIter.valid(); ++argIter) {
		Expression& argExpr = argIter.current();

		if (argExpr.arg_list().entries() > 1) {
			// Nested expression
			hasNestedExpr = true;

			Expression* intermediateExpr = binary_conversion_helper(argExpr, intermediateStmts, pgm);
			newExprs.push_back(intermediateExpr);
		} else {
			// Single expression
			newExprs.push_back(&argExpr);
		}
	}

//	cout << expr << " is simple? " << ((argCount <= 2) && !hasNestedExpr) << endl;
//	cout << argCount << " " << hasNestedExpr << endl;
//	cout << "top level?" << isTopLevel << endl;

	if (isTopLevel && (argCount <= 2) && !hasNestedExpr) {
//		cout << "too simple " << expr << endl;
		return &expr;
	}

	Expression* preComputedExpr = new_variable(new_temp_variable_name(), expr.type(), pgm);

	int operatorCount = newExprs.size();

	Expression* newExpr;
	if (operatorCount == 1) {
		// Unary operation
		newExpr = create_intermediate_expr(expr.op(), newExprs[0]);
	} else {
		// Binary operation
		newExpr = create_intermediate_expr(expr.op(), newExprs[0], newExprs[1]);
	}

	AssignmentStmt* intermediateStmt = new AssignmentStmt(pgm.stmts().new_tag(), preComputedExpr, newExpr);
	intermediateStmts.push_back(intermediateStmt);

	if (operatorCount > 2) {
		// More than 2 operands, create intermediate statements iteratively
		Expression* lastPreComputedExpr = preComputedExpr;

		for (int idx = 2; idx < newExprs.size(); idx++) {
			preComputedExpr = new_variable(new_temp_variable_name(), expr.type(), pgm);
			Expression* newExpr = create_intermediate_expr(expr.op(), lastPreComputedExpr->clone(), newExprs[idx]);

			AssignmentStmt* intermediateStmt = new AssignmentStmt(pgm.stmts().new_tag(), preComputedExpr, newExpr);
			intermediateStmts.push_back(intermediateStmt);
		}
	}

	return preComputedExpr;
}

void convert_to_binary_operation(Statement& stmt, ProgramUnit& pgm) {
	StmtList& stmts = pgm.stmts();

	for (Iterator<Expression> exprIter = stmt.iterate_expressions(); exprIter.valid(); ++exprIter) {
		Expression& expr = exprIter.current();

		if (is_targeted_expr(expr)) {
			vector<Statement*> intermediateStmts;

			Expression* lastIntermediateExpr = binary_conversion_helper(expr, intermediateStmts, pgm, true);

			for (vector<Statement*>::iterator intermediateIter = intermediateStmts.begin();
				intermediateIter != intermediateStmts.end();
				++intermediateIter) {
				Statement* intermediateStmt = *intermediateIter;

				stmts.ins_before(intermediateStmt, &stmt);
			}

			replace_expression_in_stmt(&stmt, &expr, lastIntermediateExpr);
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

set<Expression*> compute_targeted_exprs(StmtList& stmts) {
	set<Expression*> exprs;

	for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
		Statement& stmt = stmtIter.current();

		for (Iterator<Expression> exprIter = stmt.iterate_expressions(); exprIter.valid(); ++exprIter) {
			Expression& expr = exprIter.current();

			// Here we assume all the expressions have been converted to binary operations.
			if (is_targeted_expr(expr)) {
				exprs.insert(&expr);

				SubExprWorkspace* workspace = get_workspace(stmt);

				workspace->targetExpr = &expr;

				stringstream exprStrStream;
				expr.print(exprStrStream);

				string exprStr = exprStrStream.str();

				set<Statement*>* stmtWithCurrExpr = exprStmtLookup[exprStr];

				cout << stmtWithCurrExpr << endl;

				if (stmtWithCurrExpr == NULL) {
					stmtWithCurrExpr = new set<Statement*>;
					exprStmtLookup[exprStr] = stmtWithCurrExpr;
				}

				stmtWithCurrExpr->insert(&stmt);
			}
		}
	}

//	for (map<string, set<Statement*>* >::iterator it = exprStmtLookup.begin(); it != exprStmtLookup.end(); ++it) {
//		cout << "For expr " << it->first << endl;
//
//		set<Statement*>* stmts = it->second;
//
//		for (set<Statement*>::iterator itt = stmts->begin(); itt != stmts->end(); ++itt) {
//			cout << (*itt)->tag() << endl;
//		}
//
//		cout << "----------------------" << endl;
//	}

	return exprs;
}

void calculate_in_out_sets(ProgramUnit& pgm) {
	StmtList& stmts = pgm.stmts();

	// Init the out sets
	bool isEntryStmt = true;

	set<Expression*> allTargetedExprs = compute_targeted_exprs(stmts);

	for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
		Statement& stmt = stmtIter.current();

		SubExprWorkspace* workspace = get_workspace(stmt);

		if (isEntryStmt) {
			isEntryStmt = false;
		} else {
			workspace->outSet = allTargetedExprs;
		}
	}

	bool changed;

	do {
		changed = false;

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
				if (is_targeted_expr(expr)) {
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

			if (!set_equal(workspace->outSet, resultSet)) {
				changed = true;
			}

			workspace->outSet = resultSet;
		}
	} while (changed);
}

void eliminate_common_subexpr_in_stmt(Statement& stmt, ProgramUnit& pgm) {
	StmtList& stmts = pgm.stmts();

	SubExprWorkspace* defStmtWorkspace = get_workspace(stmt);

	Expression* targetExpr = defStmtWorkspace->targetExpr;

	if (targetExpr == NULL) {
		// Current statement does not have a target expression for elimination
		return;
	}

	vector<Statement*> stmtsWithCommonSubExpr;

	for (Iterator<Statement> otherIter = stmts; otherIter.valid(); ++otherIter) {
		Statement& otherStmt = otherIter.current();

		SubExprWorkspace* otherWS = get_workspace(otherStmt);

		if (otherWS->targetExpr == NULL) {
			continue;
		}

		if (expr_eq(*otherWS->targetExpr, *targetExpr)) {
			stmtsWithCommonSubExpr.push_back(&otherStmt);
		}
	}

	cout << stmt << endl;
	cout << stmtsWithCommonSubExpr.size() << endl;

}

void _eliminate_common_subexpr_in_stmt(Statement& defStmt, ProgramUnit& pgm) {
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
		// TODO: something really weird going on here. variable names becomes somethingl like "???"
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

	/*
	for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
		Statement& stmt = stmtIter.current();

		eliminate_common_subexpr_in_stmt(stmt, pgm);
	}
	*/

	for (map<string, set<Statement*>* >::iterator it = exprStmtLookup.begin(); it != exprStmtLookup.end(); ++it) {
		cout << "For expr " << it->first << endl;

		set<Statement*>* stmts = it->second;

		for (set<Statement*>::iterator itt = stmts->begin(); itt != stmts->end(); ++itt) {
			cout << (*itt)->tag() << endl;
		}

		cout << "----------------------" << endl;
	}
}

void propagate_copies(ProgramUnit& pgm) {
	StmtList& stmts = pgm.stmts();

	for (Iterator<Statement> defIter = stmts.stmts_of_type(ASSIGNMENT_STMT); defIter.valid(); ++defIter) {
		Statement& defStmt = defIter.current();

		Expression& copiedVar = defStmt.rhs();
		Expression& definedVar = defStmt.lhs();

		// TODO: arrays
		if (defStmt.rhs().op() != ID_OP) {
			continue;
		}

		for (Iterator<Statement> useIter = stmts; useIter.valid(); ++useIter) {
			Statement& useStmt = useIter.current();

			for (Mutator<Expression> inIter = useStmt.in_refs(); inIter.valid(); ++inIter) {
				Expression& inExpr = inIter.current();

				if (inExpr == definedVar) {
					cout << "Found use of var " << inExpr << " in " << useStmt.tag() << endl;

					inIter.assign() = copiedVar.clone();
				}
			}

			useStmt.build_refs();

//			replace_expression_in_stmt(&useStmt, &definedVar, &copiedVar);
		}

//		cout << "should be deleted: " << defStmt << endl;

//		stmts.del(defStmt);
	}
}

void subexpr_elimination(ProgramUnit& pgm,
                         List<BasicBlock> * pgm_basic_blocks) {

	binarify_operations(pgm);

	calculate_in_out_sets(pgm);

	eliminate_common_subexpr(pgm);

	propagate_copies(pgm);




//	calculate_in_out_sets(pgm);
//
//	eliminate_common_subexpr(pgm);
//
//	propagate_copies(pgm);
//
//	calculate_in_out_sets(pgm);
//
//	eliminate_common_subexpr(pgm);
//
//	propagate_copies(pgm);
}

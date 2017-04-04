#include <map>
#include <queue>
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
map<Statement*, SubExprWorkspace*> wsLookup;

bool is_stmt_with_named_func(Statement& stmt, const char* name) {
    if (stmt.stmt_class() != ASSIGNMENT_STMT) {
        return false;
    }

    const Expression& rhs = stmt.rhs();

    if (rhs.op() != FUNCTION_CALL_OP) {
        return false;
    }

    const Expression& func = rhs.function();

    return (strcmp(func.symbol().name_ref(), name) == 0);
}

bool is_phi_stmt(Statement& stmt) {
    return is_stmt_with_named_func(stmt, "PHI");
}

const char* new_temp_variable_name() {
	stringstream ss;

	ss << "__temp" << variableNameNum;
	variableNameNum++;

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
	SubExprWorkspace* ws = wsLookup[&stmt];

	if (ws == NULL) {
		ws = new SubExprWorkspace(PASS_TAG);
		wsLookup[&stmt] = ws;
	}

	return ws;
}

bool is_targeted_expr(Expression& expr) {
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
//    cout << "recur " << endl;
//    cout << *oldExpr << " " << *newExpr << endl;
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
    stmt->build_refs();
//    cout << "replaceing " << *oldExpr << " with " << *newExpr << " in " << *stmt << endl;
	for (Mutator<Expression> exprIter = stmt->iterate_expressions(); exprIter.valid(); ++exprIter) {
		Expression& expr = exprIter.current();
		exprIter.assign() = replace_expression(&expr, oldExpr, newExpr);
	}

	stmt->build_refs();
}

Statement* find_closest_common_dominator(set<Statement*> stmts) {
	set<Statement*> commonDominators;

	int stmtIdx = 0;

	// Create a common set of dominators
	for (set<Statement*>::iterator stmtIter = stmts.begin();
		stmtIter != stmts.end();
		++stmtIter) {

		Statement* stmt = *stmtIter;
		SubExprWorkspace* ws = get_workspace(*stmt);

		if (stmtIdx == 0) {
			commonDominators = ws->dominators;
		} else {
			set_intersect(commonDominators, ws->dominators);
		}

		stmtIdx++;
	}

	// From any of the nodes, traverse the immediate dominator iteratively until we meet an element in the
	// common dominator set. That node is the closest common dominator of all the statements.
	Statement* runner = *stmts.begin();

	while (commonDominators.count(runner) == 0) {
		SubExprWorkspace* runnerWS = get_workspace(*runner);
		runner = runnerWS->idom;
	}

	return runner;
}

Expression* create_intermediate_expr(OP_TYPE op, Expression* op1, Expression* op2 = NULL) {
    cout << op << " " << *op1 << " " << *op2 << endl;
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

void compute_dominance(ProgramUnit& pgm) {
	StmtList& stmts = pgm.stmts();

	for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
		Statement& stmt = stmtIter.current();
		SubExprWorkspace* ws = get_workspace(stmt);

		ws->dominators.clear();

		for (Iterator<Statement> initDomIter = stmts; initDomIter.valid(); ++initDomIter) {
			Statement& initDomStmt = initDomIter.current();

			ws->dominators.insert(&initDomStmt);
		}
	}

	queue<Statement*> workList;
	workList.push(&stmts[0]);

	while (!workList.empty()) {
		Statement* currStmt = workList.front();
		SubExprWorkspace* currWS = get_workspace(*currStmt);

		workList.pop();

		set<Statement*> newDominators;

		int predIdx = 0;
		for (Iterator<Statement> predIter = currStmt->pred(); predIter.valid(); ++predIter) {
			Statement& predStmt = predIter.current();
			SubExprWorkspace* predWS = get_workspace(predStmt);

			if (predIdx == 0) {
				newDominators = predWS->dominators;
			} else {
				set_intersect(newDominators, predWS->dominators);
			}

			predIdx++;
		}

		newDominators.insert(currStmt);

		if (!set_equal(newDominators, currWS->dominators)) {
			currWS->dominators = newDominators;

			for (Iterator<Statement> succIter = currStmt->succ(); succIter.valid(); ++succIter) {

				Statement& succStmt = succIter.current();
				workList.push(&succStmt);
			}
		}
	}

	for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
		Statement& stmt = stmtIter.current();
		SubExprWorkspace* ws = get_workspace(stmt);

		Statement* runner = &stmt;

		while (runner != NULL) {
			RefSet<Statement> predStmts = runner->pred();

			if (predStmts.entries() == 0) {
				break;
			}

			runner = &predStmts._element(0);

			if (ws->dominators.count(runner) > 0) {
				break;
			}
		}

		if (&stmt != runner) {
			ws->idom = runner;
		}
	}
}

Expression* binary_conversion_helper(
		Expression& expr,
		vector<Statement*>& intermediateStmts,
		ProgramUnit& pgm,
		bool isTopLevel = false
	) {
    if (!is_targeted_expr(expr)) {
        return &expr;
    }

	int argCount = expr.arg_list().entries();

	vector<Expression*> newExprs;

	bool hasNestedExpr = false;

	cout << "arg list: " << endl;
	cout << expr.arg_list() << endl;

	for (Iterator<Expression> argIter = expr.arg_list(); argIter.valid(); ++argIter) {
		Expression& argExpr = argIter.current();

		cout << argExpr.arg_list() << endl;
		cout << "params: " << argExpr.parameters_guarded() << endl;

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

	if (isTopLevel && (argCount <= 2) && !hasNestedExpr) {
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

	Statement* intermediateStmt = new AssignmentStmt(pgm.stmts().new_tag(), preComputedExpr, newExpr);
	intermediateStmts.push_back(intermediateStmt);

	if (operatorCount > 2) {
		// More than 2 operands, create intermediate statements iteratively
		Expression* lastPreComputedExpr = preComputedExpr;

		for (int idx = 2; idx < newExprs.size(); idx++) {
			preComputedExpr = new_variable(new_temp_variable_name(), expr.type(), pgm);
			Expression* newExpr = create_intermediate_expr(expr.op(), lastPreComputedExpr->clone(), newExprs[idx]);

			intermediateStmt = new AssignmentStmt(pgm.stmts().new_tag(), preComputedExpr, newExpr);
			intermediateStmts.push_back(intermediateStmt);

			lastPreComputedExpr = preComputedExpr;
		}
	}

	return preComputedExpr;
}

void convert_to_binary_operation(Statement& stmt, ProgramUnit& pgm) {
	StmtList& stmts = pgm.stmts();

	SubExprWorkspace* ws = get_workspace(stmt);

	for (Iterator<Expression> exprIter = stmt.iterate_expressions(); exprIter.valid(); ++exprIter) {
		Expression& expr = exprIter.current();

		if (is_targeted_expr(expr)) {
			vector<Statement*> intermediateStmts;

			Expression* lastIntermediateExpr = binary_conversion_helper(expr, intermediateStmts, pgm, true);

			Statement* prevCopyStmt = NULL;

			for (vector<Statement*>::iterator intermediateIter = intermediateStmts.begin();
				intermediateIter != intermediateStmts.end();
				++intermediateIter) {
				Statement* intermediateStmt = *intermediateIter;

				stmts.ins_before(intermediateStmt, &stmt);

				if (prevCopyStmt != NULL) {
				    SubExprWorkspace* intermediateStmtWS = get_workspace(*intermediateStmt);

				    intermediateStmtWS->prevCopyRefStmt = prevCopyStmt;
				}

				prevCopyStmt = intermediateStmt;
			}

			ws->prevCopyRefStmt = prevCopyStmt;

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

	exprStmtLookup.clear();

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

				if (stmtWithCurrExpr == NULL) {
					stmtWithCurrExpr = new set<Statement*>;
					exprStmtLookup[exprStr] = stmtWithCurrExpr;
				}

				stmtWithCurrExpr->insert(&stmt);
			}
		}
	}

	return exprs;
}

void eliminate_common_subexpr(ProgramUnit& pgm) {
	StmtList& stmts = pgm.stmts();

	compute_targeted_exprs(stmts);

	for (map<string, set<Statement*>* >::iterator exprIter = exprStmtLookup.begin();
		exprIter != exprStmtLookup.end();
		++exprIter) {
		set<Statement*>* stmtsWithCommonExpr = exprIter->second;

		if (stmtsWithCommonExpr->size() <= 1) {
			continue;
		}

		Statement* anyStmtWithCommonExpr = *stmtsWithCommonExpr->begin();
		SubExprWorkspace* ws = get_workspace(*anyStmtWithCommonExpr);
		Expression* targetExpr = ws->targetExpr;

		Statement* closestCommonDominator = find_closest_common_dominator(*stmtsWithCommonExpr);
		Expression* preComputedExpr = new_variable(new_temp_variable_name(), targetExpr->type(), pgm);
		Expression* rhs = targetExpr->clone();

		Statement* preComputedStmt = new AssignmentStmt(stmts.new_tag(), preComputedExpr, rhs);
		stmts.ins_before(preComputedStmt, closestCommonDominator);

		for (set<Statement*>::iterator commonExprIter = stmtsWithCommonExpr->begin();
			commonExprIter != stmtsWithCommonExpr->end();
			++commonExprIter) {
			Statement* commonExprStmt = *commonExprIter;
			SubExprWorkspace* commonExprStmtWS = get_workspace(*commonExprStmt);

			replace_expression_in_stmt(commonExprStmt, commonExprStmtWS->targetExpr, preComputedExpr);

			commonExprStmtWS->targetExpr = NULL;
		}
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

		bool used = false;

		for (Iterator<Statement> useIter = stmts; useIter.valid(); ++useIter) {
			Statement& useStmt = useIter.current();

			if (is_phi_stmt(useStmt)) {
			    continue;
			}

			for (Mutator<Expression> inIter = useStmt.in_refs(); inIter.valid(); ++inIter) {
				Expression& inExpr = inIter.current();

				if (inExpr == definedVar) {
					inIter.assign() = copiedVar.clone();

					used = true;
				}
			}

			useStmt.build_refs();
		}

		if (used) {
		    stmts.del(defStmt);
		}
	}
}

bool stmts_equal(List<Statement>& stmts1, List<Statement>& stmts2) {
    if (stmts1.entries() != stmts2.entries()) {
        return false;
    }

    for (int idx = 0; idx < stmts1.entries(); idx++) {
        const Statement& s1 = stmts1[idx];
        const Statement& s2 = stmts2[idx];

        stringstream sstream1, sstream2;

        int indent1 = 0, indent2 = 0;
        s1.write(sstream1, indent1);
        s2.write(sstream2, indent2);

//        if (sstream1.str() != sstream2.str()) {
        if (s1.stmt_class() != s2.stmt_class()) {
//            cout << sstream1.str() << " " << sstream2.str() << endl;
            return false;
        }
    }

    return true;
}

void subexpr_elimination(ProgramUnit& pgm,
                         List<BasicBlock> * pgm_basic_blocks) {
	StmtList& stmts = pgm.stmts();

	bool changed;
	do {
	    changed = false;
	    cout << "new iteration --------------------------------------------" << endl;

	    binarify_operations(pgm);

        List<Statement>* oldStmts = stmts.copy(stmts.first(), stmts.last());

	    stringstream oldStmtsStr;
	    stmts.write(oldStmtsStr);

        compute_dominance(pgm);
        eliminate_common_subexpr(pgm);
        propagate_copies(pgm);

        stringstream newStmtsStr;
        pgm.stmts().write(newStmtsStr);

        changed = !stmts_equal(*oldStmts, pgm.stmts());
	} while (changed);
}

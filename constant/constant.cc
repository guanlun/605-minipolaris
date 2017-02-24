// 
// constant.cc : constant propagation pass
//
#include "constant.h"
#include "StmtList.h"
#include "Statement/Statement.h"
#include "Expression/expr_funcs.h"
#include "DictionaryIter.h"

#include <map>

using namespace std;

const int PASS_TAG = 2;

void stmt_set_intersection(RefSet<Statement>& base, RefSet<Statement>& other) {
	for (Iterator<Statement> iter = base; iter.valid(); ++iter) {
		Statement& stmt = iter.current();

		if (!other.member(stmt)) {
			base.del(stmt);
		}
	}
}

Expression* replace_symbol(Expression* expr, Symbol* oldSymbol, Expression* newExpr) {
	if (expr->op() == ID_OP) {
		if (&expr->symbol() == oldSymbol) {
			return newExpr->clone();
		}
	} else {
		for (Mutator<Expression> exprMut = expr->arg_list(); exprMut.valid(); ++exprMut) {
			exprMut.assign() = replace_symbol(exprMut.pull(), oldSymbol, newExpr);
		}
	}

	return expr;
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

void find_function_helper(RefList<Expression>& funcExprs, Expression& expr) {
	if (expr.op() == FUNCTION_CALL_OP) {
		funcExprs.ins_last(expr);
	} else {
		for (Iterator<Expression> exprIter = expr.arg_list(); exprIter.valid(); ++exprIter) {
			find_function_helper(funcExprs, exprIter.current());
		}
	}
}

RefList<Expression> find_function_calls_in_stmt(Statement& stmt) {
	RefList<Expression> exprList;

	for (Iterator<Expression> exprIter = stmt.iterate_all_expressions();
		exprIter.valid();
		++exprIter) {
		Expression& expr = exprIter.current();

		find_function_helper(exprList, expr);
	}

	return exprList;
}

void find_common_helper(RefList<Expression>& commonExprs, Expression& expr) {
	if (expr.op() == ID_OP) {
		Symbol& symbol = expr.symbol();

		if (symbol.common_ref() != NULL) {
			commonExprs.ins_last(expr);
		}
	} else {
		for (Iterator<Expression> exprIter = expr.arg_list(); exprIter.valid(); ++exprIter) {
			find_common_helper(commonExprs, exprIter.current());
		}
	}
}

RefList<Expression> find_common_variables_in_stmt(Statement& stmt) {
	RefList<Expression> exprList;

	for (Iterator<Expression> exprIter = stmt.iterate_all_expressions();
		exprIter.valid();
		++exprIter) {
		Expression& expr = exprIter.current();

		find_common_helper(exprList, expr);
	}

	return exprList;
}

bool expr_in_expr(Expression* needle, Expression* haystack) {
	if (*needle == *haystack) {
		return true;
	} else {
		for (Iterator<Expression> iter = haystack->arg_list(); iter.valid(); ++iter) {
			if (expr_in_expr(needle, &iter.current())) {
				return true;
			}
		}

		return false;
	}
}

inline bool is_const_true_expression(const Expression* expr) {
	return (expr->op() == LOGICAL_CONSTANT_OP) && (expr->str_data() == ".TRUE.");
}

void replace_const_param_symbols(ProgramUnit& pgm) {
	StmtList& stmts = pgm.stmts();

	map<Symbol*, Expression*> constLookup;

	for (DictionaryIter<Symbol> symIter = pgm.symtab().iterator(); symIter.valid(); ++symIter) {
		Symbol& symbol = symIter.current();

		if (symbol.sym_class() == SYMBOLIC_CONSTANT_CLASS) {
			constLookup.insert(pair<Symbol*, Expression*>(&symbol, symbol.expr_ref()));
		}
	}

	for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
		Statement& stmt = iter.current();

		for (Mutator<Expression> exprIter = stmt.in_refs();
			exprIter.valid();
			++exprIter) {
			Expression& expr = exprIter.current();

			for (map<Symbol*, Expression*>::const_iterator constIter = constLookup.begin();
				constIter != constLookup.end();
				++constIter) {

				exprIter.assign() = replace_symbol(&expr, constIter->first, constIter->second);
			}
		}
	}
}

RefSet<Symbol> find_non_constant_symbols(ProgramUnit& pgm) {
	RefSet<Symbol> nonConstantSymbols;

	for (DictionaryIter<Symbol> symIter = pgm.symtab().iterator(); symIter.valid(); ++symIter) {
		Symbol& symbol = symIter.current();

		if ((symbol.equivalence_ref() != NULL) ||
			symbol.saved() ||
			(symbol.sym_class() == BLOCK_DATA_CLASS)) {
			nonConstantSymbols.ins(symbol);
		}
	}

	return nonConstantSymbols;
}

void detect_in_out_sets(ProgramUnit& pgm, RefSet<Symbol>& nonConstantSymbols, bool removingUnusedVar = false) {
	StmtList& stmts = pgm.stmts();

	bool changed = true;
	int iterCount = 0;

	while (changed) {
		changed = false;

		for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
			Statement& stmt = iter.current();

			ConstPropWS* constPropWS = (ConstPropWS*)stmt.work_stack().top_ref(PASS_TAG);

			if (!constPropWS) {
				stmt.work_stack().push(new ConstPropWS(PASS_TAG));
				constPropWS = (ConstPropWS*)stmt.work_stack().top_ref(PASS_TAG);
			}

			RefSet<Statement> genSet;
			RefSet<Statement> resultSet;

			int predStmtIdx = 0;

			// Iterate through all predecessors
			for	(Iterator<Statement> predStmtIter = stmt.pred();
				predStmtIter.valid();
				++predStmtIter) {
				Statement& predStmt = predStmtIter.current();

				ConstPropWS* predConstPropWS = (ConstPropWS*)predStmt.work_stack().top_ref(PASS_TAG);

				if (predConstPropWS) {
					// Predecessor already has a WorkSpace, meaning that it has been visited before.
					// We do not consider predecessors that hasn't been visited, because otherwise the
					// intersection would always lead to an empty set (e.g. do statements).

					if (removingUnusedVar) {
						// When doing unused variable removal, use the union operator instead.
						constPropWS->inSet += predConstPropWS->outSet;

					} else {
						// The following block act as the intersection operator
						if (predStmtIdx == 0) {
							constPropWS->inSet = predConstPropWS->outSet;
						} else {
							stmt_set_intersection(constPropWS->inSet, predConstPropWS->outSet);
						}
					}

					++predStmtIdx;
				}
			}

			if (!removingUnusedVar) {
				RefList<Expression> functionCallExprs = find_function_calls_in_stmt(stmt);

				if (functionCallExprs.entries() > 0) {
					// This statement contains at least one function call.
					RefSet<Statement> invalidDefs;

					// At function calls, remove all the COMMON variables from the in set, as the callee
					// might change them.
					RefList<Expression> commonVariables = find_common_variables_in_stmt(stmt);

					for (Iterator<Expression> commonIter = commonVariables; commonIter.valid(); ++commonIter) {
						Expression& commonExpr = commonIter.current();

						for (Iterator<Statement> defIter = constPropWS->inSet; defIter.valid(); ++defIter) {
							Statement& prevDef = defIter.current();

							if (commonExpr == prevDef.lhs()) {
								invalidDefs.ins(prevDef);
							}
						}
					}

					// For any variables passed to functions as parameters, as Fortran uses pass-by-reference,
					// we cannot assume that they would not change inside the functions, therefore we cannot
					// propagation constant values to them. Also, as their value may change, we should kill
					// any previous definitions of that variable.
					for (Iterator<Expression> funcIter = functionCallExprs; funcIter.valid(); ++funcIter) {
						Expression& funcCallExpr = funcIter.current();

						const RefList<Expression>* paramList = funcCallExpr.parameters_guarded().arg_refs();

						for (Iterator<Expression> paramIter = paramList;
							paramIter.valid();
							++paramIter) {
							// Iterator each function parameter
							Expression& param = paramIter.current();

							for (Iterator<Statement> defIter = constPropWS->inSet; defIter.valid(); ++defIter) {
								Statement& prevDef = defIter.current();

								if (param == prevDef.lhs()) {
									// Function parameter defined before, but we cannot propagate to parameters,
									// therefore we keep it in a temp set and remove it later.
									invalidDefs.ins(prevDef);
									break;
								}
							}
						}
					}

					constPropWS->inSet -= invalidDefs;
				}
			}

			resultSet += constPropWS->inSet;

			if (stmt.stmt_class() == ASSIGNMENT_STMT) {
				Expression& lhsExpr = stmt.lhs();
				OP_TYPE opType = lhsExpr.op();
				if ((opType == ID_OP) || (opType == ARRAY_REF_OP)) {
					Symbol& assignedSymbol = *stmt.lhs().base_variable_ref();

					if (!nonConstantSymbols.member(assignedSymbol)) {
						genSet.ins(stmt);
					}
				}

				for (Iterator<Statement> defIter = resultSet; defIter.valid(); ++defIter) {
					Statement& prevDef = defIter.current();

					if (stmt.lhs() == prevDef.lhs()) {
						resultSet.del(prevDef);
					} else if (!removingUnusedVar && expr_in_expr(&stmt.lhs(), &prevDef.rhs())) {
						resultSet.del(prevDef);
					}
				}
			}

			resultSet += genSet;

			if (!(constPropWS->outSet == resultSet)) {
				// Something changed in the out set, need another iteration
				constPropWS->outSet = resultSet;

				changed = true;
			}
		}

		iterCount++;
	}
}

void replace_inset_symbols(StmtList& stmts, bool& hasChange) {
	for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
		Statement& stmt = iter.current();

		ConstPropWS* constPropWS = (ConstPropWS*)stmt.work_stack().top_ref(PASS_TAG);

		for (Mutator<Expression> exprIter = stmt.in_refs();
			exprIter.valid();
			++exprIter) {
			Expression& expr = exprIter.current();

			for (Iterator<Statement> defIter = constPropWS->inSet; defIter.valid(); ++defIter) {
				Statement& def = defIter.current();

				if (expr.op() == DELETED_EXPRESSION_OP) {
					break;
				}

				switch (def.rhs().op()) {
				case INTEGER_CONSTANT_OP:
				case REAL_CONSTANT_OP:
				case STRING_CONSTANT_OP:
				case LOGICAL_CONSTANT_OP:
					Expression* replaced = replace_expression(&expr, &def.lhs(), def.rhs().clone());

					if (!(expr == *replaced)) {
						exprIter.assign() = replaced;
						hasChange = true;
					}
					break;
				}
			}
		}
	}
}

void simplify_const_expressions(StmtList& stmts, bool& hasChange) {
	for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
		Statement& stmt = iter.current();
		stmt.simplify_expressions();
	}
}

void remove_dead_branches(StmtList& stmts, bool& hasChange) {
	// remove dead if statements
	Iterator<Statement> iter = stmts.stmts_of_type(IF_STMT);

	for (; iter.valid(); ++iter) {
		Statement& stmt = iter.current();

		RefList<Statement> branches;
		Statement* branch = &stmt;

		while (branch) {
			branches.ins_last(*branch);

			branch = branch->follow_ref();
		}

		RefList<Expression> validPredicates;

		RefList<Statement> validBranchHeads;
		RefList<Statement> validBranchLasts;

		RefList<Statement> deadBranchHeads;
		RefList<Statement> deadBranchLasts;

		Statement* endIfStmt = stmt.matching_endif_ref();

		bool seenConstTrue = false;
		bool seenConstFalse = false;

		for (Iterator<Statement> branchIter = branches; branchIter.valid(); ++branchIter) {
			Statement& branchStmt = branchIter.current();

			// Skip the ENDIF statement at last
			if (&branchStmt == endIfStmt) {
				break;
			}

			Statement* firstInBranch = branchStmt.next_ref();

			// Follow ref is the next branch stmt, prev is implied goto and prev prev is the actual stmt
			Statement* lastInBranch = branchStmt.follow_ref()->prev_ref()->prev_ref();

			if (seenConstTrue) {
				// There is already a .TRUE. branch before this one, therefore this branch is dead
				deadBranchHeads.ins_last(*firstInBranch);
				deadBranchLasts.ins_last(*lastInBranch);

			} else {
				if (branchStmt.stmt_class() != ELSE_STMT) {
					Expression& predicate = branchStmt.expr();

					if (predicate.op() == LOGICAL_CONSTANT_OP) {
						String predicateVal = predicate.str_data();

						// Found the constant true predicate, ignore everything after it.
						if (predicateVal == ".TRUE.") {
							seenConstTrue = true;

							validPredicates.ins_last(predicate);
							validBranchHeads.ins_last(*firstInBranch);
							validBranchLasts.ins_last(*lastInBranch);
						} else {
							seenConstFalse = true;

							deadBranchHeads.ins_last(*firstInBranch);
							deadBranchLasts.ins_last(*lastInBranch);
						}
					} else {
						// Predicate cannot be resolved to a constant at compile time
						validPredicates.ins_last(predicate);
						validBranchHeads.ins_last(*firstInBranch);
						validBranchLasts.ins_last(*lastInBranch);
					}
				} else {
					// Else statement (last one), and there's no const TRUE in any statements above
					validBranchHeads.ins_last(*firstInBranch);
					validBranchLasts.ins_last(*lastInBranch);
				}
			}
		}

		if (seenConstTrue || seenConstFalse) {
			// Since there are one iteration for each if-elseif-else statement, we should not use
			// hasChange = seenConstTrue || seenConstFalse. This is because hasChange might be set
			// back to false in the 2nd loop and we'll lose that information.
			hasChange = true;
		}

		if (seenConstTrue || seenConstFalse) {
			Statement* newIfStmt;

			// Create new if-elseif-else branch for valid branches
			for (int i = 0; i < validBranchHeads.entries(); i++) {
				Expression* predicate;

				if (i == validPredicates.entries()) {
					// More branches than predicates, indicating we have an "else" branch at last
					predicate = NULL;
				} else {
					predicate = &validPredicates[i];
				}

				Statement& head = validBranchHeads[i];
				Statement& last = validBranchLasts[i];

				List<Statement>* stmtsToMove = stmts.copy(head, last);

				if (&head == &last) {
					stmts.del(head);
				} else {
					stmts.del(head, last);
				}

				Statement* lastInsertedBranchStmt;

				if (i == 0) {
					// First branch, and we should insert an "if"
					if ((predicate == NULL) || is_const_true_expression(predicate)) {
						// First branch is already constant true, no need for the entire if-else structure
						stmts.ins_after(stmtsToMove, endIfStmt);
						break;

					} else {
						newIfStmt = &stmts.ins_IF_after(predicate->clone(), endIfStmt);
						lastInsertedBranchStmt = newIfStmt;

						stmts.ins_after(stmtsToMove, lastInsertedBranchStmt);
					}

				} else {
					if ((predicate == NULL) || is_const_true_expression(predicate)) {
						// Meet a constant true branch in one of the "elseif"s, just change it to "else"
						lastInsertedBranchStmt = &stmts.ins_ELSE_after(newIfStmt);
						stmts.ins_after(stmtsToMove, lastInsertedBranchStmt);
						break;
					} else {
						lastInsertedBranchStmt = &stmts.ins_ELSEIF_after(predicate->clone(), newIfStmt);
						stmts.ins_after(stmtsToMove, lastInsertedBranchStmt);
					}
				}
			}

			for (int i = 0; i < deadBranchHeads.entries(); i++) {
				Statement& head = deadBranchHeads[i];
				Statement& last = deadBranchLasts[i];

				if (&head == &last) {
					stmts.del(head);
				} else {
					stmts.del(head, last);
				}
			}

			// Delete the entire if-elseif-else branch by deleting the "if" statement
			stmts.del(stmt);
		}
	}

	iter = stmts.stmts_of_type(WHILE_STMT);

	for (; iter.valid(); ++iter) {
		Statement& stmt = iter.current();

		Expression& predicate = stmt.expr();

		if ((predicate.op() == LOGICAL_CONSTANT_OP) && (predicate.str_data() == ".FALSE.")) {
			stmts.del(*stmt.next_ref(), *stmt.follow_ref()->prev_ref());
			stmts.del(stmt);
		}
	}
}

void remove_unused_variables(StmtList& stmts, bool& hasChange) {
	for (Iterator<Statement> iter = stmts;
		iter.valid(); ++iter) {
		Statement& stmt = iter.current();

		ConstPropWS* constPropWS = (ConstPropWS*)stmt.work_stack().top_ref(PASS_TAG);

		for (Iterator<Expression> exprIter = stmt.in_refs();
			exprIter.valid();
			++exprIter) {
			Expression& expr = exprIter.current();

			for (Iterator<Statement> defIter = constPropWS->inSet;
				defIter.valid();
				++defIter) {
				Statement& def = defIter.current();

				if (expr == def.lhs()) {
					ConstPropWS* defConstPropWS = (ConstPropWS*)def.work_stack().top_ref(PASS_TAG);
					defConstPropWS->refCount++;
				}
			}
		}
	}

	for (Iterator<Statement> iter = stmts.stmts_of_type(ASSIGNMENT_STMT);
		iter.valid(); ++iter) {
		Statement& stmt = iter.current();

		Expression& lhsExpr = stmt.lhs();

		if (lhsExpr.op() == ARRAY_REF_OP) {
			continue;
		}

		// Assignment to formal parameters should not be removed, as Fortran uses pass-by-reference
		if (lhsExpr.symbol().formal()) {
			continue;
		}

		// Function calls might have side effects, do not remove function calls.
		RefList<Expression> functionCallExprs = find_function_calls_in_stmt(stmt);
		if (functionCallExprs.entries() > 0) {
			continue;
		}

		ConstPropWS* constPropWS = (ConstPropWS*)stmt.work_stack().top_ref(PASS_TAG);

		if (constPropWS->refCount == 0) {
			stmts.del(stmt);
			hasChange = true;
		}
	}
}

void clean_workspace(StmtList& stmts) {
	for (Iterator<Statement> iter = stmts;
		iter.valid(); ++iter) {
		Statement& stmt = iter.current();

		stmt.work_stack().pop(PASS_TAG);

		// Re-build the in and out refs, since deleting statements would not re-link them.
		stmt.build_refs();
	}
}

void propagate_constants(ProgramUnit & pgm) {
	StmtList& stmts = pgm.stmts();

	replace_const_param_symbols(pgm);

	bool hasChange;

	do {
		hasChange = false;
		RefSet<Symbol> nonConstantSymbols = find_non_constant_symbols(pgm);
		detect_in_out_sets(pgm, nonConstantSymbols);
		replace_inset_symbols(stmts, hasChange);
		simplify_const_expressions(stmts, hasChange);
		remove_dead_branches(stmts, hasChange);
		clean_workspace(stmts);
	} while (hasChange);

	RefSet<Symbol> nonConstantSymbols = find_non_constant_symbols(pgm);
	if (nonConstantSymbols.empty()) {
		// Playing safe here: do not remove unused variables when non constant symbols exist
		do {
			hasChange = false;
			detect_in_out_sets(pgm, nonConstantSymbols, true);
			remove_unused_variables(stmts, hasChange);
			clean_workspace(stmts);
		} while (hasChange);
	}
};

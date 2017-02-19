// 
// constant.cc : constant propagation pass
// 

#include "constant.h"
#include "StmtList.h"
#include "Statement/Statement.h"
#include "Statement/IfStmt.h"
#include "Expression/IntConstExpr.h"
#include "Expression/expr_funcs.h"
#include "Symbol/SymbolicConstantSymbol.h"
#include "Collection/RefMap.h"
#include "DictionaryIter.h"

const int PASS_TAG = 2;

void stmt_set_intersection(RefSet<Statement>& base, RefSet<Statement>& other) {
	if (base.empty()) {
		base = other;
	}

	for (Iterator<Statement> iter = base; iter.valid(); ++iter) {
		Statement& stmt = iter.current();

		if (!other.member(stmt)) {
			base.del(stmt);
		}
	}
}

Expression* replace_expression(Expression* expr, Expression* oldExpr, Expression* newExpr) {
	cout << "replacing" << endl;
	if (expr->op() == ID_OP) {
		if (&expr->symbol() == &oldExpr->symbol()) {
			return newExpr->clone();
		}
	} else {
		for (Mutator<Expression> exprMut = expr->arg_list(); exprMut.valid(); ++exprMut) {
			cout << "current: " << exprMut.current() << endl;
			exprMut.assign() = replace_expression(exprMut.pull(), oldExpr, newExpr);
		}
	}

	return expr;
}

inline bool is_const_true_expression(const Expression* expr) {
	return (expr->op() == LOGICAL_CONSTANT_OP) && (expr->str_data() == ".TRUE.");
}

void replace_const_param_symbols(StmtList& stmts) {
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << "Replacing const param symbols" << endl;
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << endl;
	cout << "----------------------------------------------------------------" << endl;

	for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
		Statement& stmt = iter.current();

		cout << "Statement: " << endl << stmt << endl << "-----------------------" << endl;;

		for (Mutator<Expression> exprIter = stmt.iterate_in_exprs_guarded();
			exprIter.valid();
			++exprIter) {
			Expression& expr = exprIter.current();

			if (expr.op() == ID_OP) {
				const Symbol& symbol = expr.symbol();

				if (symbol.sym_class() == SYMBOLIC_CONSTANT_CLASS) {
//					exprIter.assign() = symbol.expr_ref()->clone();
				 	exprIter.assign() = replace_expression(&expr, &expr, symbol.expr_ref()->clone());
				}
			} else {
//				exprIter.assign() = replace_expression(&expr, &expr, symbol.expr_ref()->clone());
			}
		}
	}
}

void detect_in_out_sets(StmtList& stmts) {
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << "Analyzing in out sets" << endl;
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << endl;
	cout << "----------------------------------------------------------------" << endl;

	for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
		cout << 1 << endl;
		Statement& stmt = iter.current();

		cout << "Statement: " << endl << stmt << endl << "-----------------------" << endl;;

		ConstPropWS* constPropWS = (ConstPropWS*)stmt.work_stack().top_ref(PASS_TAG);

		// TODO: Really hacky
		if (!constPropWS) {
			stmt.work_stack().push(new ConstPropWS(PASS_TAG));
			constPropWS = (ConstPropWS*)stmt.work_stack().top_ref(PASS_TAG);
		}

		RefSet<Statement> predecessors = stmt.pred();

		RefSet<Statement> inSet;
		RefSet<Statement> genSet;
		RefSet<Statement> killSet;

		RefSet<Statement> resultSet;
		cout << 2 << endl;
		// Iterate through all predecessors
		for	(Iterator<Statement> predStmtIter = predecessors; predStmtIter.valid(); ++predStmtIter) {
			Statement& predStmt = predStmtIter.current();

			ConstPropWS* predConstPropWS = (ConstPropWS*)predStmt.work_stack().top_ref(PASS_TAG);
			stmt_set_intersection(inSet, predConstPropWS->outSet);
		}

		cout << 3 << endl;
		cout << constPropWS << endl;

		constPropWS->inSet = inSet;
		cout << 3.5 << endl;
		resultSet += inSet;

		cout << 4 << endl;

		cout << "inset: " << inSet << endl;

		if (stmt.stmt_class() == ASSIGNMENT_STMT) {
			genSet.ins(stmt);

			Symbol& assignedSymbol = stmt.lhs().symbol();

			for (Iterator<Statement> defIter = resultSet; defIter.valid(); ++defIter) {
				Statement& prevDef = defIter.current();

				if (&prevDef.lhs().symbol() == &assignedSymbol) {
					resultSet.del(prevDef);
				}
			}
		}

		resultSet += genSet;

		constPropWS->outSet = resultSet;

		cout << "----------------------------------------------------------------" << endl;
	}
}

void replace_inset_symbols(StmtList& stmts) {
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << "Replacing inset symbols" << endl;
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << endl;
	cout << "----------------------------------------------------------------" << endl;

	for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
		Statement& stmt = iter.current();
		cout << "Statement: " << endl << stmt << endl << "-----------------------" << endl;;

		ConstPropWS* constPropWS = (ConstPropWS*)stmt.work_stack().top_ref(PASS_TAG);
		RefSet<Statement> defs = constPropWS->inSet;

		for (Mutator<Expression> exprIter = stmt.iterate_in_exprs_guarded();
			exprIter.valid();
			++exprIter) {
			Expression& expr = exprIter.current();

			for (Iterator<Statement> defIter = defs; defIter.valid(); ++defIter) {
				Statement& def = defIter.current();

				cout << "def: " << def << endl;
				cout << "lhs: " << def.lhs() << endl;
				cout << "rhs: " << def.rhs() << endl;

				if (expr.op() == DELETED_EXPRESSION_OP) {
					break;
				}

				exprIter.assign() = replace_expression(&expr, &def.lhs(), def.rhs().clone());

//				if (expr.op() == ID_OP) {
//					cout << "is ID" << endl;
//					if (&expr.symbol() == &def.lhs().symbol()) {
//						cout << "replacing ID" << endl;
//						exprIter.assign() = replace_expression(&expr, &def.lhs(), &def.rhs());
//					}
//				} else {
//					cout << "not ID: " << expr.op() << endl;
//					exprIter.assign() = replace_expression(&expr, &def.lhs(), &def.rhs());
//				}
			}
		}
	}
}

void simplify_const_expressions(StmtList& stmts) {
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << "Simplifying const expressions" << endl;
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << endl;
	cout << "----------------------------------------------------------------" << endl;

	for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
		Statement& stmt = iter.current();

		cout << "Statement: " << endl << stmt << endl << "-----------------------" << endl;;

		for (Mutator<Expression> exprIter = stmt.iterate_in_exprs_guarded();
			exprIter.valid();
			++exprIter) {
			Expression& expr = exprIter.current();

			Expression* simplifiedExpr = simplify(expr.clone());

			cout << expr << " is simplified to " << *simplifiedExpr << endl;

			switch (simplifiedExpr->op()) {
			case LOGICAL_CONSTANT_OP:
				exprIter.assign() = simplifiedExpr->clone();
				break;
			case INTEGER_CONSTANT_OP:
				exprIter.assign() = simplifiedExpr->clone();
				break;
			}
		}
	}
}

void _remove_dead_branches(StmtList& stmts, bool& hasChange) {
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << "Removing dead branches" << endl;
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << endl;
	cout << "----------------------------------------------------------------" << endl;

	for (Iterator<Statement> iter = stmts.stmts_of_type(IF_STMT); iter.valid(); ++iter) {
		Statement& ifStmt = iter.current();

		Statement& endIfStmt = *ifStmt.matching_endif_ref();

		Statement& nextBranchStmt = *ifStmt.follow_ref();

		stmts.del(nextBranchStmt, endIfStmt);
	}
}

void remove_dead_branches(StmtList& stmts, bool& hasChange) {
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << "Removing dead branches" << endl;
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << "****************************************************************" << endl;
	cout << endl;
	cout << "----------------------------------------------------------------" << endl;

	// remove dead if statements
	Iterator<Statement> iter = stmts.stmts_of_type(IF_STMT);

	for (; iter.valid(); ++iter) {
		cout << "!@#$%^&*)(@!@$%$^*(&" << endl << stmts << endl;
		Statement& stmt = iter.current();

		cout << "Statement: " << endl << stmt << endl << "-----------------------" << endl;;

		RefList<Statement> branches;
		Statement* branch = &stmt;

		while (branch) {
			branches.ins_last(*branch);

			branch = branch->follow_ref();
		}

		cout << branches << endl;

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
			cout << "Branch: " << endl;
			cout << branchStmt << endl;

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

			cout << "end of branch" << endl;
		}

		cout << seenConstTrue << " " << seenConstFalse << endl;

		cout << "----------------------------------------------------" << endl;
		cout << "Here are all the valid predicates" << endl;
		cout << validPredicates << endl;

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

				cout << "head: " << head << endl;
				cout << "last: " << last << endl;

				List<Statement>* stmtsToMove = stmts.copy(head, last);

				if (&head == &last) {
					stmts.del(head);
				} else {
					stmts.del(head, last);
				}

				Statement* lastInsertedBranchStmt;

				if (i == 0) {
					// First branch, and we should insert an "if"
					if (is_const_true_expression(predicate)) {
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
}

void propagate_constants(ProgramUnit & pgm) {
	StmtList& stmts = pgm.stmts();

	replace_const_param_symbols(stmts);

	for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
		Statement& stmt = iter.current();
		stmt.work_stack().push(new ConstPropWS(PASS_TAG));
	}

	bool hasChange = false;

//	do {
		detect_in_out_sets(stmts);

		replace_inset_symbols(stmts);

		simplify_const_expressions(stmts);

		remove_dead_branches(stmts, hasChange);


		// 2nd time
//		detect_in_out_sets(stmts);
//
//		replace_inset_symbols(stmts);
//
//		simplify_const_expressions(stmts);
//
//		remove_dead_branches(stmts, hasChange);



//	} while (hasChange);
};

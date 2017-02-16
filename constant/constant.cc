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

//Expression* replace_constant_symbol(Expression* expr)

void propagate_constants(ProgramUnit & pgm) {
	StmtList& stmts = pgm.stmts();

	for (DictionaryIter<Symbol> symIter = pgm.symtab().iterator(); symIter.valid(); ++symIter) {
		Symbol& symbol = symIter.current();

		if (symbol.sym_class() == VARIABLE_CLASS) {

		}
	}

	for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
		Statement& stmt = iter.current();

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

	for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
		Statement& stmt = iter.current();
		stmt.work_stack().push(new ConstPropWS(PASS_TAG));
	}

	for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
		Statement& stmt = iter.current();

		ConstPropWS* constPropWS = (ConstPropWS*)stmt.work_stack().top_ref(PASS_TAG);

		RefSet<Statement> predecessors = stmt.pred();

		RefSet<Statement> inSet;
		RefSet<Statement> genSet;
		RefSet<Statement> killSet;

		RefSet<Statement> resultSet;

		// Iterate through all predecessors
		for	(Iterator<Statement> predStmtIter = predecessors; predStmtIter.valid(); ++predStmtIter) {
			Statement& predStmt = predStmtIter.current();

			ConstPropWS* predConstPropWS = (ConstPropWS*)predStmt.work_stack().top_ref(PASS_TAG);
			stmt_set_intersection(inSet, predConstPropWS->outSet);
		}

		constPropWS->inSet = inSet;
		resultSet += inSet;

		cout << "inset: " << inSet << endl;

		if (stmt.stmt_class() == ASSIGNMENT_STMT) {
			genSet.ins(stmt);

			Symbol& assignedSymbol = stmt.lhs().symbol();

			for (Iterator<Statement> defIter = resultSet; defIter.valid(); ++defIter) {
				Statement& prevDef = defIter.current();

				if (&prevDef.lhs().symbol() == &assignedSymbol) {
//					cout << "ASSIGNMENT TO SYMBOL!!!!" << endl;

					resultSet.del(prevDef);
				}
			}
		}

		resultSet += genSet;

		constPropWS->outSet = resultSet;
	}

	for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
		cout << "---------------------- begin of one stmt ------------------------" << endl;

		Statement& stmt = iter.current();
		cout << "Now at: " << stmt << endl;

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

		cout << "---------------------- end of one stmt ------------------------" << endl;
	}

	for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
		Statement& stmt = iter.current();

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


	// Ref: compute const value: int_const_val in https://parasol.tamu.edu/courses/minipolaris/docs/polaris/_prop_const_w_s_8cc-source.html
};

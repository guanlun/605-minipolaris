// 
// constant.cc : constant propagation pass
// 

#include "constant.h"
#include "StmtList.h"
#include "Statement/Statement.h"
#include "Statement/AssignmentStmt.h"
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

void propagate_constants(ProgramUnit & pgm) {
	StmtList& stmts = pgm.stmts();

	for (DictionaryIter<Symbol> symIter = pgm.symtab().iterator(); symIter.valid(); ++symIter) {
		Symbol& symbol = symIter.current();

		if (symbol.sym_class() == VARIABLE_CLASS) {

		}
	}

	for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
		Statement& stmt = iter.current();

		for (Iterator<Expression> exprIter = stmt.iterate_expressions();
			exprIter.valid();
			++exprIter) {
			Expression& expr = exprIter.current();

			if (expr.op() == ID_OP) {
				const Symbol& symbol = expr.symbol();

				if (symbol.sym_class() == SYMBOLIC_CONSTANT_CLASS) {
					const Expression* constExpr = symbol.expr_ref();
				}
			}
		}


	}

	for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
		Statement& stmt = iter.current();
		stmt.work_stack().push(new ConstPropWS(PASS_TAG));
	}

	for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
		cout << "---------------------- begin of one stmt ------------------------" << endl;

		Statement& stmt = iter.current();

		cout << stmt << endl;

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

		resultSet += inSet;

		cout << "inset: " << inSet << endl;

		if (stmt.stmt_class() == ASSIGNMENT_STMT) {
//			constPropWS->add_gen_stmt(stmt);
			genSet.ins(stmt);

			Symbol& assignedSymbol = stmt.lhs().symbol();

			cout << assignedSymbol << endl;

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

		cout << "---------------------- end of one stmt ------------------------" << endl;
	}

	// Ref: compute const value: int_const_val in https://parasol.tamu.edu/courses/minipolaris/docs/polaris/_prop_const_w_s_8cc-source.html
};

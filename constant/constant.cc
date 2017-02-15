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

void propagate_constants(ProgramUnit & pgm) {
	StmtList& stmts = pgm.stmts();

	for (DictionaryIter<Symbol> symIter = pgm.symtab().iterator(); symIter.valid(); ++symIter) {
		Symbol& symbol = symIter.current();

		if (symbol.sym_class() == VARIABLE_CLASS) {

		}
	}

	for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
		Statement& stmt = iter.current();

		// cout << stmt << endl;

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
		Statement& stmt = iter.current();

		if (stmt.stmt_class() == ASSIGNMENT_STMT) {
			RefSet<Statement> predecessors = stmt.pred();
			cout << stmt.lhs().symbol() << endl;

			for (Iterator<Statement> predStmtIter = predecessors; predStmtIter.valid(); ++predStmtIter) {
				Statement& predStmt = predStmtIter.current();

				ConstPropWS* predConstPropWS = (ConstPropWS* )predStmt.work_stack().top_ref(PASS_TAG);

				cout << predConstPropWS << endl;
			}

			cout << "----------------------------------------------" << endl;
		}
	}

	// Ref: compute const value: int_const_val in https://parasol.tamu.edu/courses/minipolaris/docs/polaris/_prop_const_w_s_8cc-source.html
};

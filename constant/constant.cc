// 
// constant.cc : constant propagation pass
// 

#include "constant.h"
#include "StmtList.h"
#include "Statement/Statement.h"
#include "Expression/IntConstExpr.h"
#include "Symbol/SymbolicConstantSymbol.h"

void propagate_constants(ProgramUnit & pgm) {
	StmtList& stmts = pgm.stmts();

	for (Iterator<Statement> iter = stmts; iter.valid(); ++iter) {
		Statement& stmt = iter.current();

		cout << stmt << endl;

		for (Iterator<Expression> exprIter = stmt.iterate_expressions();
			exprIter.valid();
			++exprIter) {
			Expression& expr = exprIter.current();

			if (expr.op() == ID_OP) {
				const Symbol& symbol = expr.symbol();

				if (symbol.sym_class() == SYMBOLIC_CONSTANT_CLASS) {
					const Expression& constExpr = *(symbol.expr_ref());

					// TODO: change rhs() to something else
					stmt.rhs(constExpr.clone());
				}
			}
		}

//		for (Iterator<Expression> inRefIter = stmt.in_refs();
//			inRefIter.valid();
//			++inRefIter) {
//			Expression& inRefExpr = inRefIter.current();
//
//			// inRefExpr.print(cout);
//			cout << inRefExpr << endl;
//		}


		cout << "----------------------------------------------" << endl;
	}
};

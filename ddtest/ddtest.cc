// 
// ddtest.cc : Data Dependence Test pass
// 
#include <set>
#include <vector>
#include <map>

#include "ddtest.h"
#include "StmtList.h"
#include "Collection/List.h"
#include "Statement/Statement.h"

#include "utils.cc"

set<Statement*> lhps;
set<Expression*> uses;
map<Expression*, Statement*> fudChains;
map<Expression*, vector<Statement*> > phiChains;

void preprocess(ProgramUnit& pgm) {
    StmtList& stmts = pgm.stmts();

    for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
        Statement& stmt = stmtIter.current();

        stmt.build_refs();
    }
}

void find_LHPs(ProgramUnit& pgm) {
    StmtList& stmts = pgm.stmts();

    for (Iterator<Statement> doStmtIter = stmts.stmts_of_type(DO_STMT); doStmtIter.valid(); ++doStmtIter) {
        Statement& doStmt = doStmtIter.current();

        Statement* runner = doStmt.next_ref();

        while (is_phi_stmt(*runner)) {
            lhps.insert(runner);

            runner = runner->next_ref();
        }
    }
}

void build_fud_chains(ProgramUnit& pgm) {
    StmtList& stmts = pgm.stmts();

    for (Iterator<Statement> useStmtIter = stmts; useStmtIter.valid(); ++useStmtIter) {
        Statement& useStmt = useStmtIter.current();

        bool isPhi = is_phi_stmt(useStmt);

//        for (Iterator<Expression> exprIter = useStmt.iterate_expressions(); exprIter.valid(); ++exprIter) {
//            Expression& expr = exprIter.current();
//
//            cout << expr << " is at " << &expr << endl;
//        }

        for (Iterator<Expression> useExprIter = useStmt.in_refs(); useExprIter.valid(); ++useExprIter) {
            Expression& useExpr = useExprIter.current();
            uses.insert(&useExpr);

            for (Iterator<Statement> defStmtIter = stmts.stmts_of_type(ASSIGNMENT_STMT); defStmtIter.valid(); ++defStmtIter) {
                Statement& defStmt = defStmtIter.current();

                if (defStmt.lhs() == useExpr) {
                    if (isPhi) {
//                        phiChains[&useExpr] = &defStmt;
                    } else {
                        fudChains[&useExpr] = &defStmt;
                    }
                }
            }
        }
    }
}

void find_dependence(Statement& def, Expression& use) {
//    cout << "Finding dependence for def " << def.tag() << " and use " << use << endl;
}

void find_scalar_dd(ProgramUnit& pgm) {
    cout << 1 << endl;
    cout << lhps.size() << endl;
    for (set<Statement*>::iterator lhpIter = lhps.begin(); lhpIter != lhps.end(); ++lhpIter) {
        Statement* lhpStmt = *lhpIter;
        cout << "Loop header phi: " << lhpStmt->tag() << endl;
    }
    cout << 2 << endl;

    for (set<Expression*>::iterator useExprIter = uses.begin(); useExprIter != uses.end(); ++useExprIter) {
        Expression* useExpr = *useExprIter;

        Statement* defStmt = fudChains[useExpr];

        if (defStmt != NULL) {
            find_dependence(*defStmt, *useExpr);
        }
    }
}

void ddtest(ProgramUnit & pgm)
{
    int a;

    preprocess(pgm);

    find_LHPs(pgm);

    build_fud_chains(pgm);

    find_scalar_dd(pgm);
}

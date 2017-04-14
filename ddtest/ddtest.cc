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

struct DataDependence {
    Statement* dependedStmt;
    Statement* dependingStmt;

    DataDependence(Statement* depended, Statement* depending) {
        dependedStmt = depended;
        dependingStmt = depending;
    }

    void print(ostream& o) {
        o << "Dependence: " << dependedStmt->tag() << ": " << dependingStmt->tag() << endl;
    }
};

set<Statement*> lhps;
set<Expression*> uses;
map<Expression*, Expression*> marked;
map<Expression*, Statement*> markedReaching;
map<Expression*, Expression*> fudChains;
map<Expression*, vector<Expression*>* > phiChains;
map<Statement*, vector<Expression*>* > reaching;
map<Statement*, bool> self;
map<Expression*, Statement*> exprStmtLookup;

set<DataDependence*> dependencies;

void preprocess(ProgramUnit& pgm) {
    StmtList& stmts = pgm.stmts();

    for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
        Statement& stmt = stmtIter.current();

        stmt.build_refs();

        for (Iterator<Expression> inIter = stmt.in_refs(); inIter.valid(); ++inIter) {
            Expression& inExpr = inIter.current();

            exprStmtLookup[&inExpr] = &stmt;
        }

        for (Iterator<Expression> outIter = stmt.out_refs(); outIter.valid(); ++outIter) {
            Expression& outExpr = outIter.current();

            exprStmtLookup[&outExpr] = &stmt;
        }
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

        for (Iterator<Expression> useExprIter = useStmt.in_refs(); useExprIter.valid(); ++useExprIter) {
            Expression& useExpr = useExprIter.current();

            if (!isPhi) {
                uses.insert(&useExpr);
            }

            for (Iterator<Statement> defStmtIter = stmts; defStmtIter.valid(); ++defStmtIter) {
                Statement& defStmt = defStmtIter.current();

                Expression* defExpr = get_def_expr(defStmt);

                if (defExpr == NULL) {
                    continue;
                }

                if (*defExpr == useExpr) {
                    if (isPhi) {
                        Expression& phiTargetExpr = useStmt.lhs();
                        vector<Expression*>* phiChain = phiChains[&phiTargetExpr];

                        if (phiChain == NULL) {
                            phiChain = new vector<Expression*>;
                            phiChains[&phiTargetExpr] = phiChain;
                        }

                        phiChain->push_back(defExpr);

                    } else {
                        fudChains[&useExpr] = defExpr;
                    }
                }
            }
        }
    }

    // Print phi chains
    cout << "phi chains:" << endl;

    for (map<Expression*, vector<Expression*>* >::iterator it = phiChains.begin(); it != phiChains.end(); ++it) {
        Expression* expr = it->first;
        vector<Expression*>* chain = it->second;

        cout << *expr << ": ";
        for (vector<Expression*>::iterator itt = chain->begin(); itt != chain->end(); ++itt) {
            Expression* def = *itt;

            cout << *def << "(" << exprStmtLookup[def]->tag() << "), ";
        }

        cout << endl;
    }
}

void find_reaching(Expression* def, Statement* lhp, int depth = 0) {
    tab(depth);

    cout << "Finding reaching for def " << *def << " and loop-header-phi " << lhp->tag() << endl;

    if (markedReaching[def] == lhp) {
        return;
    }

    markedReaching[def] = lhp;

    Statement* defStmt = exprStmtLookup[def];

    if (defStmt == lhp) {
        self[lhp] = true;
    } else if (is_phi_stmt(*defStmt)) {
        vector<Expression*> phiChain = *phiChains[def];
        find_reaching(phiChain[0], lhp, depth + 1);
        find_reaching(phiChain[1], lhp, depth + 1);
    } else {
        vector<Expression*> reachingDefs = *reaching[lhp];
        reachingDefs.push_back(def);

        // TODO: non-killing
    }
}

void find_dependence(Expression* def, Expression* use, int depth = 0) {
    tab(depth);

    cout << "Finding dependence for def " << *def << " and use " << *use << endl;

    if (marked[def] == use) {
        tab(depth);
        cout << "marked!!!" << endl;
        return;
    }

    marked[def] = use;

    Statement* defStmt = exprStmtLookup[def];
    tab(depth);
    cout << "def stmt: " << defStmt->tag() << endl;

    if (lhps.find(defStmt) != lhps.end()) {
        tab(depth);
        cout << "is loop header phi" << endl;

        vector<Expression*> phiChain = *phiChains[def];

        // The first element refers to the reference from outside the loop body and the second one refers the
        // reference defined inside the loop body.
        Expression* phiChainLoopOutRef = phiChain[0];
        Expression* phiChainLookInRef = phiChain[1];

        find_dependence(phiChainLoopOutRef, use, depth + 1);

        vector<Expression*> reachingDefs = *reaching[defStmt];

        if (reachingDefs.empty()) {
            find_reaching(phiChainLookInRef, defStmt, depth + 1);
        }

        if (self[defStmt]) {
            vector<Expression*> reachingDefs = *reaching[defStmt];

        } else {
            // TODO
        }

    } else if (is_phi_stmt(*defStmt)) {
        tab(depth);
        cout << "is normal phi" << endl;

        // TODO
        vector<Expression*> phiChain = *phiChains[def];
        find_dependence(phiChain[0], use, depth + 1);
        find_dependence(phiChain[1], use, depth + 1);
    } else {
        tab(depth);
        cout << "is nothing" << endl;

        dependencies.insert(new DataDependence(defStmt, exprStmtLookup[use]));
    }
}

void find_scalar_dd(ProgramUnit& pgm) {
    for (set<Statement*>::iterator lhpIter = lhps.begin(); lhpIter != lhps.end(); ++lhpIter) {
        Statement* lhpStmt = *lhpIter;
//        cout << "Loop header phi: " << lhpStmt->tag() << endl;

        reaching[lhpStmt] = new vector<Expression*>;
        self[lhpStmt] = false;
    }

    cout << "Uses: ";
    for (set<Expression*>::iterator useExprIter = uses.begin(); useExprIter != uses.end(); ++useExprIter) {
        Expression* useExpr = *useExprIter;
        cout << *useExpr << "(" << exprStmtLookup[useExpr]->tag() << "), ";
    }
    cout << endl;

    for (set<Expression*>::iterator useExprIter = uses.begin(); useExprIter != uses.end(); ++useExprIter) {
        Expression* useExpr = *useExprIter;

        Expression* defExpr = fudChains[useExpr];

        if (defExpr != NULL) {
            find_dependence(defExpr, useExpr);
        }
    }

    cout << "Data dependencies: " << endl;
    for (set<DataDependence*>::iterator it = dependencies.begin(); it != dependencies.end(); ++it) {
        DataDependence* dd = *it;

        dd->print(cout);
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

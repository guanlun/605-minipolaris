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
#include "Expression/expr_funcs.h"

#include "utils.cc"

enum DEPENDENCE_TYPE {
    FLOW,
    OUTPUT,
    ANTI
};

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

map<Expression*, vector<Expression*>* > currUse;
map<Expression*, vector<Expression*>* > chainUse;
map<Expression*, vector<Expression*>* > saveChainUse;

set<DataDependence*> dependencies;

void add_dependence(Statement* depended, Statement* depending) {
    dependencies.insert(new DataDependence(depended, depending));
}

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

void upsilon_search(Statement* x) {
    cout << "Search stmt: " << x->tag() << endl;

    if (is_phi_stmt(*x)) {
        Expression* m = &x->lhs();
        vector<Expression*> currUseM = *currUse[m];
//        saveChainUse[m] = currUseM;


    } else {

    }
}

void build_frud_chains(ProgramUnit& pgm) {
    StmtList& stmts = pgm.stmts();

    for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
        Statement& stmt = stmtIter.current();

        for (Iterator<Expression> inIter = stmt.in_refs(); inIter.valid(); ++inIter) {
            Expression& inExpr = inIter.current();

            currUse[&inExpr] = new vector<Expression*>;
        }

        for (Iterator<Expression> outIter = stmt.out_refs(); outIter.valid(); ++outIter) {
            Expression& outExpr = outIter.current();

            currUse[&outExpr] = new vector<Expression*>;
        }
    }

    upsilon_search(&stmts[0]);
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
        vector<Expression*>* reachingDefs = reaching[lhp];
        reachingDefs->push_back(def);

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

        vector<Expression*>* reachingDefs = reaching[defStmt];

        if (reachingDefs->empty()) {
            find_reaching(phiChainLookInRef, defStmt, depth + 1);
        }

        for (vector<Expression*>::iterator reachingIter = reachingDefs->begin(); reachingIter != reachingDefs->end(); ++reachingIter) {
            Expression* reachingExpr = *reachingIter;

            Statement* reachingStmt = exprStmtLookup[reachingExpr];
            Statement* useStmt = exprStmtLookup[use];

            if (self[defStmt]) {
                // TODO
                add_dependence(reachingStmt, useStmt);
            } else {
                // TODO
                add_dependence(reachingStmt, useStmt);
            }
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

        add_dependence(defStmt, exprStmtLookup[use]);
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

    cout << "Self: " << endl;
    for (map<Statement*, bool>::iterator it = self.begin(); it != self.end(); ++it) {
        cout << it->first->tag() << ": " << boolalpha << it->second << endl;
    }

    cout << "Reaching: " << endl;
    for (map<Statement*, vector<Expression*>* >::iterator it = reaching.begin(); it != reaching.end(); ++it) {
        cout << it->first->tag() << ": ";

        vector<Expression*>* reachingDefs = it->second;

        for (vector<Expression*>::iterator itt = reachingDefs->begin(); itt != reachingDefs->end(); ++itt) {
            cout << **itt << endl;
        }
        cout << endl;
    }
}

void _test(Expression& expr, int& constItem, int& varItem, int coeff = 1) {
    switch (expr.op()) {
    case ID_OP:
        varItem = coeff;
        break;
    case INTEGER_CONSTANT_OP:
        constItem = coeff;
        break;
    case ADD_OP:
        _test(expr.arg_list()[0], constItem, varItem, coeff);
        _test(expr.arg_list()[1], constItem, varItem, coeff);
        break;
    case SUB_OP:
        _test(expr.arg_list()[0], constItem, varItem, coeff);
        _test(expr.arg_list()[1], constItem, varItem, -coeff);
        break;
    case MULT_OP:
        Expression& sub1 = expr.arg_list()[0];
        Expression& sub2 = expr.arg_list()[1];

        if (sub1.op() == INTEGER_CONSTANT_OP) {
            _test(sub2, constItem, varItem, coeff * sub1.value());
        } else {
            _test(sub1, constItem, varItem, coeff * sub2.value());
        }
    }
}

void gcd_test(Expression& e1, Expression& e2) {
    cout << "-------------------GCD test---------------------" << endl;

    Expression& arg1 = e1.arg_list()[0];
    Expression& arg2 = e2.arg_list()[0];

    int constItem1 = 0;
    int varItem1 = 0;
    _test(arg1, constItem1, varItem1);

    int constItem2 = 0;
    int varItem2 = 0;
    _test(arg2, constItem2, varItem2);
}

bool iterate_test(Expression& e1, Expression& e2, Expression& indexExpr, int from, int to, int step) {
    for (int i = from; i < to; i += step) {
        for (int j = from; j < to; j += step) {
            Expression* replaced1 = replace_ssa_expression(e1.clone(), &indexExpr, constant(i));
            Expression* replaced2 = replace_ssa_expression(e2.clone(), &indexExpr, constant(j));

            Expression* simplified1 = simplify(replaced1);
            Expression* simplified2 = simplify(replaced2);
//            cout << *simplified1 << " --- " << *simplified2 << endl;

            if (*simplified1 == *simplified2) {
                return true;
            }
        }
    }

    return false;
}

void find_array_dd_between_stmts(DEPENDENCE_TYPE dType, Statement& s1, Statement& s2, Expression& indexExpr, int from, int to, int step) {
    const RefSet<Expression>* e1Set = NULL;
    const RefSet<Expression>* e2Set = NULL;

    switch (dType) {
    case FLOW:
        e1Set = &s1.out_refs();
        e2Set = &s2.in_refs();
        break;
    case OUTPUT:
        e1Set = &s1.out_refs();
        e2Set = &s2.out_refs();
        break;
    case ANTI:
        e1Set = &s1.in_refs();
        e2Set = &s2.out_refs();
        break;
    }

    for (Iterator<Expression> e1Iter = *e1Set; e1Iter.valid(); ++e1Iter) {
        Expression& e1 = e1Iter.current();

        if (e1.op() != ARRAY_REF_OP) {
            continue;
        }

        Symbol* arrayBaseSym1 = e1.base_variable_ref();

        for (Iterator<Expression> e2Iter = *e2Set; e2Iter.valid(); ++e2Iter) {
            Expression& e2 = e2Iter.current();

            if (e2.op() != ARRAY_REF_OP) {
                continue;
            }

            Symbol* arrayBaseSym2 = e2.base_variable_ref();

            if (arrayBaseSym1 != arrayBaseSym2) {
                continue;
            }

            Expression& idx1 = e1.subscript();
            Expression& idx2 = e2.subscript();

//            gcd_test(idx1, idx2);

            if (iterate_test(idx1, idx2, indexExpr, from, to, step)) {
                cout << "Found matching for d-type " << dType << ": " << idx1 << " and " << idx2 << endl;
                add_dependence(&s1, &s2);
            }
        }
    }
}

void find_array_dd(ProgramUnit& pgm) {
    StmtList& stmts = pgm.stmts();

    for (Iterator<Statement> doStmtIter = stmts.stmts_of_type(DO_STMT); doStmtIter.valid(); ++doStmtIter) {
        Statement& doStmt = doStmtIter.current();
        
        map<Symbol*, Statement*> readLookup;
        map<Symbol*, Statement*> writeLookup;

        for (Iterator<Statement> stmtIter1 = stmts.iterate_loop_body(&doStmt); stmtIter1.valid(); ++stmtIter1) {
            Statement& stmt1 = stmtIter1.current();

            for (Iterator<Statement> stmtIter2 = stmts.iterate_loop_body(&doStmt); stmtIter2.valid(); ++stmtIter2) {
                Statement& stmt2 = stmtIter2.current();

                if (stmts.index(stmt1) > stmts.index(stmt2)) {
                    // stmt1 should come before stmt2
                    continue;
                }

                find_array_dd_between_stmts(FLOW, stmt1, stmt2, doStmt.index(), doStmt.init().value(), doStmt.limit().value(), doStmt.step().value());
                find_array_dd_between_stmts(OUTPUT, stmt1, stmt2, doStmt.index(), doStmt.init().value(), doStmt.limit().value(), doStmt.step().value());
//                find_array_dd_between_stmts(ANTI, stmt1, stmt2, doStmt.index(), doStmt.init().value(), doStmt.limit().value(), doStmt.step().value());
            }

//            for (Iterator<Expression> inExprIter = stmt1.in_refs(); inExprIter.valid(); ++inExprIter) {
//                Expression& inExpr = inExprIter.current();
//
//                if (inExpr.op() == ARRAY_REF_OP) {
//                    Symbol* arrayBaseSym = inExpr.base_variable_ref();
//
//                    readLookup[arrayBaseSym] = &stmt1;
//                }
//            }
//
//            for (Iterator<Expression> outExprIter = stmt1.out_refs(); outExprIter.valid(); ++outExprIter) {
//                Expression& outExpr = outExprIter.current();
//
//                if (outExpr.op() == ARRAY_REF_OP) {
//                    Symbol* arrayBaseSym = outExpr.base_variable_ref();
//
//                    writeLookup[arrayBaseSym] = &stmt1;
//                }
//            }
        }

//        for (map<Symbol*, Statement*>::iterator readIter = readLookup.begin(); readIter != readLookup.end(); ++readIter) {
//            Symbol* readSymbol = readIter->first;
//            Statement* readStmt = readIter->second;
//
//            cout << "read: " << *readSymbol << ": " << readStmt->tag() << endl;
//        }
    }
}

void ddtest(ProgramUnit & pgm)
{
    int a;

    preprocess(pgm);

    find_LHPs(pgm);

    build_fud_chains(pgm);

    find_scalar_dd(pgm);

    cout << "For arrays: ------------------------------------------" << endl;
    find_array_dd(pgm);



    cout << "------------------------------------------------------" << endl;
    build_frud_chains(pgm);
}

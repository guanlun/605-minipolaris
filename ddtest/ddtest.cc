// 
// ddtest.cc : Data Dependence Test pass
// 
#include <set>
#include <vector>
#include <map>
#include <sstream>

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
    DEPENDENCE_TYPE dType;
    Statement* dependedStmt;
    Statement* dependingStmt;
    string note;

    DataDependence(DEPENDENCE_TYPE type, Statement* depended, Statement* depending, string n = "") {
        dType = type;
        dependedStmt = depended;
        dependingStmt = depending;
        note = n;
    }

    void print(ostream& o) {
        o << "    " << dependedStmt->tag() << " to " << dependingStmt->tag() << " (" << note << ")" << endl;
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

set<Statement*> ddVisited;
map<Expression*, Expression*> fddChains;

map<Expression*, vector<Expression*>* > currUse;
map<Expression*, vector<Expression*>* > chainUse;
map<Expression*, vector<Expression*>* > saveChainUse;

set<DataDependence*> scalarFlowDependencies;
set<DataDependence*> scalarOutputDependencies;
set<DataDependence*> scalarAntiDependencies;

set<DataDependence*> arrayFlowDependencies;
set<DataDependence*> arrayOutputDependencies;
set<DataDependence*> arrayAntiDependencies;

struct LoopDependence {
    set<DataDependence*> scalarFlowDependencies;
    set<DataDependence*> scalarOutputDependencies;
    set<DataDependence*> scalarAntiDependencies;

    set<DataDependence*> arrayFlowDependencies;
    set<DataDependence*> arrayOutputDependencies;
    set<DataDependence*> arrayAntiDependencies;
};

map<string, LoopDependence*> loopDependences;

void add_scalar_dependence(string loopName, DEPENDENCE_TYPE dType, Statement* depended, Statement* depending, string note = "") {
    LoopDependence* ld = loopDependences[loopName];

    if (ld == NULL) {
        ld = new LoopDependence;
        loopDependences[loopName] = ld;
    }

    set<DataDependence*>* ddSet;

    switch (dType) {
    case FLOW:
        ddSet = &ld->scalarFlowDependencies;
        break;
    case OUTPUT:
        ddSet = &ld->scalarOutputDependencies;
        break;
    case ANTI:
        ddSet = &ld->scalarAntiDependencies;
        break;
    }

    ddSet->insert(new DataDependence(dType, depended, depending, note));
}

void add_array_dependence(string loopName, DEPENDENCE_TYPE dType, Statement* depended, Statement* depending, string note = "") {
    LoopDependence* ld = loopDependences[loopName];

    if (ld == NULL) {
        ld = new LoopDependence;
        loopDependences[loopName] = ld;
    }

    set<DataDependence*>* ddSet;

    switch (dType) {
    case FLOW:
        ddSet = &ld->arrayFlowDependencies;
        break;
    case OUTPUT:
        ddSet = &ld->arrayOutputDependencies;
        break;
    case ANTI:
        ddSet = &ld->arrayAntiDependencies;
        break;
    }

    ddSet->insert(new DataDependence(dType, depended, depending, note));
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

void find_scalar_anti_dd_between_stmts(string loopName, Statement& s1, Statement& s2) {
    Expression* def = get_def_expr(s2);

    if ((def == NULL) || (def->op() != ID_OP)) {
        return;
    }

    set<string> addedUseNames;

    for (Iterator<Expression> useIter = s1.in_refs(); useIter.valid(); ++useIter) {
        Expression& use = useIter.current();

        if (use.op() != ID_OP) {
            continue;
        }

        char* origUseName = orig_symbol_name(use.symbol());
        char* origDefName = orig_symbol_name(def->symbol());

        string origStr(origUseName);

        if (addedUseNames.find(origStr) != addedUseNames.end()) {
            // Already exists
            continue;
        }

        addedUseNames.insert(origStr);

        if (strcmp(origUseName, origDefName) == 0) {
            add_scalar_dependence(loopName, ANTI, &s1, &s2, "0");
        }
    }
}

void find_scalar_output_dd_between_stmts(string loopName, Statement& s1, Statement& s2) {
    Expression* def1 = get_def_expr(s1);
    Expression* def2 = get_def_expr(s2);

    if ((def1 == NULL) || (def2 == NULL) || (def1->op() != ID_OP) || (def2->op() != ID_OP)) {
        return;
    }

    const char* orig1 = orig_symbol_name(def1->symbol());
    const char* orig2 = orig_symbol_name(def2->symbol());

    if (strcmp(orig1, orig2) == 0) {
        add_scalar_dependence(loopName, OUTPUT, &s1, &s2, "0");
    }
}

void find_scalar_anti_and_output_dd(ProgramUnit& pgm) {
    StmtList& stmts = pgm.stmts();

    for (Iterator<Statement> doStmtIter = stmts.stmts_of_type(DO_STMT); doStmtIter.valid(); ++doStmtIter) {
        Statement& doStmt = doStmtIter.current();

        string loopName(doStmt.get_loop_name());

        map<Symbol*, Statement*> readLookup;
        map<Symbol*, Statement*> writeLookup;

        for (Iterator<Statement> stmtIter1 = stmts.iterate_loop_body(&doStmt); stmtIter1.valid(); ++stmtIter1) {
            Statement& stmt1 = stmtIter1.current();

            for (Iterator<Statement> stmtIter2 = stmts.iterate_loop_body(&doStmt); stmtIter2.valid(); ++stmtIter2) {
                Statement& stmt2 = stmtIter2.current();

                if (stmts.index(stmt1) < stmts.index(stmt2)) {
                    find_scalar_output_dd_between_stmts(loopName, stmt1, stmt2);
                    find_scalar_anti_dd_between_stmts(loopName, stmt1, stmt2);
                }
            }
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

            bool defFound = false;

            for (Iterator<Statement> defStmtIter = stmts; defStmtIter.valid(); ++defStmtIter) {
                Statement& defStmt = defStmtIter.current();

                Expression* defExpr = get_def_expr(defStmt);

                if (defExpr == NULL) {
                    continue;
                }

                if (*defExpr == useExpr) {
                    defFound = true;
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

            if (isPhi && !defFound) {
                Expression& phiTargetExpr = useStmt.lhs();
                vector<Expression*>* phiChain = phiChains[&phiTargetExpr];
                if (phiChain == NULL) {
                    phiChain = new vector<Expression*>;
                    phiChains[&phiTargetExpr] = phiChain;
                }

                phiChain->push_back(NULL);
            }
        }
    }
}

void find_reaching(Expression* def, Statement* lhp, int depth = 0) {
    if (markedReaching[def] == lhp) {
        return;
    }

    markedReaching[def] = lhp;

    Statement* defStmt = exprStmtLookup[def];

    if (defStmt == lhp) {
        self[lhp] = true;
    } else if (is_phi_stmt(*defStmt)) {
        vector<Expression*> phiChain = *phiChains[def];
        if (phiChain[0] != NULL) {
            find_reaching(phiChain[0], lhp, depth + 1);
        }

        if (phiChain[1] != NULL) {
            find_reaching(phiChain[1], lhp, depth + 1);
        }
    } else {
        vector<Expression*>* reachingDefs = reaching[lhp];
        reachingDefs->push_back(def);
    }
}

void find_dependence(Expression* def, Expression* use, int depth = 0) {
//    tab(depth);
//
//    cout << "Finding dependence for def " << *def << " and use " << *use << endl;

    if (marked[def] == use) {
        return;
    }

    marked[def] = use;

    Statement* defStmt = exprStmtLookup[def];
//    tab(depth);
//    cout << "def stmt: " << defStmt->tag() << endl;

    if (lhps.find(defStmt) != lhps.end()) {
//        tab(depth);
//        cout << "is loop header phi" << endl;

        vector<Expression*> phiChain = *phiChains[def];

        // The first element refers to the reference from outside the loop body and the second one refers the
        // reference defined inside the loop body.
        Expression* phiChainLoopOutRef = phiChain[0];
        Expression* phiChainLookInRef = phiChain[1];

        if (phiChainLoopOutRef != NULL) {
            find_dependence(phiChainLoopOutRef, use, depth + 1);
        }

        vector<Expression*>* reachingDefs = reaching[defStmt];

        if (reachingDefs->empty() && (phiChainLookInRef != NULL)) {
            find_reaching(phiChainLookInRef, defStmt, depth + 1);
        }

        for (vector<Expression*>::iterator reachingIter = reachingDefs->begin(); reachingIter != reachingDefs->end(); ++reachingIter) {
            Expression* reachingExpr = *reachingIter;

            Statement* reachingStmt = exprStmtLookup[reachingExpr];
            Statement* useStmt = exprStmtLookup[use];

            if (self[defStmt]) {
                add_scalar_dependence(get_stmt_loop_name(useStmt), FLOW, reachingStmt, useStmt, "<");
            } else {
                add_scalar_dependence(get_stmt_loop_name(useStmt), FLOW, reachingStmt, useStmt, "1");
            }
        }

    } else if (is_phi_stmt(*defStmt)) {
        vector<Expression*> phiChain = *phiChains[def];
        find_dependence(phiChain[0], use, depth + 1);
        find_dependence(phiChain[1], use, depth + 1);
    } else {
        Statement* useStmt = exprStmtLookup[use];
        add_scalar_dependence(get_stmt_loop_name(useStmt), FLOW, defStmt, useStmt, "0");
    }
}

void find_scalar_flow_dd(ProgramUnit& pgm) {
    for (set<Statement*>::iterator lhpIter = lhps.begin(); lhpIter != lhps.end(); ++lhpIter) {
        Statement* lhpStmt = *lhpIter;

        reaching[lhpStmt] = new vector<Expression*>;
        self[lhpStmt] = false;
    }

    for (set<Expression*>::iterator useExprIter = uses.begin(); useExprIter != uses.end(); ++useExprIter) {
        Expression* useExpr = *useExprIter;
    }

    for (set<Expression*>::iterator useExprIter = uses.begin(); useExprIter != uses.end(); ++useExprIter) {
        Expression* useExpr = *useExprIter;

        Expression* defExpr = fudChains[useExpr];

        if (defExpr != NULL) {
            find_dependence(defExpr, useExpr);
        }
    }

//    cout << "Data dependencies: " << endl;
//    for (set<DataDependence*>::iterator it = dependencies.begin(); it != dependencies.end(); ++it) {
//        DataDependence* dd = *it;
//
//        dd->print(cout);
//    }
//
//    cout << "Self: " << endl;
//    for (map<Statement*, bool>::iterator it = self.begin(); it != self.end(); ++it) {
//        cout << it->first->tag() << ": " << boolalpha << it->second << endl;
//    }
//
//    cout << "Reaching: " << endl;
//    for (map<Statement*, vector<Expression*>* >::iterator it = reaching.begin(); it != reaching.end(); ++it) {
//        cout << it->first->tag() << ": ";
//
//        vector<Expression*>* reachingDefs = it->second;
//
//        for (vector<Expression*>::iterator itt = reachingDefs->begin(); itt != reachingDefs->end(); ++itt) {
//            cout << **itt << endl;
//        }
//        cout << endl;
//    }
}

void gcd_test_helper(Expression& expr, int& constItem, int& varItem, int coeff = 1) {
    switch (expr.op()) {
    case ID_OP:
        varItem = coeff;
        break;
    case INTEGER_CONSTANT_OP:
        constItem = coeff;
        break;
    case ADD_OP:
        gcd_test_helper(expr.arg_list()[0], constItem, varItem, coeff);
        gcd_test_helper(expr.arg_list()[1], constItem, varItem, coeff);
        break;
    case SUB_OP:
        gcd_test_helper(expr.arg_list()[0], constItem, varItem, coeff);
        gcd_test_helper(expr.arg_list()[1], constItem, varItem, -coeff);
        break;
    case MULT_OP:
        Expression& sub1 = expr.arg_list()[0];
        Expression& sub2 = expr.arg_list()[1];

        if (sub1.op() == INTEGER_CONSTANT_OP) {
            gcd_test_helper(sub2, constItem, varItem, coeff * sub1.value());
        } else {
            gcd_test_helper(sub1, constItem, varItem, coeff * sub2.value());
        }
    }
}

bool gcd_test(Expression& e1, Expression& e2) {
    Expression& arg1 = e1.arg_list()[0];
    Expression& arg2 = e2.arg_list()[0];

    if ((arg1.op() == INTEGER_CONSTANT_OP) && (arg2.op() == INTEGER_CONSTANT_OP)) {
        return (arg1.value() == arg2.value());
    }

    int constItem1 = 0;
    int varItem1 = 0;
    gcd_test_helper(arg1, constItem1, varItem1);

    int constItem2 = 0;
    int varItem2 = 0;
    gcd_test_helper(arg2, constItem2, varItem2);

    int constItem = constItem1 - constItem2;
    int gcd = find_gcd(varItem1, varItem2);

    return (constItem % gcd == 0);
}

bool iterate_test(Expression& e1, Expression& e2, Expression& indexExpr, int from, int to, int step, bool& hasFlow, bool& hasAnti) {
    for (int i = from; i <= to; i += step) {
        for (int j = from; j <= to; j += step) {
            Expression* replaced1 = replace_ssa_expression(e1.clone(), &indexExpr, constant(i));
            Expression* replaced2 = replace_ssa_expression(e2.clone(), &indexExpr, constant(j));

            Expression* simplified1 = simplify(replaced1);
            Expression* simplified2 = simplify(replaced2);

            if (*simplified1 == *simplified2) {
                if (i < j) {
                    hasFlow = true;
                    return true;
                } else if (i > j) {
                    hasAnti = true;
                    return true;
                }
            }
        }
    }

    return false;
}

void find_array_dd_between_stmts(string loopName, DEPENDENCE_TYPE dType, Statement& s1, Statement& s2, Expression& indexExpr, int from, int to, int step) {
    const RefSet<Expression>* e1Set = NULL;
    const RefSet<Expression>* e2Set = NULL;

    if (dType == OUTPUT) {
        e1Set = &s1.out_refs();
        e2Set = &s2.out_refs();
    } else {
        e1Set = &s1.out_refs();
        e2Set = &s2.in_refs();
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

            gcd_test(idx1, idx2);

            bool hasFlowDependence = false, hasAntiDependence = false;
            if (iterate_test(idx1, idx2, indexExpr, from, to, step, hasFlowDependence, hasAntiDependence)) {
                stringstream ss;
                e1.print(ss);
                ss << " and ";
                e2.print(ss);

                if (dType == OUTPUT) {
                    add_array_dependence(loopName, OUTPUT, &s1, &s2, ss.str());
                } else {
                    if (hasFlowDependence) {
                        add_array_dependence(loopName, FLOW, &s1, &s2, ss.str());
                    }

                    if (hasAntiDependence) {
                        add_array_dependence(loopName, ANTI, &s1, &s2, ss.str());
                    }
                }
            }
        }
    }
}

void find_array_dd(ProgramUnit& pgm) {
    StmtList& stmts = pgm.stmts();

    for (Iterator<Statement> doStmtIter = stmts.stmts_of_type(DO_STMT); doStmtIter.valid(); ++doStmtIter) {
        Statement& doStmt = doStmtIter.current();
        
        string loopName(doStmt.get_loop_name());

        map<Symbol*, Statement*> readLookup;
        map<Symbol*, Statement*> writeLookup;

        for (Iterator<Statement> stmtIter1 = stmts.iterate_loop_body(&doStmt); stmtIter1.valid(); ++stmtIter1) {
            Statement& stmt1 = stmtIter1.current();

            for (Iterator<Statement> stmtIter2 = stmts.iterate_loop_body(&doStmt); stmtIter2.valid(); ++stmtIter2) {
                Statement& stmt2 = stmtIter2.current();
                find_array_dd_between_stmts(loopName, FLOW, stmt1, stmt2, doStmt.index(), doStmt.init().value(), doStmt.limit().value(), doStmt.step().value());

                if (stmts.index(stmt1) < stmts.index(stmt2)) {
                    find_array_dd_between_stmts(loopName, OUTPUT, stmt1, stmt2, doStmt.index(), doStmt.init().value(), doStmt.limit().value(), doStmt.step().value());
                }
            }
        }
    }
}

void print_dd(bool forArray, bool flow, bool output, bool anti) {
    for (map<string, LoopDependence*>::iterator ldIter = loopDependences.begin(); ldIter != loopDependences.end(); ++ldIter) {
        string loopName = ldIter->first;
        LoopDependence* ld = ldIter->second;

        cout << "LOOP " << loopName << " contains the following dependencies : " << endl;

        if (flow) {
            set<DataDependence*>* ddSet = forArray ? &ld->arrayFlowDependencies : &ld->scalarFlowDependencies;

            cout << "  Flow : " << endl;
            for (set<DataDependence*>::iterator ddIter = ddSet->begin(); ddIter != ddSet->end(); ++ddIter) {
                DataDependence* dd = *ddIter;

                dd->print(cout);
            }
        }

        if (output) {
            set<DataDependence*>* ddSet = forArray ? &ld->arrayOutputDependencies : &ld->scalarOutputDependencies;

            cout << "  Output : " << endl;
            for (set<DataDependence*>::iterator ddIter = ddSet->begin(); ddIter != ddSet->end(); ++ddIter) {
                DataDependence* dd = *ddIter;

                dd->print(cout);
            }
        }

        if (anti) {
            set<DataDependence*>* ddSet = forArray ? &ld->arrayAntiDependencies : &ld->scalarAntiDependencies;

            cout << "  Anti : " << endl;
            for (set<DataDependence*>::iterator ddIter = ddSet->begin(); ddIter != ddSet->end(); ++ddIter) {
                DataDependence* dd = *ddIter;

                dd->print(cout);
            }
        }
    }
}

void ddtest(ProgramUnit & pgm)
{
    // TODO: remove
    int a;

    preprocess(pgm);

    find_LHPs(pgm);

    build_fud_chains(pgm);

    find_scalar_flow_dd(pgm);

    find_scalar_anti_and_output_dd(pgm);

    find_array_dd(pgm);
}

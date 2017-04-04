// 
// ssa.cc : SSA transformations
// 

#include "ssa.h"
#include "StmtList.h"
#include "Collection/List.h"
#include "Statement/Statement.h"
#include "Statement/AssignmentStmt.h"
#include "Expression/FunctionCallExpr.h"
#include "Expression/IDExpr.h"
#include "Expression/BinaryExpr.h"
#include "Expression/expr_funcs.h"
#include "Symbol/VariableSymbol.h"

#include <sstream>
#include <queue>
#include <vector>
#include <map>

int PASS_TAG = 3;

template <class T>
void set_intersect(set<T>& s1, set<T>& s2) {
    for (typename set<T>::iterator it = s1.begin(); it != s1.end(); ++it) {
        if (s2.count(*it) == 0) {
            s1.erase(it);
        }
    }
}

template <class T>
bool set_equal(set<T>& s1, set<T>& s2) {
    if (s1.size() != s2.size()) {
        return false;
    }

    for (typename set<T>::iterator it = s1.begin(); it != s1.end(); ++it) {
        if (s2.count(*it) == 0) {
            return false;
        }
    }

    return true;
}

bool is_stmt_with_named_func(Statement& stmt, const char* name) {
    if (stmt.stmt_class() != ASSIGNMENT_STMT) {
        return false;
    }

    const Expression& rhs = stmt.rhs();

    if (rhs.op() != FUNCTION_CALL_OP) {
        return false;
    }

    const Expression& func = rhs.function();

    return (strcmp(func.symbol().name_ref(), name) == 0);
}

bool is_upsilon_stmt(Statement& stmt) {
    return is_stmt_with_named_func(stmt, "UPSILON");
}

bool is_phi_stmt(Statement& stmt) {
    return is_stmt_with_named_func(stmt, "PHI");
}

bool is_phi_stmt_for_function(Statement& stmt) {
    if (!is_phi_stmt(stmt)) {
        return false;
    }

    SSAWorkSpace* ws = (SSAWorkSpace*)stmt.work_stack().top_ref(PASS_TAG);

    if (!ws) {
        return false;
    }

    return ws->isPhiForFunction;
}

void compute_dominance(ProgramUnit& pgm, List<BasicBlock>* basicBlocks) {
    cout << 11 << endl;
    for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
        BasicBlock& bb = bbIter.current();

        for (Iterator<BasicBlock> domIter = *basicBlocks; domIter.valid(); ++domIter) {
            BasicBlock& initBB = domIter.current();

            bb.dominators.insert(&initBB);
        }
    }
    cout << 22 << endl;

    queue<BasicBlock*> workList;
    workList.push(basicBlocks->first_ref());

    while (!workList.empty()) {
        cout << workList.size() << endl;
        BasicBlock* currNode = workList.front();
        workList.pop();

        set<BasicBlock*> newDominators;

        cout << "a" << endl;
        for (int predIdx = 0; predIdx < currNode->predecessors.entries(); predIdx++) {
            BasicBlock& predBB = currNode->predecessors[predIdx];

            if (predIdx == 0) {
                newDominators = predBB.dominators;
            } else {
                set_intersect(newDominators, predBB.dominators);
            }
        }
        cout << "b" << endl;

        newDominators.insert(currNode);

        if (!set_equal(newDominators, currNode->dominators)) {
            currNode->dominators = newDominators;

            for (int succIdx = 0; succIdx < currNode->successors.entries(); succIdx++) {
                BasicBlock& succBB = currNode->successors[succIdx];

                workList.push(&succBB);
            }
        }
        cout << "c" << endl;
    }
    cout << "ooo" << endl;

    for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
        BasicBlock& bb = bbIter.current();

        BasicBlock* runner = &bb;
        while (runner != NULL) {
            cout << runner << endl;
            RefList<BasicBlock&>& predBBs = runner->predecessors;
            if (predBBs.entries() == 0) {
                break;
            }

            runner = &predBBs[0];

            if (bb.dominators.count(runner) > 0) {
                break;
            }
        }
        cout << "out of loop" << endl;

        if (&bb != runner) {
            bb.immediateDominator = runner;
        }
    }

    for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
        BasicBlock& bb = bbIter.current();
        RefList<BasicBlock&>& predBBs = bb.predecessors;

        if (predBBs.entries() >= 2) {
            // Only a merge-point could be a dominance frontier of other nodes

            for (int predIdx = 0; predIdx < predBBs.entries(); predIdx++) {
                BasicBlock& predBB = predBBs[predIdx];

                BasicBlock* runner = &predBB;

                while ((runner != NULL) && (runner != bb.immediateDominator)) {
                    cout << *runner << endl;
                    runner->dominanceFrontiers.insert(&bb);
                    runner = runner->immediateDominator;
                }
            }
        }
    }

    for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
        BasicBlock& bb = bbIter.current();

        if (bb.immediateDominator) {
            bb.immediateDominator->dominants.insert(&bb);
        }
    }
}

void find_function_helper(set<Expression*>& funcExprs, Expression& expr) {
    if (expr.op() == FUNCTION_CALL_OP) {
        funcExprs.insert(&expr);
    } else {
        for (Iterator<Expression> exprIter = expr.arg_list(); exprIter.valid(); ++exprIter) {
            find_function_helper(funcExprs, exprIter.current());
        }
    }
}

set<Expression*> find_functions(Statement& stmt) {
    set<Expression*> exprList;

    for (Iterator<Expression> exprIter = stmt.iterate_all_expressions();
        exprIter.valid();
        ++exprIter) {
        Expression& expr = exprIter.current();

        find_function_helper(exprList, expr);
    }

    return exprList;
}

void build_phi_for_function(
    Expression& argsCommaExpr,
    Expression* phiFunc,
    StmtList& stmts,
    Statement& stmt,
    BasicBlock& bb) {
    for (Iterator<Expression> argIter = argsCommaExpr.arg_list(); argIter.valid(); ++argIter) {
        Expression& argExpr = argIter.current();

        if (argExpr.op() != ID_OP) {
            continue;
        }

        Symbol& phiAssignedSymbol = argExpr.symbol();

        Expression* phiArgs = comma();
        Expression* phiFuncExpr = function_call(phiFunc->clone(), phiArgs);
        Expression* assignedExpr = new IDExpr(phiAssignedSymbol.type(), phiAssignedSymbol);

        Statement* phiStmt = new AssignmentStmt(stmts.new_tag(), assignedExpr, phiFuncExpr);
        phiStmt->work_stack().push(new SSAWorkSpace(PASS_TAG, true));

        stmts.ins_after(phiStmt, &stmt);
        bb.stmts.ins_after(*phiStmt, stmt);
    }
}

void generate_upsilon_stmts(ProgramUnit& pgm, List<BasicBlock>* basicBlocks) {
    Expression* upsilonFunc = new_function("UPSILON", make_type(INTEGER_TYPE, 4), pgm);

    StmtList& stmts = pgm.stmts();

    map<Statement*, Statement*> toBeInserted;

    for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
        BasicBlock& bb = bbIter.current();

        for (int stmtIdx = 0; stmtIdx < bb.stmts.entries(); stmtIdx++) {
            Statement& stmt = bb.stmts[stmtIdx];

            if (is_phi_stmt(stmt) && !is_phi_stmt_for_function(stmt)) {
                Expression& assignedPhiExpr = stmt.lhs();
                Expression& phiFuncArgs = stmt.rhs().parameters_guarded();

                for (Iterator<Statement> rStmtIter = stmts; rStmtIter.valid(); ++rStmtIter) {
                    Statement& rStmt = rStmtIter.current();

                    for (Iterator<Expression> outRefIter = rStmt.out_refs(); outRefIter.valid(); ++outRefIter) {
                        Expression& outRefExpr = outRefIter.current();

                        for (Iterator<Expression> argIter = phiFuncArgs.arg_list(); argIter.valid(); ++argIter) {
                            Expression& argExpr = argIter.current();

                            if (outRefExpr == argExpr) {
                                Symbol& assignedSymbol = outRefExpr.symbol();

                                Expression* upsilonArgs = comma(assignedPhiExpr.clone());
                                Expression* upsilonFuncExpr = function_call(upsilonFunc->clone(), upsilonArgs);
                                Expression* assignedExpr = new IDExpr(assignedSymbol.type(), assignedSymbol);

                                Statement* upsilonStmt = new AssignmentStmt(stmts.new_tag(), assignedExpr, upsilonFuncExpr);

                                toBeInserted.insert(pair<Statement*, Statement*>(upsilonStmt, &rStmt));
                            }
                        }
                    }
                }
            }
        }
    }

    for (map<Statement*, Statement*>::iterator insertIter = toBeInserted.begin();
        insertIter != toBeInserted.end();
        ++insertIter) {
        stmts.ins_after(insertIter->first, insertIter->second);
    }
}

void generate_phi_stmts(ProgramUnit& pgm, List<BasicBlock>* basicBlocks) {
    StmtList& stmts = pgm.stmts();

    for (DictionaryIter<Symbol> symIter = pgm.symtab().iterator(); symIter.valid(); ++symIter) {
        Symbol& sym = symIter.current();
        set<BasicBlock*> workList;
        set<BasicBlock*> added;

        for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
            BasicBlock& bb = bbIter.current();

            for (int stmtIdx = 0; stmtIdx < bb.stmts.entries(); stmtIdx++) {
                Statement& stmt = bb.stmts[stmtIdx];
                Iterator<Expression> outIter = stmt.out_refs();

                if (!outIter.valid()) {
                    continue;
                }

                Expression& outExpr = outIter.current();

                if (outExpr.op() != ID_OP) {
                    continue;
                }

                Symbol& assignee = outExpr.symbol();

                if (&assignee == &sym) {
                    workList.insert(&bb);
                }
            }
        }

        while (!workList.empty()) {
            BasicBlock* currNode = *workList.begin();
            workList.erase(currNode);

            for (set<BasicBlock*>::iterator dfIter = currNode->dominanceFrontiers.begin();
                 dfIter != currNode->dominanceFrontiers.end();
                 ++dfIter) {
                BasicBlock* dfNode = *dfIter;

                dfNode->phiSymbols.insert(&sym);

                if (added.count(dfNode) == 0) {
                    workList.insert(dfNode);
                    added.insert(dfNode);
                }
            }
        }
    }

    Expression* phiFunc = new_function("PHI", make_type(INTEGER_TYPE, 4), pgm);

    // Insert PHI statements after function calls for each of the arguments
    for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
        BasicBlock& bb = bbIter.current();

        for (int stmtIdx = 0; stmtIdx < bb.stmts.entries(); stmtIdx++) {
            Statement& stmt = bb.stmts[stmtIdx];

            if (is_phi_stmt(stmt)) {
                // Already a PHI statement, ignore this stmt
                continue;
            }

            if (stmt.stmt_class() == CALL_STMT) {
                build_phi_for_function(stmt.parameters_guarded(), phiFunc, stmts, stmt, bb);
            } else {
                set<Expression*> functionCallExprs = find_functions(stmt);

                for (set<Expression*>::iterator funcExprIter = functionCallExprs.begin();
                    funcExprIter != functionCallExprs.end();
                    ++funcExprIter) {
                    Expression* funcExpr = *funcExprIter;

                    build_phi_for_function(funcExpr->parameters_guarded(), phiFunc, stmts, stmt, bb);
                }
            }
        }
    }

    for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
        BasicBlock& bb = bbIter.current();

        for (set<Symbol*>::iterator phiIter = bb.phiSymbols.begin();
            phiIter != bb.phiSymbols.end();
            ++phiIter) {
            Symbol* phiSymbol = *phiIter;

            Expression* args = comma();
            Expression* phiFuncExpr = function_call(phiFunc->clone(), args);
            Expression* assignedExpr = new IDExpr(phiSymbol->type(), *phiSymbol);

            Statement* phiStmt = new AssignmentStmt(stmts.new_tag(), assignedExpr, phiFuncExpr);

            stmts.ins_after(phiStmt, &bb.stmts[0]);
            bb.stmts.ins_after(*phiStmt, bb.stmts[0]);
        }
    }
}

const char* new_name(Symbol* sym, map<Symbol*, vector<int> >& variableNumLookup, map<Symbol*, int>& counter) {
    map<Symbol*, vector<int> >::iterator vnIter = variableNumLookup.find(sym);

    if (vnIter == variableNumLookup.end()) {
        // Not found, return original symbol name
        return sym->name_ref();
    }

    map<Symbol*, int>::iterator currCountIter = counter.find(sym);

    vector<int>& numStack = vnIter->second;
    int currCount = currCountIter->second;

    int newCount = currCount + 1;

    currCountIter->second = newCount;

    numStack.push_back(newCount);

    stringstream ss;
    ss << sym->name_ref() << "@" << newCount;

    return ss.str().c_str();
}

const char* current_name(Symbol* sym, map<Symbol*, vector<int> >& variableNumLookup) {

    map<Symbol*, vector<int> >::iterator vnIter = variableNumLookup.find(sym);

    if (vnIter == variableNumLookup.end()) {
        // Not found, return original symbol name
        return sym->name_ref();
    }

    vector<int>& numStack = vnIter->second;

    stringstream ss;
    ss << sym->name_ref() << "@" << numStack.back();

    return ss.str().c_str();
}

char* orig_symbol_name(Symbol& symbol) {
    const char* symbolName = symbol.name_ref();
    char* origName = new char[strlen(symbolName)];

    strcpy(origName, symbolName);

    origName = strtok(origName, "@");

    return origName;
}

void populate_phi_args(Statement& phiStmt, map<Symbol*, vector<int> >& variableNumLookup) {
    if (is_phi_stmt_for_function(phiStmt)) {
        // No need to populate phi statements for function again
        return;
    }

    Expression& lhs = phiStmt.lhs();
    Symbol& phiSymbol = lhs.symbol();

    Expression& phiFunc = phiStmt.rhs();
    Expression& phiParams = phiFunc.parameters_guarded();

    char* origName = orig_symbol_name(phiSymbol);

    for (map<Symbol*, vector<int> >::iterator mapIter = variableNumLookup.begin();
         mapIter != variableNumLookup.end();
         ++mapIter) {
        Symbol* key = mapIter->first;

        if (strcmp(key->name_ref(), origName) == 0) {
            const char* phiArgName = current_name(key, variableNumLookup);
            Symbol* phiArgSymbol = key->clone();
            phiArgSymbol->name(phiArgName);

            Expression* argExpr = new IDExpr(phiArgSymbol->type(), *phiArgSymbol);

            bool alreadyInserted = false;
            for (Iterator<Expression> currArgIter = phiParams.arg_list();
                currArgIter.valid();
                ++currArgIter) {
                Expression& currArgExpr = currArgIter.current();

                if (currArgExpr == (*argExpr)) {
                    alreadyInserted = true;
                    break;
                }
            }

            if (!alreadyInserted) {
                phiParams.arg_list().ins_last(argExpr);
            }
        }
    }
}

void variable_renaming_helper(
        ProgramUnit& pgm,
        BasicBlock* bb,
        map<Symbol*, vector<int> >& variableNumLookup,
        map<Symbol*, int>& counter,
        set<BasicBlock*>& visited,
        int depth = 0) {

    if (visited.count(bb) > 0) {
        return;
    }

    visited.insert(bb);

    for (int stmtIdx = 0; stmtIdx < bb->stmts.entries(); stmtIdx++) {
        Statement& stmt = bb->stmts[stmtIdx];

        if (is_phi_stmt_for_function(stmt)) {
            Symbol& assignedSymbol = stmt.lhs().symbol();

            Expression& phiFuncExpr = stmt.rhs();
            Expression& paramCommaExpr = phiFuncExpr.parameters_guarded();

            const char* origName = current_name(&assignedSymbol, variableNumLookup);
            Symbol* origVarSymbol = assignedSymbol.clone();
            origVarSymbol->name(origName);
            Expression* origParamExpr = new IDExpr(origVarSymbol->type(), *origVarSymbol);

            const char* alteredName = new_name(&assignedSymbol, variableNumLookup, counter);
            Symbol* alteredVarSymbol = assignedSymbol.clone();
            alteredVarSymbol->name(alteredName);
            Expression* alteredParamExpr = new IDExpr(alteredVarSymbol->type(), *alteredVarSymbol);

            paramCommaExpr.arg_list().ins_last(origParamExpr);
            paramCommaExpr.arg_list().ins_last(alteredParamExpr);

            const char* phiResultName = new_name(&assignedSymbol, variableNumLookup, counter);
            Symbol* phiResultVarSymbol = assignedSymbol.clone();
            phiResultVarSymbol->name(phiResultName);
            stmt.lhs().symbol(*phiResultVarSymbol);

            continue;
        }

        for (Iterator<Expression> inRefIter = stmt.in_refs(); inRefIter.valid(); ++inRefIter) {
            Expression& inRefExpr = inRefIter.current();

            if (inRefExpr.op() != ID_OP) {
                continue;
            }

            Symbol& inRefSymbol = inRefExpr.symbol();
            const char* varName = current_name(&inRefSymbol, variableNumLookup);

            Symbol* renamedSymbol = inRefSymbol.clone();
            renamedSymbol->name(varName);

            inRefExpr.symbol(*renamedSymbol);
        }

        for (Iterator<Expression> outRefMut = stmt.out_refs(); outRefMut.valid(); ++outRefMut) {
            Expression& outRefExpr = outRefMut.current();

            // Check whether an out-ref is also an in-ref (which occurs for function parameters)
            bool isAlreadyInRef = false;
            for (Iterator<Expression> inRefIter = stmt.in_refs(); inRefIter.valid(); ++inRefIter) {
                Expression& inRefExpr = inRefIter.current();

                if (&inRefExpr == &outRefExpr) {
                    isAlreadyInRef = true;
                    break;
                }
            }

            if (isAlreadyInRef) {
                continue;
            }

            if (outRefExpr.op() != ID_OP) {
                continue;
            }

            Symbol& outRefSymbol = outRefExpr.symbol();
            const char* newVarName = new_name(&outRefSymbol, variableNumLookup, counter);

            Symbol* renamedSymbol = outRefSymbol.clone();
            renamedSymbol->name(newVarName);

            outRefExpr.symbol(*renamedSymbol);
        }
    }

    // Iterate each successor in the CFG
    for (int succIdx = 0; succIdx < bb->successors.entries(); succIdx++) {
        BasicBlock& succBB = bb->successors[succIdx];

        for (int stmtIdx = 0; stmtIdx < succBB.stmts.entries(); stmtIdx++) {
            Statement& stmt = succBB.stmts[stmtIdx];

            if (!is_phi_stmt(stmt)) {
                continue;
            }

            populate_phi_args(stmt, variableNumLookup);
        }
    }

    // Iterate each child in the dominator tree
    for (set<BasicBlock*>::iterator domIter = bb->dominants.begin();
         domIter != bb->dominants.end();
         ++domIter) {
        BasicBlock* dominantBB = *domIter;

        variable_renaming_helper(pgm, dominantBB, variableNumLookup, counter, visited, depth + 1);
    }

    // Pop the stacks
    for (int stmtIdx = 0; stmtIdx < bb->stmts.entries(); stmtIdx++) {
        Statement& stmt = bb->stmts[stmtIdx];

        for (Iterator<Expression> outRefMut = stmt.out_refs(); outRefMut.valid(); ++outRefMut) {
            Expression& outRefExpr = outRefMut.current();

            if (outRefExpr.op() != ID_OP) {
                continue;
            }

            Symbol& outRefSymbol = outRefExpr.symbol();

            // Here we scan the entire map for the original symbol as we cannot lookup directly
            for (map<Symbol*, vector<int> >::iterator vnIter = variableNumLookup.begin();
                vnIter != variableNumLookup.end();
                ++vnIter) {
                Symbol* origSymbol = vnIter->first;
                if (strcmp(origSymbol->name_ref(), orig_symbol_name(outRefSymbol)) == 0) {
                    vector<int>& numStack = vnIter->second;

                    numStack.pop_back();
                    break;
                }
            }
        }
    }
}

void rename_variables(ProgramUnit& pgm, List<BasicBlock>* basicBlocks) {
    set<BasicBlock*> visited;
    map<Symbol*, int> counter;
    map<Symbol*, vector<int> > variableNumLookup;

    for (DictionaryIter<Symbol> symIter = pgm.symtab().iterator(); symIter.valid(); ++symIter) {
        Symbol& symbol = symIter.current();

        vector<int> numStack;
        numStack.push_back(0);
        counter.insert(pair<Symbol*, int>(&symbol, 0));

        variableNumLookup.insert(pair<Symbol*, vector<int> >(&symbol, numStack));
    }

    Iterator<BasicBlock> startIter = *basicBlocks;

    variable_renaming_helper(pgm, &startIter.current(), variableNumLookup, counter, visited);
}

void ssa(ProgramUnit & pgm, List<BasicBlock>* basicBlocks)
{
    compute_dominance(pgm, basicBlocks);
    generate_phi_stmts(pgm, basicBlocks);
    rename_variables(pgm, basicBlocks);
    // generate_upsilon_stmts(pgm, basicBlocks);
}

void restore_variable_name(Expression& expr) {
    if (expr.op() != ID_OP) {
        return;
    }

    Symbol& outRefSymbol = expr.symbol();
    const char* varName = outRefSymbol.name_ref();

    if (strchr(varName, '@')) {
        char* origName = orig_symbol_name(outRefSymbol);
        Symbol* renamedSymbol = outRefSymbol.clone();
        renamedSymbol->name(origName);

        expr.symbol(*renamedSymbol);
    }
}

void dessa(ProgramUnit & pgm)
{
    StmtList& stmts = pgm.stmts();

    for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
        Statement& stmt = stmtIter.current();

        if (is_phi_stmt(stmt) || is_upsilon_stmt(stmt)) {
            stmts.del(stmt);
            continue;
        }

        for (Iterator<Expression> outRefIter = stmt.out_refs(); outRefIter.valid(); ++outRefIter) {
            Expression& outRefExpr = outRefIter.current();

            restore_variable_name(outRefExpr);
        }

        for (Iterator<Expression> inRefIter = stmt.in_refs(); inRefIter.valid(); ++inRefIter) {
            Expression& inRefExpr = inRefIter.current();

            restore_variable_name(inRefExpr);
        }
    }
}

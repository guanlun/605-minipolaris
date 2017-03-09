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
#include "Expression/expr_funcs.h"
#include "Symbol/VariableSymbol.h"

#include <sstream>
#include <queue>
#include <vector>
#include <map>

int PASS_TAG = 3;

void per_stmt_operation(Iterator<Statement> stmts, void (*forEachFunction)(Statement&)) {
    for (Iterator<Statement> stmtIter = stmts; stmtIter.valid(); ++stmtIter) {
        Statement& stmt = stmtIter.current();
        forEachFunction(stmt);
    }
}

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

bool is_phi_stmt(Statement& stmt) {
    if (stmt.stmt_class() != ASSIGNMENT_STMT) {
        return false;
    }

    const Expression& rhs = stmt.rhs();

    if (rhs.op() != FUNCTION_CALL_OP) {
        return false;
    }

    const Expression& func = rhs.function();

    return (strcmp(func.symbol().name_ref(), "PHI") == 0);
}

void link_dominants(Statement& stmt) {
}

void find_immediate_dominator(Statement& stmt) {
}

void find_dominance_frontier(Statement& stmt) {
}

void find_phi_insertion_points(Statement& stmt) {
}

void insert_phi_stmts(Statement& stmt) {
}

void compute_dominance(ProgramUnit& pgm, List<BasicBlock>* basicBlocks) {
    for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
        BasicBlock& bb = bbIter.current();

        for (Iterator<BasicBlock> domIter = *basicBlocks; domIter.valid(); ++domIter) {
            BasicBlock& initBB = domIter.current();

            bb.dominators.insert(&initBB);
        }
    }

    queue<BasicBlock*> workList;
    workList.push(basicBlocks->first_ref());

    while (!workList.empty()) {
        BasicBlock* currNode = workList.front();
        workList.pop();

        set<BasicBlock*> newDominators;

        for (int predIdx = 0; predIdx < currNode->predecessors.entries(); predIdx++) {
            BasicBlock& predBB = currNode->predecessors[predIdx];

            if (predIdx == 0) {
                newDominators = predBB.dominators;
            } else {
                set_intersect(newDominators, predBB.dominators);
            }

            predIdx++;
        }

        newDominators.insert(currNode);

        if (!set_equal(newDominators, currNode->dominators)) {
            currNode->dominators = newDominators;

            for (int succIdx = 0; succIdx < currNode->successors.entries(); succIdx++) {
                BasicBlock& succBB = currNode->successors[succIdx];

                workList.push(&succBB);
            }
        }
    }

    for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
        BasicBlock& bb = bbIter.current();

        BasicBlock* runner = &bb;
        while (runner != NULL) {
            RefList<BasicBlock&>& predBBs = runner->predecessors;
            if (predBBs.entries() == 0) {
                break;
            }

            runner = &predBBs[0];

            if (bb.dominators.count(runner) > 0) {
                break;
            }
        }

        bb.immediateDominator = runner;
    }

    for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
        BasicBlock& bb = bbIter.current();

        RefList<BasicBlock&>& predBBs = bb.predecessors;

        if (predBBs.entries() >= 2) {
            // Only a merge-point could be a dominance frontier of other nodes

            for (int predIdx = 0; predIdx < predBBs.entries(); predIdx++) {
                BasicBlock& predBB = predBBs[predIdx];

                BasicBlock* runner = &predBB;

                while (runner != bb.immediateDominator) {
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

    for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
        BasicBlock& bb = bbIter.current();

        cout << bb << endl;
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

//                cout << sym.tag_ref() << " is added to " << dfNode->name << endl;
            }
        }
    }

    Expression* phiFunc = new_function("PHI", make_type(INTEGER_TYPE, 4), pgm);

    for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
        BasicBlock& bb = bbIter.current();

        for (set<Symbol*>::iterator phiIter = bb.phiSymbols.begin();
             phiIter != bb.phiSymbols.end();
             ++phiIter) {
            Symbol* phiSymbol = *phiIter;

            /*
            // Create placeholder argument list using constants
            int predCount = bb.predecessors.entries();
            List<Expression>* argList = new List<Expression>();
            for (int predIdx = 0; predIdx < predCount; predIdx++) {
                argList->ins_last(constant(0));
            }
            */

            Expression* args = comma();
            Expression* phiFuncExpr = function_call(phiFunc->clone(), args);
            Expression* assignedExpr = new IDExpr(phiSymbol->type(), *phiSymbol);

            Statement* phiStmt = new AssignmentStmt(stmts.new_tag(), assignedExpr, phiFuncExpr);

            stmts.ins_after(phiStmt, &bb.stmts[0]);
            bb.stmts.ins_after(*phiStmt, bb.stmts[0]);
        }
    }

    for (Iterator<BasicBlock> bbIter = *basicBlocks; bbIter.valid(); ++bbIter) {
        BasicBlock& bb = bbIter.current();

//        cout << bb << endl;
    }
}

const char* new_name(Symbol* sym, map<Symbol*, vector<int> >& variableNumLookup, map<Symbol*, int>& counter) {
    map<Symbol*, vector<int> >::iterator vnIter = variableNumLookup.find(sym);
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
    Expression& lhs = phiStmt.lhs();
    Symbol& phiSymbol = lhs.symbol();

    Expression& phiFunc = phiStmt.rhs();
    Expression& phiParams = phiFunc.parameters_guarded();

    char* origName = orig_symbol_name(phiSymbol);

    for (map<Symbol*, vector<int> >::iterator mapIter = variableNumLookup.begin();
         mapIter != variableNumLookup.end();
         ++mapIter) {
        Symbol* key = mapIter->first;

        if (strcmp(orig_symbol_name(*key), origName) == 0) {
            const char* phiArgName = current_name(key, variableNumLookup);
            Symbol* phiArgSymbol = key->clone();
            phiArgSymbol->name(phiArgName);

            Expression* argExpr = new IDExpr(phiArgSymbol->type(), *phiArgSymbol);
            phiParams.arg_list().ins_last(argExpr);

            /*
            for (Mutator<Expression> argIter = phiArgs; argIter.valid(); ++argIter) {
                Expression& argExpr = argIter.current();
                cout << "arg: " << argExpr << endl;

                if (argExpr.op() == INTEGER_CONSTANT_OP) {
                    cout << "is constant!!!!!!!!!!!!!!!!!!" << endl;
                    argIter.assign() = constant(1);
                    break;
                }
            }
            */
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

    for (int i = 0; i < depth; i++) {
        cout << "    ";
    }

    cout << "Now visited bb: " << bb->name << endl;

    visited.insert(bb);

    for (int stmtIdx = 0; stmtIdx < bb->stmts.entries(); stmtIdx++) {
        Statement& stmt = bb->stmts[stmtIdx];

        for (Iterator<Expression> inRefIter = stmt.in_refs(); inRefIter.valid(); ++inRefIter) {
            Expression& inRefExpr = inRefIter.current();

            // TODO: Consider array later
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

            // TODO: Consider array later
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

        cout << "in succ " << succBB.name << " with phi #: " << succBB.phiSymbols.size() << endl;

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

    // TODO: pop the stack
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
}

void dessa(ProgramUnit & pgm)
{

}

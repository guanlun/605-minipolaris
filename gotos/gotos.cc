// 
// gotos.cc : convert gotos pass
// 

#include "gotos.h"
#include "StmtList.h"
#include "Collection/List.h"
#include "Statement/Statement.h"
#include "Statement/IfStmt.h"
#include "Statement/EndIfStmt.h"
#include "Statement/GotoStmt.h"
#include "Expression/BinaryExpr.h"
#include "utilities/precalc_util.h"

using std::cout;
using std::endl;

void convert_arithmetic_ifs(ProgramUnit& pgm) {
    StmtList& statements = pgm.stmts();

    for (Iterator<Statement> iter = statements.stmts_of_type(ARITHMETIC_IF_STMT);
        iter.valid();
        ++iter) {
        Statement& aIfStmt = iter.current();

        RefList<Statement> labelList = aIfStmt.label_list();

        Statement& ltStmt = labelList[0];
        Statement& eqStmt = labelList[1];
        Statement& gtStmt = labelList[2];

        Expression* zero = constant(0);

        if (!aIfStmt.expr().is_side_effect_free()) {
            Expression* precalcExpr = get_precalc(
                aIfStmt.expr().clone(), 
                pgm, 
                aIfStmt,
                PRECALC_ALWAYS,
                statements.new_tag()
            );

            aIfStmt.expr(precalcExpr);
        }

        Expression& predicate = aIfStmt.expr();

        BinaryExpr* ltCmprExpr = new BinaryExpr(LT_OP, 
            expr_type(LT_OP, predicate.type(), zero->type()), 
            predicate.clone(), 
            zero->clone()
        );

        BinaryExpr* eqCmprExpr = new BinaryExpr(EQ_OP, 
            expr_type(EQ_OP, predicate.type(), zero->type()), 
            predicate.clone(), 
            zero->clone()
        );

        Statement& ltBranchIfStmt = statements.ins_IF_after(ltCmprExpr, &aIfStmt);
        Statement& eqBranchElseIfStmt = statements.ins_ELSEIF_after(eqCmprExpr, &ltBranchIfStmt);
        Statement& gtBranchElseStmt = statements.ins_ELSE_after(&ltBranchIfStmt);

        statements.ins_after(new GotoStmt(statements.new_tag(), &ltStmt), &ltBranchIfStmt);
        statements.ins_after(new GotoStmt(statements.new_tag(), &eqStmt), &eqBranchElseIfStmt);
        statements.ins_after(new GotoStmt(statements.new_tag(), &gtStmt), &gtBranchElseStmt);

        statements.del(aIfStmt);
    }
}

void convert_computed_gotos(ProgramUnit& pgm) {
    StmtList& statements = pgm.stmts();

    for (Iterator<Statement> iter = statements.stmts_of_type(COMPUTED_GOTO_STMT);
        iter.valid();
        ++iter) {
        Statement& computedGotoStmt = iter.current();

        if (!computedGotoStmt.expr().is_side_effect_free()) {
            Expression* precalcExpr = get_precalc(
                computedGotoStmt.expr().clone(), 
                pgm, 
                computedGotoStmt,
                PRECALC_ALWAYS,
                statements.new_tag()
            );

            computedGotoStmt.expr(precalcExpr);
        }

        Expression& predicate = computedGotoStmt.expr();

        RefList<Statement>& labelList = computedGotoStmt.label_list();

        Statement* prevStmt = &computedGotoStmt;

        int numEntries = labelList.entries();
        for (int i = 0; i < numEntries; i++) {
            Statement& labelItem = labelList[i];

            Expression* cmprConst = constant(i + 1);

            BinaryExpr* eqCmprExpr = new BinaryExpr(EQ_OP, 
                expr_type(EQ_OP, predicate.type(), cmprConst->type()), 
                predicate.clone(), 
                cmprConst->clone()
            );

            Statement* branchStmt;

            if (i == 0) {
                branchStmt = &statements.ins_IF_after(eqCmprExpr, prevStmt);
                prevStmt = branchStmt;

            } else {
                branchStmt = &statements.ins_ELSEIF_after(eqCmprExpr, prevStmt);
            }

            statements.ins_after(new GotoStmt(statements.new_tag(), &labelItem), branchStmt);
        }

        statements.del(computedGotoStmt);
    }
}

void convert_gotos(ProgramUnit& pgm) {
    convert_arithmetic_ifs(pgm);
    convert_computed_gotos(pgm);
};

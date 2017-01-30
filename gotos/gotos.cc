// 
// gotos.cc : convert gotos pass
// 

#include "gotos.h"
#include "StmtList.h"
#include "Collection/List.h"
#include "Statement/Statement.h"
#include "Statement/ArithmeticIfStmt.h"
#include "Statement/LabelStmt.h"
#include "Statement/IfStmt.h"
#include "Statement/EndIfStmt.h"
#include "Statement/GotoStmt.h"
#include "Expression/BinaryExpr.h"

using std::cout;
using std::endl;

void convert_gotos(ProgramUnit & pgm) {
    StmtList& statements = pgm.stmts();

    for (Iterator<Statement> iter = statements.stmts_of_type(ARITHMETIC_IF_STMT);
        iter.valid();
        ++iter) {
        ArithmeticIfStmt& aIfStmt = dynamic_cast<ArithmeticIfStmt&>(iter.current());

        Expression& predicate = aIfStmt.expr();

        RefList<Statement> labelList = aIfStmt.label_list();

        LabelStmt& ltStmt = dynamic_cast<LabelStmt&>(labelList[0]);
        LabelStmt& eqStmt = dynamic_cast<LabelStmt&>(labelList[1]);
        LabelStmt& gtStmt = dynamic_cast<LabelStmt&>(labelList[2]);

        Expression* zero = constant(0);

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


        /*
        const Statement* nextStmt = statements.next_ref(aIfStmt);
        cout << nextStmt->stmt_class() << endl;
        */
    }
};

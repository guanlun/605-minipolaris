// 
// gotos.cc : convert gotos pass
// 

#include "gotos.h"
#include "StmtList.h"
#include "Collection/List.h"
#include  "Statement/Statement.h"
#include "Statement/ArithmeticIfStmt.h"
#include "Statement/LabelStmt.h"
#include "Statement/IfStmt.h"
#include "Statement/EndIfStmt.h"
#include "Statement/GotoStmt.h"

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

        LabelStmt& gtStmt = dynamic_cast<LabelStmt&>(labelList[0]);
        LabelStmt& ltStmt = dynamic_cast<LabelStmt&>(labelList[1]);
        LabelStmt& eqStmt = dynamic_cast<LabelStmt&>(labelList[2]);

        gtStmt.print(cout);
        ltStmt.print(cout);
        eqStmt.print(cout);

        Statement* gotoStmt = new GotoStmt(statements.new_tag(), &gtStmt);

        Statement& newIfStmt = statements.ins_IF_after(predicate.clone(), &aIfStmt);

        statements.ins_after(gotoStmt, &newIfStmt);


        /*
        const Statement* nextStmt = statements.next_ref(aIfStmt);
        cout << nextStmt->stmt_class() << endl;
        */
    }
};

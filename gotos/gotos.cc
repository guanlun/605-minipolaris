// 
// gotos.cc : convert gotos pass
// 

#include "gotos.h"
#include "StmtList.h"
#include "Collection/List.h"
#include  "Statement/Statement.h"

using std::cout;
using std::endl;

void convert_gotos(ProgramUnit & pgm) {
    const StmtList& statements = pgm.stmts();

    for (Iterator<Statement> iter = statements; iter.valid(); ++iter) {
        Statement& stmt = iter.current();
        cout << iter.current().line() << endl;
    }
};

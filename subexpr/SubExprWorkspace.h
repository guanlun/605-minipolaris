/*
 * SubExprWorkspace.h
 *
 *  Created on: Mar 24, 2017
 *      Author: guanlun
 */

#ifndef _MINI_POLARIS_SUBEXPR_SUBEXPRWORKSPACE_H_
#define _MINI_POLARIS_SUBEXPR_SUBEXPRWORKSPACE_H_

#include <set>

class SubExprWorkspace : public WorkSpace {
public:
	SubExprWorkspace(int pass_tag) : WorkSpace(pass_tag), targetExpr(NULL), prevCopyRefStmt(NULL), idom(NULL) {}

	Listable *listable_clone() const { return NULL; };

	INLINE void print(ostream &o) const {
		o << "[ SubExprWorkspace: ";

		if (prevCopyRefStmt != NULL) {
		    o << prevCopyRefStmt->tag() << endl;
		}

		o << "]" << endl;
	}

	INLINE int structures_OK() const { return 1; };

	Expression* targetExpr;
	Statement* prevCopyRefStmt;

	set<Statement*> dominators;
	Statement* idom;
};

#endif /* SUBEXPR_SUBEXPRWORKSPACE_H_ */

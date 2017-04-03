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
	SubExprWorkspace(int pass_tag) : WorkSpace(pass_tag), targetExpr(NULL), prevCopyRefStmt(NULL) {}

	Listable *listable_clone() const { return NULL; };

	INLINE void print(ostream &o) const {
		o << "[ SubExprWorkspace: ";
//		for (set<Expression*>::iterator outIter = outSet.begin(); outIter != outSet.end(); ++outIter) {
//			Expression* outExpr = *outIter;
//
//			o << *outExpr << " ";
//		}

//		o << prevCopyRefStmt->tag() << endl;

		o << "]" << endl;
	}

	INLINE int structures_OK() const { return 1; };

	set<Expression*> inSet;
	set<Expression*> outSet;

	Expression* targetExpr;

	Statement* prevCopyRefStmt;
};

#endif /* SUBEXPR_SUBEXPRWORKSPACE_H_ */

#ifndef _MINI_POLARIS_SSA_WORKSPACE_H
#define _MINI_POLARIS_SSA_WORKSPACE_H

#include <set>

class SSAWorkSpace : public WorkSpace {
public:
	SSAWorkSpace(int pass_tag) : WorkSpace(pass_tag), immediateDominator(NULL) {
	}

	Listable *listable_clone() const { return NULL; };

	INLINE void print(ostream &o) const {
		o << "idom: " << immediateDominator->tag() << " ";
		o << dominanceFrontiers.size() << " items: ";
		for (set<Statement*>::iterator it = dominanceFrontiers.begin(); it != dominanceFrontiers.end(); ++it) {
			o << (*it)->tag() << " ";
		}
	}

	INLINE int structures_OK() const { return 1; };

	set<Statement*> dominators;
	Statement* immediateDominator;
	set<Statement*> dominanceFrontiers;
};

#endif

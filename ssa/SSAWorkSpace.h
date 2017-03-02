#ifndef _MINI_POLARIS_SSA_WORKSPACE_H
#define _MINI_POLARIS_SSA_WORKSPACE_H

#include <set>

class SSAWorkSpace : public WorkSpace {
public:
	SSAWorkSpace(int pass_tag) : WorkSpace(pass_tag), immediateDominator(NULL) {
	}

	Listable *listable_clone() const { return NULL; };

	INLINE void print(ostream &o) const {
//		o << "idom: " << immediateDominator->tag() << " ";
//		o << dominanceFrontiers.size() << " items: ";
//		for (set<Statement*>::iterator it = dominanceFrontiers.begin(); it != dominanceFrontiers.end(); ++it) {
//			o << (*it)->tag() << " ";
//		}

		o << dominants.size() << " dominants: ";
		for (set<Statement*>::iterator it = dominants.begin(); it != dominants.end(); ++it) {
			o << (*it)->tag() << " ";
		}

		for (set<Symbol*>::iterator it = phiSymbols.begin(); it != phiSymbols.end(); ++it) {
			o << (*it)->name_ref() << " ";
		}
	}

	INLINE int structures_OK() const { return 1; };

	set<Statement*> dominators;
	set<Statement*> dominants;
	Statement* immediateDominator;
	set<Statement*> dominanceFrontiers;
	set<Symbol*> phiSymbols;
};

#endif

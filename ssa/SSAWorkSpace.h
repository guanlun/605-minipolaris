#ifndef _MINI_POLARIS_SSA_WORKSPACE_H
#define _MINI_POLARIS_SSA_WORKSPACE_H

#include <set>

class SSAWorkSpace : public WorkSpace {
public:
    SSAWorkSpace(int pass_tag, bool isPhiForFunction = false)
        : WorkSpace(pass_tag), isPhiForFunction(isPhiForFunction) {
	}

	Listable *listable_clone() const { return NULL; };

	INLINE void print(ostream &o) const {
        cout << "SSAWorkSpace" << endl;
    };

	INLINE int structures_OK() const { return 1; };

    bool isPhiForFunction;
};

#endif

#ifndef _MINI_POLARIS_SSA_WORKSPACE_H
#define _MINI_POLARIS_SSA_WORKSPACE_H

#include <set>

class SSAWorkSpace : public WorkSpace {
public:
	SSAWorkSpace(int pass_tag) : WorkSpace(pass_tag) {}

	Listable *listable_clone() const { return NULL; };

	INLINE void print(ostream &o) const { o << ""; }

	INLINE int structures_OK() const { return 1; };


};

#endif

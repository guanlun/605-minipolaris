#ifndef _MINI_POLARIS_CONST_PROP_WS_H
#define _MINI_POLARIS_CONST_PROP_WS_H

class ConstPropWS : public WorkSpace {
public:
	ConstPropWS(int pass_tag) : WorkSpace(pass_tag), refCount(0) {}

	Listable *listable_clone() const { return NULL; };

	INLINE void print(ostream &o) const { o << "[ ConstPropWS: print not properly implemented. ]"; }

	INLINE int structures_OK() const { return 1; };

	RefSet<Statement> inSet;
	RefSet<Statement> outSet;

	int refCount;
};

#endif

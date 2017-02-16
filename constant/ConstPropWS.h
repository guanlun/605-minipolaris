#ifndef _MINI_POLARIS_CONST_PROP_WS_H
#define _MINI_POLARIS_CONST_PROP_WS_H

class ConstPropWS : public WorkSpace {
public:
	ConstPropWS(int pass_tag);

	void add_gen_stmt(const Statement& stmt);

	Statement* get_stmt_by_symbol(const Symbol& sym);

	Listable *listable_clone() const { return NULL; };

	INLINE void print(ostream &o) const { o << "[ ConstPropWS: print not properly implemented. ]"; }

	INLINE int structures_OK() const { return 1; };

	RefSet<Statement> outSet;
	RefSet<Statement> genSet;
private:

};

ConstPropWS::ConstPropWS(int pass_tag) : WorkSpace(pass_tag) {
}

void ConstPropWS::add_gen_stmt(const Statement& stmt) {
	genSet.ins(stmt);
}

Statement* ConstPropWS::get_stmt_by_symbol(const Symbol& sym) {
	for (Iterator<Statement> iter = genSet; iter.valid(); ++iter) {
		Statement& stmt = iter.current();

		cout << sym << "    " << stmt.lhs() << endl;

		if (&(stmt.lhs().symbol()) == &sym) {
			cout << "found!!!" << endl;
			return &stmt;
		}
	}

	return NULL;
}

#endif

#include <string>

bool is_stmt_with_named_func(Statement& stmt, const char* name) {
	if (stmt.stmt_class() != ASSIGNMENT_STMT) {
		return false;
	}

	const Expression& rhs = stmt.rhs();
	if (rhs.op() != FUNCTION_CALL_OP) {
		return false;
	}

	const Expression& func = rhs.function();

	return (strcmp(func.symbol().name_ref(), name) == 0);
}

bool is_phi_stmt(Statement& stmt) {
	return is_stmt_with_named_func(stmt, "PHI");
}

Expression* get_def_expr(Statement& stmt) {
	switch (stmt.stmt_class()) {
	case ASSIGNMENT_STMT:
		return &stmt.lhs();
	case DO_STMT:
		return &stmt.index();
	default:
		return NULL;
	}
}

void tab(int n) {
	for (int i = 0; i < n; i++) {
		cout << "    ";
	}
}

char* orig_symbol_name(Symbol& symbol) {
	const char* symbolName = symbol.name_ref();
	char* origName = new char[strlen(symbolName)];

	strcpy(origName, symbolName);

	origName = strtok(origName, "@");

	return origName;
}

int find_gcd(int a, int b) {
	if ((a == 0) || (b == 0)) {
		return a + b;
	}

	return find_gcd(b, a % b);
}

Expression* replace_ssa_expression(Expression* expr, Expression* oldExpr, Expression* newExpr) {
	if (*expr == *oldExpr) {
		return newExpr->clone();
	}
	else if ((expr->op() == ID_OP) && (oldExpr->op() == ID_OP)) {
		const char* origExprName = orig_symbol_name(expr->symbol());
		const char* origOldExprName = orig_symbol_name(oldExpr->symbol());

		if (strcmp(origExprName, origOldExprName) == 0) {
			return newExpr->clone();
		}
		else {
			return expr;
		}
	}
	else {
		for (Mutator<Expression> exprMut = expr->arg_list(); exprMut.valid(); ++exprMut) {
			exprMut.assign() = replace_ssa_expression(exprMut.pull(), oldExpr, newExpr);
		}
	}

	return expr;
}

string get_stmt_loop_name(Statement* stmt) {
	string loopName = "";

	Statement *runner = stmt;

	while (runner->stmt_class() != DO_STMT) {
		runner = runner->prev_ref();

		if (runner == NULL) {
			return "";
		}
	}

	loopName = runner->get_loop_name();

	return loopName;
}

bool expr_match_symbol(Expression& expr, Symbol& sym) {
	if (expr.op() != ID_OP) {
		return false;
	}

	string exprName(expr.symbol().name_ref());
	string symName(sym.name_ref());

	return (strcasecmp(exprName.c_str(), symName.c_str()) == 0);
}
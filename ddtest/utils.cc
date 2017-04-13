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

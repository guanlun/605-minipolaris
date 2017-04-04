# Global Common Subexpression Elimination

The Global Common Subexpression Elimination works by iteratively binarifying operations, finding common subexpressions, replacing common subexpressions with precomputed values, and propagating copies.

### Calculate available expressions
Available expressions are calculated based on the SSA form. Since each SSA variable could only have one possible value, variables with same names hold the same value and the available expression computation is greatly simplified.

At a particular point in the control flow, whether a expression `a2 + b3` is available simply depends on whether it has been used elsewhere. Therefore, any SSA expression that appears more than once could be eliminated, and the main problem is where the precomputed expression should be inserted, which would be explained in detail in the GCSE section.

Before finding available expressions, a pass would binarify the complex operations. This is done in a recursive manner that utilizes the built-in `arg_list` function, so that expressions like `a + b + c * d` would be simplified to binary expressions.

### GCSE
The subexpression elimination process would take as input a program with only binary or unary operations. It looks at which subexpressions have been used more than once, and computes a position to insert the common subexpression.

If an SSA expression `a2 + b3` is used 3 times in the program, we'd like to insert a `__temp1 = a2 + b3` statement where it could be available for all the 3 statements that use it, which should be the closest common dominator of the 3 statements. For cases like

    if (...) {
        s3 = a2 + b3;
    } else {
        t4 = a2 + b3;
    }

The position is obviously before the if block.

After the precomputed statement is inserted, we go ahead and replace all the occurrences of that subexpression with the precomputed variable.

### Copy propagation

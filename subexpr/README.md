# Global Common Subexpression Elimination

The Global Common Subexpression Elimination works by iteratively binarifying operations, finding common subexpressions, replacing common subexpressions with precomputed values, and propagating copies.

### Calculate available expressions
Available expressions are calculated based on the SSA form. Since each SSA variable could not have one possible value, variables with same names hold the same value and the available expression computation is greatly simplified.

At a particular point in the control flow, wheather an expression `a2 + b3` is available simply depends on whether it has been used elsewhere. Therefore, any SSA expression that appears more than once could be eliminated, and the main problem is where the precomputed expression should be inserted, which would be explained in detail in the GCSE section.

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

For special cases like function calls:

    a = foo(i, j)
    b = foo(i, j)

As Fortran might alter the value of actual parameters, `foo(i, j)` should not be considered as a common subexpression. However, this could be easily handled by making use of the SSA form and does not require special treatment.

### Copy propagation
Copy propagation is implemented using a simple method: whenever we find a copy assignment `x3 = y2` (SSA form), we replace every `x3` with `y2` in the entire program. Since the program is in SSA form, every `x3` should hold the same value as `y2`.

The more advanced form of copy propagation that requires tagging the generated subexpressions is not implemented.

### Example
For this program:

    program name 
    integer*4 i, j,a ,b ,c
    i = 3
    j = 4
    a = i + j + c
    j = i + j
    b = j + i
    c = i + j + d
    a = b - c
    j = j
    end 

After common subexpression elimination (before copy propagation), the program would be:

    PROGRAM name
    EXTERNAL phi
    INTEGER*4 a, b, c, i, j, phi, __temp1, __temp2, __temp3, __temp5
    INTEGER*4 __temp6
    REAL*8 d, __temp4
    i = 3
    j = 4
    __temp5 = i+j
    __temp1 = __temp5
    __temp2 = c+__temp1
    a = __temp2
    j = __temp5
    __temp6 = i+j
    b = __temp6
    __temp3 = __temp6
    __temp4 = __temp3+d
    c = __temp4
    a = b-c
    j = j
    STOP 
    END

The final output after copy propagation would be:

    PROGRAM name
    EXTERNAL phi
    INTEGER*4 a, b, c, i, j, phi, __temp1, __temp2, __temp3, __temp5
    INTEGER*4 __temp6
    REAL*8 d, __temp4
    i = 3
    j = 4
    __temp5 = i+j
    __temp2 = c+__temp5
    a = __temp2
    __temp6 = i+__temp5
    __temp4 = __temp6+d
    a = __temp6-__temp4
    j = __temp5
    STOP 
    END


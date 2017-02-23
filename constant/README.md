Constant Propagation
====================

I’ve used the standard approach described in the project specifications. First, constant parameters are substituted. Then the main loop would iterate until there’s no change. The main loop includes finding the IN and OUT sets of each statement (I used only statements instead of basic block), substituting constant values, simplifying values and removing the un-reachable branches.

GEN and KILL sets are computed based on the current statements. Combined with the IN sets, OUT sets are computed and passed on to the successor statements. This sub-process is executed until the IN and OUT sets of all statement converge.

In this process, some variables cannot be propagated due to various reasons. For example, actual parameter of functions and variables in the COMMON block should not be propagated. They would remove the corresponding definitions from the OUT set. This is because we have to assume the callee would modify the parameter or common variables, rendering the previous definitions invalid.

Elimination dead branches involves checking whether each branch is constantly true, false, or cannot be determined at compile time. My approach was to extract all the valid branches (branches not constantly false) and re-build the branch structure before removing the entire original branch statement, as modifying branch statements in Polaris is error-prone.

When nothing is changed in an iteration, the constant propagation procedures is completed and the next stage is removing unused definitions, as after propagation the constants, some references to variable definition may not be needed anymore. This requires detecting which definitions are not used in later part of the program, and removing them. This is also an iterative process and it completes when nothing more is removed. Doing this requires changing the constant propagation method by using the union operator when handling IN sets of a statement.

There are also several cases where unused variables should not be removed. For example, assignments to array indices or formal function parameters. As we could see in test17, although a(1) is not needed anymore, the array might be reference in some other functions, and the assignment introduced side-effects to the outside world. The same goes for assignments to formal parameters, as Fortran uses pass-by-reference, modifying formal parameters causes the actual parameter in the caller to change. Additional care needs to be taken when handling these side-effects.

There are also several features that have not be handled in this project.

* Calling functions with actual parameters that are constant so far in an expression that refers to the same variables in front of the function invocation, e.g. i = 2; i + foo(i) + i. While the function call might change the i parameter passed, the first parameter could still be resolved and we could change it to 2 + foo(i) + i.
* While loops with a constant false predicate at the first iteration should be eliminated. They are not eliminated in my implementation because the IN set of the while statement is determined by the OUT set of both the previous statement and that of the ENDDO statement. Therefore, the predicate is not constantly false. This could be solved by adding a special handling at the first iteration for while loops.
* When removing unused definitions, if the removed definition is the only statement in a branch statement, the entire branch should be removed. However this case is relatively rare and not taken care of.
* When DATA, COMMON, SAVED or equivalent variables exist, unused variable removal is not performed. This is to keep things safe to prevent useful code from being deleted.

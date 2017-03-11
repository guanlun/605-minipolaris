Since the SSA algorithm is based on basic blocks, I’ve modified driver.cc to pass the basic blocks to it. Therefore I’ve submitted the entire project folder.

PHI node insertion:
The standard algorithm for PHI node insertion is used:
1. Find dominators for each basic block
2. Find the immediate dominator for each block
3. Find the dominance frontier
4. Insert PHI nodes at DF

UPSILON node insertion:
UPSILON nodes are inserted in a final pass after PHI nodes have been inserted and variables renamed. The following algorithm is used to insert the UPSILON nodes:
1. Find PHI nodes
2. Find the definition of each PHI node’s argument
3. Insert UPSILON after definition

Variable renaming:
Variable renaming is implemented recursively based on the dominator tree structure. For each basic block:
1. Create a new variable name and push it to the stack for each defined variable and use the current variable name for each referenced variable.
2. Iterate through all the successor blocks in the CFG and populate the corresponding PHI nodes by inserting the variable defined in the current block to the arguments of that PHI function.
3. Recursively rename variables in each of the dominated blocks
4. Pop inserted new variables names

PHI statements for functions:
PHI statements are inserted for each function parameter as functions calls could alter the value of the parameters.

DeSSA:
All the PHI and UPSILON nodes would be deleted and variables renamed restored during DeSSA.

Limitations:
* Arrays are not handled
* DO statements do not have PHI statements after the corresponding ENDDO statements, as in https://piazza.com/class/iyatcoquc9y4fz?cid=25. This is because these PHI statements could not be inserted by only looking at the pred/succ relations in the CFG. To achieve that, we need special handling for DO statements by looking at which variables are defined in the DO body and adding PHI nodes after the ENDDO statement.


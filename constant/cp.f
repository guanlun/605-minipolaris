       INTEGER FUNCTION foo (a)
       INTEGER*4 a,k,c
       PARAMETER (c = 1)
       a = c + 1
       if (1 .EQ. c) then
           a = 4
       elseif (2 .EQ. 2) then
           a = 5
       else
           a = 8
       endif
       k = a + 1
       RETURN
       END
 
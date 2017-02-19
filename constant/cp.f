       INTEGER FUNCTION foo (a)
       INTEGER*4 a
       if (1 .EQ. 1) then
           a = 4
       elseif (2 .EQ. 2) then
           a = 5
       elseif (3 .EQ. 3) then
           a = 6
       elseif (3 .EQ. 3) then
           a = 7
       else
           a = 8
       endif
       FOO = a + 1
       RETURN
       END
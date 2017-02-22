       INTEGER FUNCTION foo (a)
       INTEGER*4 a
       a = a + 1
       RETURN
       END

       PROGRAM TESTME
       INTEGER*4 i, j, k
C       EXTERNAL FUNCTION foo
	   if (2 .EQ. 2) then
	   	   i = 4
	   else
	   	   i = 5
	   endif

       k = i + 1
       print *, k
	   i = 5
	   print *, k
       END
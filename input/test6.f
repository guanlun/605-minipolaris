       INTEGER FUNCTION foo (a)
       INTEGER*4 a
       FOO = a + 1
       RETURN
       END

       PROGRAM TESTME
       INTEGER*4 a
       EXTERNAL FUNCTION foo

       GOTO (10, 40, 20, 30) foo(3)
 10    PRINT *, 1
 20    PRINT *, 2
 30    PRINT *, 3
 40    PRINT *, 4
       STOP
       END

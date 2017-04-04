
       PROGRAM TESTFUNC
       INTEGER x, y, tmp, a, b, c, ifoo
       DATA x/2/, y/2/
       COMMON /area1/a,b

       c = 2 + ifoo(x, y)

       STOP
       END

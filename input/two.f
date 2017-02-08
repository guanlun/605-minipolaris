      PROGRAM TESTME
       EXTERNAL F
       INTEGER I, J

       READ *, I,J
       IF (I .LT. 10) THEN
	  GOTO (20,40,20,30,10) I+F(J)
       ELSE
	  GOTO (20,40,20,30,10) J*3
       ENDIF
       IF (J) 10,30,20
       PRINT *, J
       STOP
10     CONTINUE
       J = J + 3
       GOTO 30
20     CONTINUE
       PRINT *, 'BLAH'
30     CONTINUE
       PRINT *, J
       STOP
40     CONTINUE
       PRINT *, I

       STOP
       END

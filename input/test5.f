       PROGRAM test5

       INTEGER I, J

       J = 0 
       DO I=1, 10
       J=J+2
       IF (J .GT. 10) THEN
           GOTO 20
       ENDIF
20     CONTINUE
       ENDDO

       STOP
       END


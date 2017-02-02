        PROGRAM TEST4
        INTEGER*4 i,j

        READ *, i
        DO i = 1,10
          IF ( i .EQ. j) THEN
            j = 1
          ELSE
            j = 2
          ENDIF
        END DO
        PRINT *, j
        STOP
        END


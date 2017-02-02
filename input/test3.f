        PROGRAM TEST3
        INTEGER*4 i,j

        REAd *, i
        IF ( i .EQ. 0) THEN
          j = 1
        ELSE
          j = 2
        ENDIF
        PRINT *, j
        STOP
        END


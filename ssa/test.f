        PROGRAM TEST5
        INTEGER*4 i,j
        
        i = 1
        if ( i .EQ. 0 ) THEN
          j = 1
        ELSE
          i = j
          j = 2
        ENDIF
        PRINT *, j
        STOP
        END


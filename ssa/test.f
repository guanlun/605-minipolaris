        PROGRAM TEST7
        INTEGER*4 i,j
        
        i = 1
        if ( i .EQ. 0 ) THEN
          i = 1
        else if (i .EQ. 1) then
          i = 4
          j = 2
        else
          i = 3
        ENDIF
        PRINT *, j,i
        STOP
        END


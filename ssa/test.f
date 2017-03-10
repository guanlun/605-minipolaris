        PROGRAM TEST15
        INTEGER*4 i,j,k,l
        
        k = 4
        IF (i.EQ.j) THEN
          k = 2
        ELSE
          j = k
          k = 3
        ENDIF
        print j,k
        STOP
        END

        PROGRAM TEST19
        INTEGER*4 i,j,k,l

        i = 1;
        if ( i .EQ. 0) THEN
          if (j .EQ. 0) THEN
            goto 10
          endif
          if ( i .EQ. 2) THEN
          goto 10
          endif
        endif
        i = 3
 10     i = 21   
        print *,i
        STOP
        END



        PROGRAM TEST19
        INTEGER*4 i,j,k
        i = 4
        j = i
        if ( j .EQ. i) then
           j = 3
        else
           j = 7
        endif
        if ( j .EQ. 3) then
          print *,j
        else
          k = 2
        endif
        do l=1,10,1
          if ( k .EQ. 2) then
             k = 7
             print *,k
          else
           k = 10
             print *,k
          endif
          k = 3
        enddo

        k = i 
        print *, k,j
        STOP
        END

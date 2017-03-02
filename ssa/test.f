        PROGRAM TEST19
        INTEGER*4 i,j,k
        i = 4
        j = i
        if ( j .EQ. i) then
           j = 3
           i = 4
        else
           j = 7
        endif
        
        k = i 
        print *, k,j
        STOP
        END

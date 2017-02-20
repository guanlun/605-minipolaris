        PROGRAM TEST18
        INTEGER*4 i,j,k
        i = 4
        j = i
        
        do l=1,10,1
          i = 5
        enddo

        k = i 
        print *, k,j
        STOP
        END

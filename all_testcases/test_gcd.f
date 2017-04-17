        PROGRAM TEST2
        INTEGER*4 a,i,j
        dimension a(10)

        do i=1,10,1
          a(2*i) = a(2*i+1)
        enddo
        STOP
        END


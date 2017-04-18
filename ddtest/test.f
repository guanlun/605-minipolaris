        PROGRAM TEST5
        INTEGER*4 a,i,j
        dimension a(10)

        do i=2,10,1
          a(i) = 10
          j = a(i + 1)
        enddo
        STOP
        END


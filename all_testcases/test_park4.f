        PROGRAM TEST8
        INTEGER*4 a,i,j
        dimension a(10)

        do i=1,10,1
          j = 1
          i = j
        enddo
        PRINT *,i,j
        STOP
        END


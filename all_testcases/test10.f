        PROGRAM TEST10
        INTEGER*4 a,b,i,j
        dimension a(10), b(10)

        do i=1,10,1
          if ( i .EQ. 0) then
            a(i) = b(i)
          endif
          b(i+1) = a(i+1)
        enddo
        STOP
        END



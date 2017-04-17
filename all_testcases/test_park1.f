        PROGRAM TEST8
        INTEGER*4 a,i,j
        dimension a(10)

        do i=1,10,1
          if (a .eq. 0) then
            j = i
          endif 
          i = j
        enddo
        STOP
        END


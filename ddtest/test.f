        PROGRAM TEST4
        INTEGER*4 a,i,j,t,n
        dimension a(10), b(10)

		j = n
        do i=1,10,1
          j = b(i + 1)
		  t = a(i)
		  a(i + 1) = j+2;
		  a(i) = t
        enddo
		
        STOP
        END
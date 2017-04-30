        PROGRAM TEST4
        INTEGER*4 a,i,j,t,n
        dimension a(10), b(10)

		j = n
        do i=1,10,1
          t = b(i) + b(j)
		  n = t + a(i)
          j = j - 1
        enddo
		
        STOP
        END
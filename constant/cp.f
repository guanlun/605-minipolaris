        PROGRAM TEST20
        INTEGER*4 i,j,k
        i = 4
        j = 4
		DO WHILE (i .EQ. 3)
			j = i + 4
		ENDDO
        k = i 
        print *, k,j
        STOP
        END

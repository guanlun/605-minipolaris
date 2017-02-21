        PROGRAM TEST16
        INTEGER*4 i,j,k
        i = 4

		IF (i .EQ. j) THEN
			k = 1
		ELSE
			k = 2
		ENDIF
		
		if (k .EQ. 3) THEN
			i = 5
		ENDIF
        STOP
        END

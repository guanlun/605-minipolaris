
        PROGRAM TESTME
        INTEGER*4 i, j, k
        i = 2
       
        j = bar(i)

        k = i
        print *, k
	    i = j
	    print *, k
        END
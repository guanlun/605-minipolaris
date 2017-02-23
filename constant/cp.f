        INTEGER FUNCTION bar (a)
        INTEGER*4 a, b
        a = a + 1
        b = 1
        print *, b
        RETURN
        END

        PROGRAM TESTME
        INTEGER*4 i, j, k
        i = 2
       
        j = bar(i) + i

        k = i + 1
        print *, k
	    i = j
	    print *, k
        END
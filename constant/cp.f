        INTEGER FUNCTION bar (a)
        INTEGER*4 a
        a = a + 1
        RETURN
        END

        PROGRAM TESTME
        INTEGER*4 i, j, k
        i = 2
       
        j = bar(i)

        k = i + 1
        print *, k
	    i = j
	    print *, k
        END
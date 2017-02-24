
       
        PROGRAM TEST20
        INTEGER*4 i,j,k
        i = 4
        call foo(i)
        i = i + 1
        j = i + 1
        k = j + 1

        print *,j
        STOP
        END

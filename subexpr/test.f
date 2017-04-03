        PROGRAM TEST4
        INTEGER*4 i,j, n
        DATA l /4/
        i = 4
        j = i + 5
        a = 4
        b = 6
        IF ( s .EQ. 5) THEN
           a = i + j
        ELSE
           b = i + j
        ENDIF
        s = i + j
        s = s + a + b
        PRINT *, j
        STOP
        END

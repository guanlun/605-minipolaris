        PROGRAM TEST15
        INTEGER*4 i,j,k,l
        
        k = 4
        IF ( j .EQ. i) THEN
          DO i=1,10,1
            k = 5
            print *,i
          ENDDO
          print *,j
        ENDIF
        PRINT *, j,i
        STOP
        END


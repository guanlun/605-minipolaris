        program name 
        integer*4 i, j,a ,b ,c
        logical*1 x
        
        i = 3
        if (.NOT.(i .EQ. j)) then
        	j = 5
        else
        	j = 6
        endif
        
        if (.NOT.(i .EQ. j)) then
        	print j
        endif
        end 
			PROGRAM TEST
			INTEGER I, J, K, X, A(10)

			DO K=2,6
				DO I=1, 10
					J=I*K
					X = I+1
					PRINT*, J
					A(I)=J+X
				ENDDO
			ENDDO

			PRINT*, A(2)+A(3), J
			END



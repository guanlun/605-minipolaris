			PROGRAM TEST
			INTEGER I, J, X, A(10)

			PARAMETER (X = 100)

			J = 1;

			DO I=1, 10

				IF (X.EQ.100) THEN
					J = 2*I
				ELSE
					J = J + X
				ENDIF

				PRINT*, J
				A(I)=J
			ENDDO

			PRINT*, A(2)+A(3), J
			END



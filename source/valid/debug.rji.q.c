#include "Q.h"
BEGIN
STAT(0)
	STR(0x11ffb,"%li");
CODE(0)
STAT(1)
	STR(0x11ff7,"%c");
CODE(1)
L 1:	// kansu i
	R0 = I(R6+0);
	R0 = ++R0;
	I(R6+0) = R0;
	R0 = U(R6+0);
	R0 = I(R0);
	R0 = I(R6);
	R0 = R0 + 12;
	I(R6+R0) = R0;
	R7 = R6;
	R7 = R7 + 4;
	R6 = I(R7);
	R7 = R6 + 4;
	R6 = I(R7);
	GT(R6);
L 2:	// kansu h
	R7 = R7 - 8;
	R7 = R7 - 4;
	P(R7) = R6;
	R7 = R7 - 4;
	P(R7) = 3;
	R7 = R7 - 4;
	R1 = P(R6+0);
	P(R7) = R1;
	R1 = U(R6+0);
	R1 = I(R1);
	I(R7) = 4;
	R6 = R7;
L 3:	
	R7 = R7 + 4;
	R6 = I(R7);
	R7 = R7 + 4;
	R0 = I(R7);
	R7 = R7 + 8;
	I(R6+0) = R0;
	R0 = U(R6+0);
	R0 = I(R0);
	R0 = I(R6);
	R0 = R0 + 12;
	I(R6+R0) = R0;
	R7 = R6;
	R7 = R7 + 4;
	R6 = I(R7);
	R7 = R6 + 4;
	R6 = I(R7);
	GT(R6);
L 4:	// kansu g
	R7 = R7 - 8;
	R7 = R7 - 4;
	P(R7) = R6;
	R7 = R7 - 4;
	P(R7) = 5;
	R7 = R7 - 4;
	R2 = P(R6+0);
	P(R7) = R2;
	R2 = U(R6+0);
	R2 = I(R2);
	I(R7) = 4;
	R6 = R7;
L 5:	
	R7 = R7 + 4;
	R6 = I(R7);
	R7 = R7 + 4;
	R0 = I(R7);
	R7 = R7 + 8;
	I(R6+0) = R0;
	R0 = U(R6+0);
	R0 = I(R0);
	R0 = I(R6);
	R0 = R0 + 12;
	I(R6+R0) = R0;
	R7 = R6;
	R7 = R7 + 4;
	R6 = I(R7);
	R7 = R6 + 4;
	R6 = I(R7);
	GT(R6);
L 6:	// kansu f
	R7 = R7 - 8;
	R7 = R7 - 4;
	P(R7) = R6;
	R7 = R7 - 4;
	P(R7) = 7;
	R7 = R7 - 4;
	R3 = P(R6+0);
	P(R7) = R3;
	R3 = U(R6+0);
	R3 = I(R3);
	I(R7) = 4;
	R6 = R7;
L 7:	
	R7 = R7 + 4;
	R6 = I(R7);
	R7 = R7 + 4;
	R0 = I(R7);
	R7 = R7 + 8;
	I(R6+0) = R0;
	R0 = U(R6+0);
	R0 = I(R0);
	R0 = I(R6);
	R0 = R0 + 12;
	I(R6+R0) = R0;
	R7 = R6;
	R7 = R7 + 4;
	R6 = I(R7);
	R7 = R6 + 4;
	R6 = I(R7);
	GT(R6);
L 0:		// entry point
	R7 = R7 - 4;
	R4 = 2;
	R5 = 3;
	R6 = R4 + R5;
	I(R6-4) = R6;
	R4 = I(R6-4);
	R2 = R4;
	R1 = 0x11ffb;
	R0 = 8;
	GT(putf_);
L 8:	
	R7 = R7 - 8;
	R7 = R7 - 4;
	P(R7) = R6;
	R7 = R7 - 4;
	P(R7) = 9;
	R7 = R7 - 4;
	P(R7) = R6 - 4;
	R4 = I(R6-4);
	I(R7) = 4;
	R6 = R7;
L 9:	
	R7 = R7 + 4;
	R6 = I(R7);
	R7 = R7 + 4;
	R0 = I(R7);
	R7 = R7 + 8;
	R5 = I(R6-4);
	R5 = ++R5;
	I(R6-4) = R5;
	R5 = I(R6-4);
	R2 = R5;
	R1 = 0x11ffb;
	R0 = 10;
	GT(putf_);
L 10:	
	R0=0;	// no return
	GT(-2);	// exit
END


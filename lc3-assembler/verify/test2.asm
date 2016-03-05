.ORIG x3000
LEA R0, EXPR
ADD R0, R0, #2
LD R3, SEMI

SCAN
LDR R1, R0, #0
        ;askdhkfjashkjhfsa
ADD R2, R1, R3
;askdhkfjashkjhfsa
BRz QUIT



ADD R0, R0, #1
BRnzp SCAN

QUIT 
LEA R0, PR ;ashdjkhqwk
PUTS                ;askdhkfjashkjhfsa qwhekjhaksf

HALT


SEMI .FILL xFFC5
EXPR .STRINGZ "y=((2-5)*(4+2));" ;qwekhfkajshkjasf
PR .STRINGZ "Quit!\n"
.END

.ORIG x3310

; ===================== Pop subroutine begin =========================

POP
AND R5, R5, #0
LEA R0, StackBase
NOT R0, R0 ; - StackBase - 1
ADD R0, R6, R0
BRz UnderFlow
LDR R0, R6, #0
ADD R6, R6, #1
RET

UnderFlow
ADD R5, R5, #1
RET

; ===================== Pop subroutine begin =========================


; ===================== Push subroutine begin =========================

PUSH
ST R1, SAVE1
AND R5, R5, #0
LEA R1, StackMax
NOT R1, R1
ADD R1, R1, #1
ADD R1, R6, R1
BRz OverFlow
ADD R6, R6, #-1
STR R0, R6, #0
LD R1, SAVE1
RET

OverFlow
ADD R5, R5, #1
RET

; ===================== Push subroutine end =========================

SAVE1 .FILL x0

StackMax .BLKW 100
StackBase .FILL x0
.END

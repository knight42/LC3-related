.ORIG x3000

TMP LD R6, BASE

LD R4, EXPR
ADD R4, R4, #2

SCAN
LDR R0, R4, #0

LD R3, NegSemi
ADD R2, R0, R3
BRz SAVE_RES            ; read ';', end of the expression

LD R3, Neg0
ADD R2, R0, R3
BRn if_right_bracket
ADD R0, R2, #0          ; read digit
BR insert

if_right_bracket
LD R3, NegRb
ADD R2, R0, R3
BRz HANDLE              ; read ')'

insert
LDI R5, PUSH_L
JSRR R5
ADD R5, R5, #0
BRz NEXT
LEA R0, StOver
PUTS
HALT

HANDLE
ST R4, TMP
LDI R4, POP_L

JSRR R4
ADD R5, R5, #0
BRz POP_2nd
BR err_stunder

POP_2nd
ADD R1, R0, #0

JSRR R4
ADD R5, R5, #0
BRz POP_Oper
BR err_stunder

POP_Oper
ADD R3, R0, #0

JSRR R4
ADD R5, R5, #0
BRz POP_1st

err_stunder
LEA R0, StUnder
PUTS
HALT

POP_1st
; R0: numA
; R1: numB
; R3: oper
LD R5, NegPlus
ADD R2, R3, R5
BRnp IF_MINUS      ; not '+'
JSR ADDITION
BR test_res

IF_MINUS
LD R5, NegMinus
ADD R2, R3, R5
BRnp IF_MUL        ; not '-'

; Minus
JSR MINUS
BR test_res

IF_MUL
JSR MULTIPLY

test_res
ADD R5, R5, #0
BRz sav_mid_res
BRp load_o_str
LEA R0, under_str
BR show_str

load_o_str
LEA R0, over_str

show_str
PUTS
HALT

sav_mid_res
JSRR R4;
ADD R0, R1, #0
LDI R4, PUSH_L
JSRR R4;
LD R4, TMP

ADD R5, R5, #0
BRz NEXT
LEA R0, StOver
PUTS
HALT

NEXT
ADD R4, R4, #1
BR SCAN

SAVE_RES
LDI R5, POP_L
JSRR R5
STI R0, RESULT
HALT

NegPlus .FILL xFFD5
NegMul .FILL xFFD6
NegMinus .FILL xFFD3
NegSemi .FILL xFFC5
NegLb .FILL xFFD8
NegRb .FILL xFFD7
Neg0 .FILL xFFD0

RESULT .FILL x4000
PUSH_L .FILL x4001
POP_L .FILL x4002
EXPR .FILL x4003
BASE .FILL xFE00

StOver .STRINGZ "Stack Overflow\n"
StUnder .STRINGZ "Stack Underflow\n"

over_str .STRINGZ "(Intermediate) Result Overflow\n"
under_str .STRINGZ "(Intermediate) Result Underflow\n"

; ================================ functions begin ====================================

; ===================== Addition subroutine begin =========================

ADDITION

AND R5, R5, #0
ADD R0, R0, #0
BRzp add_a_pos
BRn add_a_neg

add_a_pos
ADD R1, R1, #0
BRp add_may_over
BRnz add_ok

add_a_neg
ADD R1, R1, #0
BRzp add_ok
BRn add_may_under

add_ok
ADD R1, R0, R1
RET

add_may_over
ADD R1, R0, R1
BRn add_over
RET
add_over
ADD R5, R5, #1
RET

add_may_under
ADD R1, R0, R1
BRp add_under
RET
add_under
ADD R5, R5, #-1
RET

; ===================== Addition subroutine end =========================

; ===================== Minus subroutine begin =========================

MINUS
ST R7, SAVE1
NOT R1, R1
ADD R1, R1, #1
JSR ADDITION
LD R7, SAVE1
RET

; ===================== Minus subroutine end =========================

; ===================== Multiply subroutine begin =========================

Multiply
AND R5, R5, #0
ST R2, SAVE1
ST R3, SAVE2
ST R6, mul_tmp
AND R2, R2, #0
AND R3, R3, #0

ADD R1, R1, #0
BRp mul_step1
BRz mul_ret
NOT R1, R1
ADD R1, R1, #1
NOT R2, R2

mul_step1
ADD R0, R0, #0
BRp mul_loop
BRn mul_step2
BRz mul_fix_neg

mul_step2
NOT R0, R0
ADD R0, R0, #1
NOT R2, R2

mul_loop
ADD R3, R3, R1
;
; handle the special case
BRzp mul_loop_next
LD R6, NegMin
ADD R6, R3, R6
BRnp mul_fail
ADD R2, R2, #0
BRz mul_fail
;
;
mul_loop_next
ADD R0, R0, #-1
BRp mul_loop

mul_fix_neg
ADD R1, R3, #0
ADD R2, R2, #0
BRz mul_ret
NOT R1, R1
ADD R1, R1, #1

mul_ret 
LD R2, SAVE1
LD R3, SAVE2
LD R6, mul_tmp
RET

mul_fail
LD R3, SAVE2
LD R6, mul_tmp
ADD R2, R2, #0
BRz mul_over

; underflow
LD R2, SAVE1
ADD R5, R5, #-1
RET

mul_over
LD R2, SAVE1
ADD R5, R5, #1
RET

NegMin .FILL x8000
mul_tmp .FILL #0

; ===================== Multiply subroutine end =========================

; ================================ functions end ====================================

SAVE1 .FILL x0
SAVE2 .FILL x0
.END

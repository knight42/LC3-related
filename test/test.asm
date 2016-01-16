.ORIG x3000
LD R0, zero
LD R6, base
LD R1, pop_addr
LD R2, push_addr
jsrr r2
LD R0, one
jsrr r2
LD R0, two
jsrr r2
LD R0, three
jsrr r2

jsrr r1
OUT
jsrr r1
OUT
jsrr r1
OUT
jsrr r1
OUT
LD r0, newline
OUT

HALT

zero .fill x0030
one .fill x0031
two .fill x0032
three .fill x0033
newline .fill xA
base .fill x338c
pop_addr .fill x3310
push_addr .fill x331A
.END

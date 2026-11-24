; ==========================================
; all-opcodes-i80.asm
; ==========================================
; Covers all valid i8080 instruction,
; Along with labels and comments.

NOP

LXI B, 0x1234
STAX B
INX B
INR B
DCR B
MVI B, 0x12
RLC
DAD B
LDAX B
DCX B
INR C
DCR C
MVI C, 0x34
RRC

LXI D, 0x5678
STAX D
INX D
INR D
DCR D
MVI D, 0x56
RAL
DAD D
LDAX D
DCX D
INR E
DCR E
MVI E, 0x78
RAR

LXI H, 0x9ABC
SHLD DATA_WORD
INX H
INR H
DCR H
MVI H, 0x9A
DAA
DAD H
LHLD DATA_WORD
DCX H
INR L
DCR L
MVI L, 0xBC
CMA

LXI SP, 0xF000
STA DATA_BYTE
INX SP
INR M
DCR M
MVI M, 0x42
STC
DAD SP
LDA DATA_BYTE
DCX SP
INR A
DCR A
MVI A, 0xFF
CMC

; ------------------------------------------
; 40-7F: MOV register matrix and HLT
; ------------------------------------------

MOV B, B
MOV B, C
MOV B, D
MOV B, E
MOV B, H
MOV B, L
MOV B, M
MOV B, A

MOV C, B
MOV C, C
MOV C, D
MOV C, E
MOV C, H
MOV C, L
MOV C, M
MOV C, A

MOV D, B
MOV D, C
MOV D, D
MOV D, E
MOV D, H
MOV D, L
MOV D, M
MOV D, A

MOV E, B
MOV E, C
MOV E, D
MOV E, E
MOV E, H
MOV E, L
MOV E, M
MOV E, A

MOV H, B
MOV H, C
MOV H, D
MOV H, E
MOV H, H
MOV H, L
MOV H, M
MOV H, A

MOV L, B
MOV L, C
MOV L, D
MOV L, E
MOV L, H
MOV L, L
MOV L, M
MOV L, A

MOV M, B
MOV M, C
MOV M, D
MOV M, E
MOV M, H
MOV M, L
HLT
MOV M, A

MOV A, B
MOV A, C
MOV A, D
MOV A, E
MOV A, H
MOV A, L
MOV A, M
MOV A, A

; ------------------------------------------
; 80-BF: Arithmetic and logical operations
; ------------------------------------------

ADD B
ADD C
ADD D
ADD E
ADD H
ADD L
ADD M
ADD A

ADC B
ADC C
ADC D
ADC E
ADC H
ADC L
ADC M
ADC A

SUB B
SUB C
SUB D
SUB E
SUB H
SUB L
SUB M
SUB A

SBB B
SBB C
SBB D
SBB E
SBB H
SBB L
SBB M
SBB A

ANA B
ANA C
ANA D
ANA E
ANA H
ANA L
ANA M
ANA A

XRA B
XRA C
XRA D
XRA E
XRA H
XRA L
XRA M
XRA A

ORA B
ORA C
ORA D
ORA E
ORA H
ORA L
ORA M
ORA A

CMP B
CMP C
CMP D
CMP E
CMP H
CMP L
CMP M
CMP A

; ------------------------------------------
; C0-CF: Conditional control and immediate ops
; ------------------------------------------

RNZ
POP B
JNZ FORWARD_1
JMP FORWARD_2
CNZ FORWARD_3
PUSH B
ADI 0x11
RST 0
RZ
RET
JZ BACKWARD_1
CZ BACKWARD_2
CALL BACKWARD_3
ACI 0x22
RST 1

; ------------------------------------------
; D0-DF
; ------------------------------------------

RNC
POP D
JNC FORWARD_2
OUT 0x01
CNC FORWARD_3
PUSH D
SUI 0x33
RST 2
RC
; Conditional carry return
JC BACKWARD_1
CC BACKWARD_2
IN 0x02
SBI 0x44
RST 3

; ------------------------------------------
; E0-EF
; ------------------------------------------

RPO
POP H
JPO FORWARD_3
XTHL
CPO BACKWARD_3
PUSH H
ANI 0x55
RST 4
RPE
PCHL
JPE BACKWARD_1
XCHG
CPE BACKWARD_2
XRI 0x66
RST 5

; ------------------------------------------
; F0-FF
; ------------------------------------------

RP
POP PSW
JP FORWARD_1
DI
CP FORWARD_2
PUSH PSW
ORI 0x77
RST 6
RM
SPHL
JM BACKWARD_3
EI
CM BACKWARD_1
CPI 0x88
RST 7

; ------------------------------------------
; Label reference tests
; ------------------------------------------

BACKWARD_1:
NOP

BACKWARD_2:
NOP

BACKWARD_3:
NOP

FORWARD_1:
NOP

FORWARD_2:
NOP

FORWARD_3:
NOP

; ------------------------------------------
; Data addresses
; ------------------------------------------

DATA_BYTE:
NOP

DATA_WORD:
NOP

; ------------------------------------------
; End
; ------------------------------------------

END_LABEL:

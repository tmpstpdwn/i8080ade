; ==========================================
; all-errors.asm
; ==========================================
; Triggers every diagnostic the assembler can emit.

; --- DIAG_LEX_UNRECOGNISED ---
; An unrecognised character
NOP
@

; --- DIAG_LABEL_IS_MNEMONIC ---
; Using an instruction mnemonic as a label
NOP:

; --- DIAG_LABEL_IS_REGISTER ---
; Using a register name as a label
A:

; --- DIAG_LABEL_BAD_NAME ---
; A token that isn't a valid identifier before ':'
42bad:

; --- DIAG_EXPECTED_MNEMONIC ---
; Something that isn't an opcode where one is expected
NOTANINSTR

; --- DIAG_INCOMPLETE_INSTR ---
; An instruction that expects operands but the line ends
MVI

; --- DIAG_EXPECTED_COMMA ---
; Missing comma between operands
MOV B B

; --- DIAG_BAD_NUMBER ---
; A malformed number literal
MVI A, 0xZZ

; --- DIAG_EXPECTED_NUM ---
; Expected a number but got something else
ADI B

; --- DIAG_EXPECTED_NUM_OR_LABEL ---
; Expected a number or label for an address operand, got punctuation
JMP ,

; --- DIAG_EXPECTED_REGISTER ---
; Expected a register but got a number
MOV 42, B

; --- DIAG_TRAILING_TOKENS ---
; Extra tokens after a complete instruction
NOP NOP

; --- DIAG_LABEL_DUPLICATE ---
; Defining the same label twice
DUP_LABEL:
NOP
DUP_LABEL:
NOP

; --- DIAG_LABEL_UNDEFINED ---
; Referencing a label that doesn't exist
JMP DOES_NOT_EXIST

; --- DIAG_MOV_M_M ---
; MOV M, M is invalid (encodes HLT)
MOV M, M

; --- DIAG_VALUE_RANGE (16-bit address) ---
; Address operand exceeding 16 bits
JMP 0x1FFFF

; --- DIAG_VALUE_RANGE (16-bit immediate) ---
; Immediate16 operand exceeding 16 bits
LXI B, 0x10000

; --- DIAG_VALUE_RANGE (8-bit) ---
; Immediate8 operand exceeding 8 bits
MVI A, 0x100

; --- DIAG_VALUE_RANGE (3-bit / RST) ---
; RST vector out of range
RST 8

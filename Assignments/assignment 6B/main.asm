//-----------------------------
// Title: assignment 6B: Designing a Counter
//-----------------------------
// Purpose: 
// Dependencies: NONE
// Compiler: pic-as v3.10
// Author: Jaime Yah-Perez
// OUTPUTS: PORTD = 7-segment common cathode display
// INPUTS: 2 button switches,
//  	V1.0: 3/29/2026 - First version
//	V1.1: 4/1/2026 - all features implemented
//	V1.2: 4/2/2026 - significant debugging in simulation done
//-----------------------------

#include "ConfigureFile.inc"
#include <xc.inc>

;----------------------------------------------------------
; Constants
;----------------------------------------------------------
INNER_LOOP      equ     255
OUTER_LOOP      equ     255

COUNTREG        equ     20h
BTNREG          equ     21h
REG10           equ     22h
REG11           equ     23h

BTN_A_ONLY      equ     08h     ; RB3=1, RB0=0
BTN_B_ONLY      equ     01h     ; RB3=0, RB0=1
BTN_BOTH        equ     00h     ; RB3=0, RB0=0

;----------------------------------------------------------
; Reset Vector
;----------------------------------------------------------
PSECT absdata,abs,ovrld
ORG     0x0000
    GOTO    start

;----------------------------------------------------------
; Main Program
;----------------------------------------------------------
ORG     0x0020

start:
    ;-----------------------------
    ; Inline setup for PORTD
    ;-----------------------------
    BANKSEL ANSELD
    CLRF    ANSELD,1

    BANKSEL TRISD
    CLRF    TRISD,1

    BANKSEL LATD
    CLRF    LATD,1

    BANKSEL ODCOND
    CLRF    ODCOND,1

    BANKSEL SLRCOND
    CLRF    SLRCOND,1

    ;-----------------------------
    ; Inline setup for PORTB
    ; RB0 and RB3 as inputs
    ;-----------------------------
    BANKSEL ANSELB
    CLRF    ANSELB,1

    BANKSEL TRISB
    MOVLW   0x09
    MOVWF   TRISB,1

    BANKSEL LATB
    CLRF    LATB,1

    BANKSEL WPUB
    MOVLW   0x09
    MOVWF   WPUB,1

    BANKSEL ODCONB
    CLRF    ODCONB,1

    BANKSEL SLRCONB
    CLRF    SLRCONB,1

    ;-----------------------------
    ; Initialize count to 0
    ;-----------------------------
    CLRF    COUNTREG,0
    RCALL   _loadDisplayFromCount
    MOVFF   TABLAT, LATD

main:
    RCALL   _readButtons

    ; both pressed -> reset
    MOVLW   BTN_BOTH
    CPFSEQ  BTNREG,0
    GOTO    checkA
    RCALL   ResetZero
    GOTO    updateDisplay

checkA:
    ; A only -> count up
    MOVLW   BTN_A_ONLY
    CPFSEQ  BTNREG,0
    GOTO    checkB
    RCALL   CountUP
    GOTO    updateDisplay

checkB:
    ; B only -> count down
    MOVLW   BTN_B_ONLY
    CPFSEQ  BTNREG,0
    GOTO    updateDisplay
    RCALL   CountDOWN

updateDisplay:
    MOVFF   TABLAT, LATD
    RCALL   DELAY
    GOTO    main

;----------------------------------------------------------
; Subroutine: CountUP
;----------------------------------------------------------
CountUP:
    MOVLW   0x0F
    CPFSEQ  COUNTREG,0
    GOTO    CountUP_Increment

    ; wrap F -> 0
    CLRF    COUNTREG,0
    RCALL   _loadDisplayFromCount
    RETURN

CountUP_Increment:
    INCF    COUNTREG,1,0
    RCALL   _loadDisplayFromCount
    RETURN

;----------------------------------------------------------
; Subroutine: CountDOWN
;----------------------------------------------------------
CountDOWN:
    MOVLW   0x00
    CPFSEQ  COUNTREG,0
    GOTO    CountDOWN_Decrement

    ; wrap 0 -> F
    MOVLW   0x0F
    MOVWF   COUNTREG,0
    RCALL   _loadDisplayFromCount
    RETURN

CountDOWN_Decrement:
    DECF    COUNTREG,1,0
    RCALL   _loadDisplayFromCount
    RETURN

;----------------------------------------------------------
; Subroutine: ResetZero
;----------------------------------------------------------
ResetZero:
    CLRF    COUNTREG,0
    RCALL   _loadDisplayFromCount
    RETURN

;----------------------------------------------------------
; Subroutine: _readButtons
; Reads PORTB and keeps only RB0 and RB3
;----------------------------------------------------------
_readButtons:
    MOVFF   PORTB, BTNREG
    MOVLW   0x09
    ANDWF   BTNREG,1,0
    RETURN

;----------------------------------------------------------
; Subroutine: _loadDisplayFromCount
; Loads TABLAT = NUMBERS[COUNTREG]
;----------------------------------------------------------
_loadDisplayFromCount:
    CLRF    TBLPTRU,0
    MOVLW   0x02
    MOVWF   TBLPTRH,0
    MOVF    COUNTREG,0,0
    MOVWF   TBLPTRL,0
    TBLRD*
    RETURN
    

;----------------------------------------------------------
; Subroutine: DELAY
;----------------------------------------------------------
DELAY:
    RCALL   loop1
    RCALL   loop1
    RCALL   loop1
    RETURN

loop1:
    MOVLW   INNER_LOOP
    MOVWF   REG10,0
    MOVLW   OUTER_LOOP
    MOVWF   REG11,0

loop2:
    DECF    REG10,1,0
    BNZ     loop2

    MOVLW   INNER_LOOP
    MOVWF   REG10,0
    DECF    REG11,1,0
    BNZ     loop2
    RETURN

;----------------------------------------------------------
; 7-segment lookup table
; Common-cathode encodings for 0..F
;----------------------------------------------------------
ORG     0x0200

NUMBERS:
    DB  0x3F    ; 0
    DB  0x06    ; 1
    DB  0x5B    ; 2
    DB  0x4F    ; 3
    DB  0x66    ; 4
    DB  0x6D    ; 5
    DB  0x7D    ; 6
    DB  0x07    ; 7
    DB  0x7F    ; 8
    DB  0x6F    ; 9
    DB  0x77    ; A
    DB  0x7C    ; b
    DB  0x39    ; C
    DB  0x5E    ; d
    DB  0x79    ; E
    DB  0x71    ; F

END

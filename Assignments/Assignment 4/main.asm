//-----------------------------
// Title: myFirstAssemblyProgram
//-----------------------------
// Purpose: a basic heating and cooling system. The user enters a desired 
// temperature, and the system reads the environment temperature. If the 
// reference temperature is higher, the cooling system will trigger. If the 
// reference temperature is lower, the heating system will trigger.
// Dependencies: NONE
// Compiler: pic-as v3.10
// Author: Jaime Yah-Perez
// OUTPUTS: 2 LED's, one Hot pin and one Cold pin
// INPUTS: keypad interface for user input, temperature sensor for environment
// Versions:
//  	V1.0: 3/9/2026 - First version 
//-----------------------------
#include "MyConfigFile.inc"
#include <xc.inc>

;----------------
; PROGRAM INPUTS
;----------------
;The DEFINE directive is used to create macros or symbolic names for values.
;It is more flexible and can be used to define complex expressions or sequences of instructions.
;It is processed by the preprocessor before the assembly begins.

#define  measuredTempInput 	5 ; this is the input value
#define  refTempInput		15 ; this is the input value

;---------------------
; Definitions
;---------------------
#define SWITCH    LATD,2  
#define LED0      PORTD,0
#define LED1	     PORTD,1
    
 
;---------------------
; Program Constants
;---------------------
; The EQU (Equals) directive is used to assign a constant value to a symbolic name or label.
; It is simpler and is typically used for straightforward assignments.
;It directly substitutes the defined value into the code during the assembly process.
    
REG10   equ     10h   ; in HEX
REG11   equ     11h
REG01   equ     1h

;---------------------
; Required variable locations
;---------------------
refTemp      equ 0x20
measuredTemp equ 0x21
contReg      equ 0x22

; Scratch regs for conversion
numerator    equ 0x23
quotient     equ 0x24
     
; R9: refTemp decimal digits
refOnes  equ 0x60
refTens  equ 0x61
refHund  equ 0x62
  
; R10: measuredTemp decimal digits
measOnes equ 0x70
measTens equ 0x71
measHund equ 0x72
 
 ; Reset / start (R7)
;---------------------
    PSECT absdata,abs,ovrld
    ORG 0x0000
    GOTO _start

; Program must start at 0x20 in program memory (R7)
    ORG 0x0020

_start:
;---------------------
; 1) Initialize PORTD (R6)
;---------------------
    ; Make RD1 and RD2 outputs, everything else input
    ; RD2 (cool) output => TRISD,2 = 0
    ; RD1 (heat) output => TRISD,1 = 0
    MOVLW   249
    MOVWF   TRISD,0

    ; Turn everything off initially
    CLRF    LATD,0
    
    ;---------------------
; 2) Load simulated inputs into required registers (R8)
;---------------------
    MOVLW   refTempInput
    MOVWF   refTemp,0

    MOVLW   measuredTempInput
    MOVWF   measuredTemp,0

;---------------------
; 3) convert to decimal digits and store (R9/R10)
;---------------------
    CALL    CONVERT_REF_TO_DEC
    CALL    CONVERT_MEAS_TO_DEC

;---------------------
; 4) Compare logic and set contReg
;---------------------
MAIN_LOOP:

    ;neg measuredTTemp is below refTemp range, therefore it always goes to HOT
    BTFSC   measuredTemp,7,0     ; if bit7=1 => negative
    BRA     MEAS_NEGATIVE
    
    ; Equality check: if measuredTemp == refTemp
    MOVF    refTemp,0,0         ; W = refTemp
    CPFSEQ  measuredTemp,0
    BRA     NOT_EQUAL

EQUAL_CASE:
    CLRF    contReg,0           ; contReg=0
    BRA     LED_OFF

NOT_EQUAL:
    ; If measuredTemp > refTemp => contReg=2 => COOL (RD2)
    MOVF    refTemp,0,0         ; W = refTemp
    CPFSGT  measuredTemp,0      ; skip next if measuredTemp > W
    BRA     MEAS_NOT_GREATER

MEAS_GREATER:
    MOVLW   2
    MOVWF   contReg,0
    BRA     LED_COOL

MEAS_NOT_GREATER:
    ; Else measuredTemp < refTemp => contReg=1 => HEAT (RD1)
    MOVLW   1
    MOVWF   contReg,0
    BRA     LED_HOT

    MEAS_NEGATIVE:
    MOVLW   1
    MOVWF   contReg,0
    BRA     LED_HOT
    
;---------------------
; 5) Output actions
;---------------------
LED_COOL:
    ; Turn ON COOL on RD2, turn OFF HEAT on RD1
    BCF     LATD,1,0             ; heat off
    BSF     LATD,2,0             ; cool on
    CALL    DELAY_1S
    BRA     MAIN_LOOP

LED_HOT:
    ; Turn ON HEAT on RD1, turn OFF COOL on RD2
    BCF     LATD,2,0             ; cool off
    BSF     LATD,1,0             ; heat on
    CALL    DELAY_1S
    BRA     MAIN_LOOP

LED_OFF:
    ; Turn both off
    BCF     LATD,1,0
    BCF     LATD,2,0
    CALL    DELAY_1S
    BRA     MAIN_LOOP

    

;===========================================================
; Subroutine: CONVERT_REF_TO_DEC
; Input:  refTemp (0x20), assumed 10..50
; Output: refOnes/refTens/refHund at 0x60/0x61/0x62
; Method: repeated subtraction by 10 (simple technique)
;===========================================================
CONVERT_REF_TO_DEC:
    ; clear outputs
    CLRF    refOnes,0
    CLRF    refTens,0
    CLRF    refHund,0

    ; numerator = refTemp
    MOVF  refTemp,0,0
    MOVWF numerator,0

    ; If negative (bit7=1), take absolute value (ignore sign)
    BTFSS numerator,7,0
    BRA   REF_POS
    NEGF  numerator,0
    
REF_POS:
    ; tens = 0
    CLRF    quotient,0
    MOVLW   10

REF_TENS_LOOP:
    SUBWF   numerator,1,0       ; numerator -= 10
    BC      REF_TENS_INC        ; if no borrow (C=1), keep going
    ; went negative once: undo and finish
    ADDWF   numerator,1,0       ; numerator += 10
    BRA     REF_STORE

REF_TENS_INC:
    INCF    quotient,1,0        ; quotient++
    BRA     REF_TENS_LOOP

REF_STORE:
    ; ones = numerator
    MOVFF   numerator, refOnes
    ; tens = quotient
    MOVFF   quotient, refTens
    ; hundreds stays 0 for 10..50
    RETURN


;===========================================================
; Subroutine: CONVERT_MEAS_TO_DEC
; Input:  measuredTemp (0x21), may be negative (-10..60)
; Output: measOnes/measTens/measHund at 0x70/0x71/0x72
; Note: ignores negative sign (stores magnitude)
;===========================================================
CONVERT_MEAS_TO_DEC:
    ; clear outputs
    CLRF    measOnes,0
    CLRF    measTens,0
    CLRF    measHund,0

    ; numerator = measuredTemp
    MOVF    measuredTemp,0,0
    MOVWF   numerator,0

    ; If negative (bit7=1), take absolute value
    BTFSS   numerator,7,0       ; skip if negative
    BRA     MEAS_POS
    NEGF    numerator,0         ; numerator = -numerator (magnitude)
MEAS_POS:
    ; First compute tens/ones from numerator
    CLRF    quotient,0
    MOVLW   10

MEAS_TENS_LOOP:
    SUBWF   numerator,1,0
    BC      MEAS_TENS_INC
    ADDWF   numerator,1,0
    BRA     MEAS_STORE_ONES_TENS

MEAS_TENS_INC:
    INCF    quotient,1,0
    BRA     MEAS_TENS_LOOP

MEAS_STORE_ONES_TENS:
    MOVFF   numerator, measOnes
    MOVFF   quotient,  measTens

    RETURN
;===========================================================
; Subroutine: DELAY_1S
; Input:  nothing
; Output: nothing
; Note: Its' sole purpose is to create a 1 second delay in runtime
;===========================================================    
    
DELAY_1S:
    ; nested loops using REG10/REG11
    MOVLW   0xFF
    MOVWF   REG11,0
DLY_OUTER:
    MOVLW   0xFF
    MOVWF   REG10,0
DLY_INNER:
    DECFSZ  REG10,1,0
    BRA     DLY_INNER
    DECFSZ  REG11,1,0
    BRA     DLY_OUTER
    RETURN
    
    END
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    STATE_WAIT_DIGIT1 = 0,
    STATE_WAIT_GAP1,
    STATE_WAIT_DIGIT2,
    STATE_WAIT_GAP2,
    STATE_CHECK_CODE,
    STATE_UNLOCK,
    STATE_ERROR
} system_state_t;

extern volatile uint8_t emergency_flag;
extern uint8_t secret_digit1;
extern uint8_t secret_digit2;

void SevenSeg_DisplayDigit(uint8_t digit);
void SevenSeg_Blank(void);

void ConfirmLED_Pulse(void);
void Buzzer_Beep(uint16_t ms);
void Buzzer_ErrorTone(void);

void Relay_On(void);
void Relay_Off(void);

void ResetEntry(void);
void ProcessStateMachine(void);
void ScanSensorEdges(void);

// Keypad
char Keypad_ScanRaw(void);
char Keypad_GetKeyDebounced(void);
void Keypad_WaitRelease(void);
uint8_t Keypad_ReadDigit_1to4(void);
void Keypad_SetSecretCode(void);

#endif
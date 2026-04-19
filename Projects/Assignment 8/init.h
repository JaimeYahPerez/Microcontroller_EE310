#ifndef INIT_H
#define INIT_H

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

// --------------------
// Main project pins
// --------------------
#define SYS_LED             LATCbits.LATC4
#define CONFIRM_LED         LATBbits.LATB3
#define RELAY_OUT           LATCbits.LATC5
#define BUZZER_OUT          LATCbits.LATC6

#define PR1_IN              PORTBbits.RB1
#define PR2_IN              PORTBbits.RB2
#define EMERGENCY_IN        PORTBbits.RB0

// --------------------
// Keypad pins
// Rows = outputs
// Cols = inputs
// --------------------
#define KP_R1               LATAbits.LATA0
#define KP_R2               LATAbits.LATA1
#define KP_R3               LATAbits.LATA2
#define KP_R4               LATAbits.LATA3

#define KP_C1               PORTCbits.RC2
#define KP_C2               PORTCbits.RC3
#define KP_C3               PORTCbits.RC7
#define KP_C4               PORTBbits.RB4

// --------------------
// Build options
// --------------------
#define USE_KEYPAD_SECRET   1

// Default code if keypad is not used
#define DEFAULT_SECRET_DIGIT1   2
#define DEFAULT_SECRET_DIGIT2   3

// --------------------
// Timing
// --------------------
#define LOOP_DELAY_MS           10
#define DIGIT_GAP_TICKS         150   // 1.5 s at 10 ms loop
#define SENSOR_DEBOUNCE_MS      30
#define BUTTON_DEBOUNCE_MS      25
#define KEYPAD_DEBOUNCE_MS      20

void SYSTEM_Initialize(void);
void PORTS_Initialize(void);
void INTERRUPT_Initialize(void);
void KEYPAD_Initialize(void);

#endif
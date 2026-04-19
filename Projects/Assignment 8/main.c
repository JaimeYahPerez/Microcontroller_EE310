/*
 * ---------------------
 * Title: IO Ports Relay
 * ---------------------
 * Program Details:
 *      This program is designed to be the logical infrastructure for a lockbox.
 * The user can input a passcode with the keypad which the program will remember.
 * Two photoresistors are used as code inputs with the current digit to be 
 * entered appearing on a 7 segment display, and input being confirmed after a 
 * timeout and an led flash confirmation. If the correct input is entered, the 
 * relay will be activated and a motor will be powered. If incorrect or an 
 * interrupt button is pressed, a buzzer will go off and a system reset will occur.
 * 
 * Inputs: PORTA, PORTB, PORTC
 *
 * PORTA Mapping (Keypad Rows - Outputs during scan):
 *  RA0 - Keypad Row 1
 *  RA1 - Keypad Row 2
 *  RA2 - Keypad Row 3
 *  RA3 - Keypad Row 4
 *
 * PORTB Mapping:
 *  RB0 - Interrupt Button (INT0)
 *  RB1 - Photoresistor 1 (First Input)
 *  RB2 - Photoresistor 2 (Second Input)
 *  RB3 - Confirm/Input Accepted LED
 *  RB4 - Keypad Column 4
 * 
 * PORTC Mapping:
 *  RC2 - Keypad Column 1
 *  RC3 - Keypad Column 2
 *  RC4 - System ON LED
 *  RC5 - Motor / Relay Output (Correct Code)
 *  RC6 - Incorrect Code / Interrupt Buzzer
 *  RC7 - Keypad Column 3
 *
  * Outputs: PORTB, PORTC, PORTD
 *
 * PORTD Mapping (7-Segment Display):
 *  RD0 - Segment A
 *  RD1 - Segment B
 *  RD2 - Segment C
 *  RD3 - Segment D
 *  RD4 - Segment E
 *  RD5 - Segment F
 *  RD6 - Segment G
 * 
 * Setup: C- Simulator
 * Date: April 18th, 2025
 * File Dependencies / Libraries: It is required to include the 
 * ConfigureFile Header File, Functions Header File, and Init Header File,
 * along with their functions.c and init.c file
 * Compiler: Xc8, 3.10
 * Author: Jaime Yah-Perez
 * Versions:
 *      V1.0: first implementations of required files
 *      V1.1: fixed bouncing issue and moved away from pin RC1
 *      V1.2: implemented keypad and changing code capability
 * Useful links:  
 *      Datasheet: https://ww1.microchip.com/downloads/en/DeviceDoc/PIC18(L)F26-27-45-46-47-55-56-57K42-Data-Sheet-40001919G.pdf 
 */
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "Config.h"
#include "init.h"
#include "functions.h"

void __interrupt(irq(IRQ_INT0), base(8)) INT0_ISR(void)
{
    PIE1bits.INT0IE = 0;    
    PIR1bits.INT0IF = 0;

    __delay_ms(BUTTON_DEBOUNCE_MS);

    // Active-low emergency button
    if(EMERGENCY_IN == 0)
    {
        emergency_flag = 1;

        
        BUZZER_OUT = 1; __delay_ms(100);
        BUZZER_OUT = 0; __delay_ms(80);
        BUZZER_OUT = 1; __delay_ms(250);
        BUZZER_OUT = 0;

        // Wait for button release so bounce/hold does not retrigger
        while(EMERGENCY_IN == 0)
        {
            __delay_ms(1);
        }

        __delay_ms(BUTTON_DEBOUNCE_MS);
    }

    PIR1bits.INT0IF = 0;
    PIE1bits.INT0IE = 1;
}

void main(void)
{
    SYSTEM_Initialize();

#if USE_KEYPAD_SECRET
    Keypad_SetSecretCode();
#endif

    ResetEntry();

    while(1)
    {
        if(emergency_flag)
        {
            Relay_Off();
            SevenSeg_Blank();
            CONFIRM_LED = 0;

            emergency_flag = 0;
            ResetEntry();
        }

        ScanSensorEdges();
        ProcessStateMachine();

        __delay_ms(LOOP_DELAY_MS);
    }
}
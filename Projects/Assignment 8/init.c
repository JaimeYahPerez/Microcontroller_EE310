#include <xc.h>
#include "Config.h"
#include "init.h"
void KEYPAD_Initialize(void)
{
    // RA0-RA3 are analog-capable, so force them digital
    ANSELAbits.ANSELA0 = 0;
    ANSELAbits.ANSELA1 = 0;
    ANSELAbits.ANSELA2 = 0;
    ANSELAbits.ANSELA3 = 0;

    // Rows = outputs
    TRISAbits.TRISA0 = 0;
    TRISAbits.TRISA1 = 0;
    TRISAbits.TRISA2 = 0;
    TRISAbits.TRISA3 = 0;

    // idle rows high
    KP_R1 = 1;
    KP_R2 = 1;
    KP_R3 = 1;
    KP_R4 = 1;

    // Columns = inputs
    TRISCbits.TRISC2 = 1;
    TRISCbits.TRISC3 = 1;
    TRISCbits.TRISC7 = 1;
    TRISBbits.TRISB4 = 1;

    // Internal weak pull-ups for keypad columns
    WPUCbits.WPUC2 = 1;
    WPUCbits.WPUC3 = 1;
    WPUCbits.WPUC7 = 1;
    WPUBbits.WPUB4 = 1;
}

void PORTS_Initialize(void)
{
    // Make used pins digital
    ANSELB = 0x00;
    ANSELC = 0x00;
    ANSELD = 0x00;

    // Inputs
    TRISBbits.TRISB0 = 1;   // emergency button
    TRISBbits.TRISB1 = 1;   // PR1
    TRISBbits.TRISB2 = 1;   // PR2

    // Outputs
    TRISBbits.TRISB3 = 0;   // confirm LED
    TRISCbits.TRISC4 = 0;   // system LED
    TRISCbits.TRISC5 = 0;   // relay control
    TRISCbits.TRISC6 = 0;   // buzzer
    TRISD = 0x00;           // 7-segment

    // Use internal weak pull-up for emergency button
    // Assumes button goes from RB0 to GND
    WPUBbits.WPUB0 = 1;

    // Clear outputs
    LATBbits.LATB3 = 0;
    LATCbits.LATC4 = 0;
    LATCbits.LATC5 = 0;
    LATCbits.LATC6 = 0;
    LATD = 0x00;
}

void INTERRUPT_Initialize(void)
{
    INTCON0bits.GIE = 0;
    INTCON0bits.IPEN = 0;

    // Active-low button with pull-up -> falling edge
    INTCON0bits.INT0EDG = 0;

    PIR1bits.INT0IF = 0;
    PIE1bits.INT0IE = 1;

    INTCON0bits.GIE = 1;
}

void SYSTEM_Initialize(void)
{
    PORTS_Initialize();
    KEYPAD_Initialize();
    INTERRUPT_Initialize();

    SYS_LED = 1;
}
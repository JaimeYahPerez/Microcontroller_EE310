/*
 * File: PWM.h
 * Timer2 + CCP2 PWM helper functions for PIC18F47K42
 */

#ifndef PWM_H
#define PWM_H

#include <xc.h>
#include <stdint.h>

void TMR2_Initialize(void)
{
    // Timer2 clock source = Fosc/4.
    T2CLKCON = 0x01;

    // Free-running Timer2 mode.
    T2HLT = 0x00;
    T2RST = 0x00;

    /*
     * Servo period setup:
     *
     * Fosc = 4 MHz
     * Tosc = 0.25 us
     * Prescale = 1:128
     * T2PR = 155
     *
     * PWM Period = (T2PR + 1) * 4 * Tosc * Prescale
     *            = (155 + 1) * 4 * 0.25 us * 128
     *            = 19,968 us ? 20 ms
     */
    T2PR = 0x9B;     // 155 decimal

    // Clear Timer2 counter.
    T2TMR = 0x00;

    // Clear Timer2 interrupt flag.
    PIR4bits.TMR2IF = 0;

    /*
     * T2CON = 0xF0
     *
     * bit 7    ON = 1
     * bits 6:4 CKPS = 111 = 1:128 prescale
     * bits 3:0 OUTPS = 0000 = 1:1 postscale
     */
    T2CON = 0xF0;
}

void TMR2_Start(void)
{
    T2CONbits.TMR2ON = 1;
}

void TMR2_StartTimer(void)
{
    TMR2_Start();
}

void TMR2_Stop(void)
{
    T2CONbits.TMR2ON = 0;
}

void TMR2_StopTimer(void)
{
    TMR2_Stop();
}

uint8_t TMR2_Counter8BitGet(void)
{
    return T2TMR;
}

uint8_t TMR2_ReadTimer(void)
{
    return TMR2_Counter8BitGet();
}

void TMR2_Counter8BitSet(uint8_t timerVal)
{
    T2TMR = timerVal;
}

void TMR2_WriteTimer(uint8_t timerVal)
{
    TMR2_Counter8BitSet(timerVal);
}

void TMR2_Period8BitSet(uint8_t periodVal)
{
    T2PR = periodVal;
}

void TMR2_LoadPeriodRegister(uint8_t periodVal)
{
    TMR2_Period8BitSet(periodVal);
}

/*
 * This is kept from the original sample.
 * It maps the real CCP2 output to RB3/D8 through PPS.
 *
 * For the simpler simulation option, RB2 is still produced by software-copying
 * CCP2CONbits.OUT inside main().
 */
void PWM_Output_D8_Enable(void)
{
    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 0x00;

    // Route CCP2 output to RB3/D8.
    RB3PPS = 0x0A;

    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 0x01;
}

void PWM_Output_D8_Disable(void)
{
    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 0x00;

    RB3PPS = 0x00;

    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 0x01;

    TRISBbits.TRISB3 = 0;
}

void PWM2_Initialize(void)
{
    // Disable CCP2 while configuring.
    CCP2CON = 0x00;

    // Clear duty registers.
    CCPR2H = 0x00;
    CCPR2L = 0x00;

    // Select Timer2 as CCP2 timer source.
    CCPTMRS0bits.C2TSEL = 0x1;

    /*
     * CCP2CON = 0x8C
     *
     * bit 7 EN = 1, CCP2 enabled
     * bit 4 FMT = 0, right-aligned duty format
     * bits 3:0 MODE = 1100, PWM mode
     */
    CCP2CON = 0x8C;
}

void PWM2_LoadDutyValue(uint16_t dutyValue)
{
    // CCP PWM duty value is 10-bit.
    dutyValue &= 0x03FF;

    if (CCP2CONbits.FMT)
    {
        // Left-aligned format.
        dutyValue <<= 6;
        CCPR2H = dutyValue >> 8;
        CCPR2L = dutyValue;
    }
    else
    {
        // Right-aligned format.
        CCPR2H = dutyValue >> 8;
        CCPR2L = dutyValue;
    }
}

_Bool PWM2_OutputStatusGet(void)
{
    return CCP2CONbits.OUT;
}

#endif /* PWM_H */
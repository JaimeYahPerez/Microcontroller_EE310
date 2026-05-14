/*
* ---------------------
 * Title: Servo Motor Control 
 * ---------------------
 * Program Details: A simple program to control a servo motor with two switches
 * using PWM
 * Author: Jaime Yah-Perez
 * Inputs: RB0, RB1
 * Outputs: Outputs RB2
 * Setup: C- Simulator
 * Date: May 14, 2026
 * 
 * File Dependencies / Libraries: PWM.h is required
 * Compiler: Xc8, 3.10
 * Versions:
 *  1.0 first initial setup for simulation with PWM
 * 1.1 Switches and servo I/O included for simulation
 * 1.2 Corrections for actual circuit
 */

#include <xc.h>
#include <stdint.h>
#include "ConfigFile.h"
#include "PWM.h"

#define _XTAL_FREQ 4000000UL
#define FCY        (_XTAL_FREQ / 4)

// Switches use internal pull-ups.
// Released = 1
// Pressed  = 0
#define SW_LEFT   PORTBbits.RB0
#define SW_RIGHT  PORTBbits.RB1

// RB2 is the simulated PWM output.
#define PWM_SIM_OUT LATBbits.LATB2

// Servo pulse-width values using 32 us per CCPR2 count.
#define SERVO_LEFT_COUNT    31   // about 0.992 ms
#define SERVO_CENTER_COUNT  47   // about 1.504 ms
#define SERVO_RIGHT_COUNT   63   // about 2.016 ms

// How many 20 ms PWM periods to wait before moving one count.
// 2 means the servo position changes every ~40 ms.
#define MOVE_FRAME_DELAY    2

volatile uint16_t servoDuty = SERVO_CENTER_COUNT;
volatile uint16_t pulseWidth_us = 1504;
volatile _Bool pwmStatus = 0;

static void OSCILLATOR_Initialize(void)
{
    // HFINTOSC selected, no divider.
    OSCCON1 = 0x60;

    // 4 MHz internal oscillator.
    OSCFRQ = 0x02;
}

static void IO_Initialize(void)
{
    // Make PORTB digital.
    ANSELB = 0x00;

    // Clear output latch.
    LATB = 0x00;

    // RB0 and RB1 are switch inputs.
    TRISBbits.TRISB0 = 1;
    TRISBbits.TRISB1 = 1;

    // RB2 is software-mirrored PWM output.
    TRISBbits.TRISB2 = 0;

    // Enable weak pull-ups on RB0 and RB1.
    WPUBbits.WPUB0 = 1;
    WPUBbits.WPUB1 = 1;
}

static void Servo_SetDuty(uint16_t newDuty)
{
    if (newDuty < SERVO_LEFT_COUNT)
    {
        newDuty = SERVO_LEFT_COUNT;
    }
    else if (newDuty > SERVO_RIGHT_COUNT)
    {
        newDuty = SERVO_RIGHT_COUNT;
    }

    servoDuty = newDuty;
    PWM2_LoadDutyValue(servoDuty);

    // For debugging/watch window.
    // Each count is 32 us with this Timer2 setup.
    pulseWidth_us = servoDuty * 32;
}

void main(void)
{
    uint8_t moveFrameCounter = 0;

    OSCILLATOR_Initialize();
    IO_Initialize();

    TMR2_Initialize();
    PWM2_Initialize();

    // Start servo in center position.
    Servo_SetDuty(SERVO_CENTER_COUNT);

    TMR2_StartTimer();

    // Optional: keep the original sample PPS setup.
    // This is not what creates RB2. RB2 is created by the software mirror below.
    PWM_Output_D8_Enable();

    while (1)
    {
        /*
         * This is what makes RB2 show the PWM in the simulator.
         */
        pwmStatus = PWM2_OutputStatusGet();
        PWM_SIM_OUT = pwmStatus;

        /*
         * Timer2 flag happens once per PWM period, about every 20 ms.
         * Use this event to update the servo position slowly without using delay.
         */
        if (PIR4bits.TMR2IF == 1)
        {
            PIR4bits.TMR2IF = 0;

            moveFrameCounter++;

            if (moveFrameCounter >= MOVE_FRAME_DELAY)
            {
                moveFrameCounter = 0;

                if ((SW_LEFT == 0) && (SW_RIGHT == 1))
                {
                    // SW1 pressed: move left slowly.
                    if (servoDuty > SERVO_LEFT_COUNT)
                    {
                        Servo_SetDuty(servoDuty - 1);
                    }
                }
                else if ((SW_LEFT == 1) && (SW_RIGHT == 0))
                {
                    // SW2 pressed: move right slowly.
                    if (servoDuty < SERVO_RIGHT_COUNT)
                    {
                        Servo_SetDuty(servoDuty + 1);
                    }
                }
                else
                {
                    // No switch: hold current position.
                    //Servo_SetDuty(servoDuty);
                }
            }
        }
    }
}
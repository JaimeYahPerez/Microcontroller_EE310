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
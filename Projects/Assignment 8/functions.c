#include <xc.h>
#include "Config.h"
#include "init.h"
#include "functions.h"

volatile uint8_t emergency_flag = 0;

uint8_t secret_digit1 = DEFAULT_SECRET_DIGIT1;
uint8_t secret_digit2 = DEFAULT_SECRET_DIGIT2;

static system_state_t state = STATE_WAIT_DIGIT1;

static uint8_t digit1 = 0;
static uint8_t digit2 = 0;

static uint8_t pr1_count = 0;
static uint8_t pr2_count = 0;

static uint8_t pr1_armed = 1;
static uint8_t pr2_armed = 1;

static uint16_t idle_ticks = 0;

// --------------------
// 7-segment
// --------------------
void SevenSeg_DisplayDigit(uint8_t digit)
{
    switch(digit)
    {
        case 0: LATD = 0b00111111; break;
        case 1: LATD = 0b00000110; break;
        case 2: LATD = 0b01011011; break;
        case 3: LATD = 0b01001111; break;
        case 4: LATD = 0b01100110; break;
        default: LATD = 0x00; break;
    }
}

void SevenSeg_Blank(void)
{
    LATD = 0x00;
}

void ConfirmLED_Pulse(void)
{
    CONFIRM_LED = 1;
    __delay_ms(120);
    CONFIRM_LED = 0;
}

void Buzzer_Beep(uint16_t ms)
{
    BUZZER_OUT = 1;
    while(ms--)
    {
        __delay_ms(1);
    }
    BUZZER_OUT = 0;
}

void Buzzer_ErrorTone(void)
{
    Buzzer_Beep(300);
    __delay_ms(100);
    Buzzer_Beep(300);
}

void Relay_On(void)
{
    RELAY_OUT = 1;   // flip if relay module is active-low
}

void Relay_Off(void)
{
    RELAY_OUT = 0;   // flip if relay module is active-low
}

void ResetEntry(void)
{
    state = STATE_WAIT_DIGIT1;

    digit1 = 0;
    digit2 = 0;

    pr1_count = 0;
    pr2_count = 0;

    pr1_armed = 1;
    pr2_armed = 1;

    idle_ticks = 0;

    Relay_Off();
    SevenSeg_Blank();
    CONFIRM_LED = 0;
}

// --------------------
// Keypad helpers
// --------------------
static void Keypad_AllRowsHigh(void)
{
    KP_R1 = 1;
    KP_R2 = 1;
    KP_R3 = 1;
    KP_R4 = 1;
}

char Keypad_ScanRaw(void)
{
    // Row 1
    Keypad_AllRowsHigh();
    KP_R1 = 0;
    __delay_us(20);
    if(KP_C1 == 0) return '1';
    if(KP_C2 == 0) return '2';
    if(KP_C3 == 0) return '3';
    if(KP_C4 == 0) return 'A';

    // Row 2
    Keypad_AllRowsHigh();
    KP_R2 = 0;
    __delay_us(20);
    if(KP_C1 == 0) return '4';
    if(KP_C2 == 0) return '5';
    if(KP_C3 == 0) return '6';
    if(KP_C4 == 0) return 'B';

    // Row 3
    Keypad_AllRowsHigh();
    KP_R3 = 0;
    __delay_us(20);
    if(KP_C1 == 0) return '7';
    if(KP_C2 == 0) return '8';
    if(KP_C3 == 0) return '9';
    if(KP_C4 == 0) return 'C';

    // Row 4
    Keypad_AllRowsHigh();
    KP_R4 = 0;
    __delay_us(20);
    if(KP_C1 == 0) return '*';
    if(KP_C2 == 0) return '0';
    if(KP_C3 == 0) return '#';
    if(KP_C4 == 0) return 'D';

    Keypad_AllRowsHigh();
    return 0;
}

char Keypad_GetKeyDebounced(void)
{
    char k1 = Keypad_ScanRaw();
    char k2;

    if(k1 == 0)
    {
        return 0;
    }

    __delay_ms(KEYPAD_DEBOUNCE_MS);
    k2 = Keypad_ScanRaw();

    if(k1 == k2)
    {
        return k1;
    }

    return 0;
}

void Keypad_WaitRelease(void)
{
    while(Keypad_ScanRaw() != 0)
    {
        __delay_ms(10);
    }
    __delay_ms(KEYPAD_DEBOUNCE_MS);
}

uint8_t Keypad_ReadDigit_1to4(void)
{
    char key = 0;

    while(1)
    {
        key = Keypad_GetKeyDebounced();

        if((key >= '1') && (key <= '4'))
        {
            SevenSeg_DisplayDigit((uint8_t)(key - '0'));
            ConfirmLED_Pulse();
            Keypad_WaitRelease();
            return (uint8_t)(key - '0');
        }
    }
}

void Keypad_SetSecretCode(void)
{
#if USE_KEYPAD_SECRET
    SevenSeg_Blank();
    Keypad_WaitRelease();

    secret_digit1 = Keypad_ReadDigit_1to4();
    __delay_ms(300);

    secret_digit2 = Keypad_ReadDigit_1to4();
    __delay_ms(300);

    Buzzer_Beep(80);
    SevenSeg_Blank();
#endif
}

// --------------------
// Photoresistor counting
// Assumes active state = HIGH
// --------------------
void ScanSensorEdges(void)
{
    uint8_t pr1_now = PR1_IN ? 1 : 0;
    uint8_t pr2_now = PR2_IN ? 1 : 0;

    if((state == STATE_WAIT_DIGIT1) || (state == STATE_WAIT_GAP1))
    {
        if(pr1_armed && (pr1_now == 1))
        {
            __delay_ms(SENSOR_DEBOUNCE_MS);
            if(PR1_IN)
            {
                if(pr1_count < 4)
                {
                    pr1_count++;
                }
                idle_ticks = 0;
                state = STATE_WAIT_GAP1;
                pr1_armed = 0;
            }
        }

        if((pr1_armed == 0) && (pr1_now == 0))
        {
            __delay_ms(15);
            if(PR1_IN == 0)
            {
                pr1_armed = 1;
            }
        }
    }

    if((state == STATE_WAIT_DIGIT2) || (state == STATE_WAIT_GAP2))
    {
        if(pr2_armed && (pr2_now == 1))
        {
            __delay_ms(SENSOR_DEBOUNCE_MS);
            if(PR2_IN)
            {
                if(pr2_count < 4)
                {
                    pr2_count++;
                }
                idle_ticks = 0;
                state = STATE_WAIT_GAP2;
                pr2_armed = 0;
            }
        }

        if((pr2_armed == 0) && (pr2_now == 0))
        {
            __delay_ms(15);
            if(PR2_IN == 0)
            {
                pr2_armed = 1;
            }
        }
    }
}

void ProcessStateMachine(void)
{
    switch(state)
    {
        case STATE_WAIT_DIGIT1:
            break;

        case STATE_WAIT_GAP1:
            idle_ticks++;
            if(idle_ticks >= DIGIT_GAP_TICKS)
            {
                if(pr1_count == 0)
                {
                    ResetEntry();
                    break;
                }

                digit1 = pr1_count;
                SevenSeg_DisplayDigit(digit1);
                ConfirmLED_Pulse();

                idle_ticks = 0;
                state = STATE_WAIT_DIGIT2;
            }
            break;

        case STATE_WAIT_DIGIT2:
            break;

        case STATE_WAIT_GAP2:
            idle_ticks++;
            if(idle_ticks >= DIGIT_GAP_TICKS)
            {
                if(pr2_count == 0)
                {
                    ResetEntry();
                    break;
                }

                digit2 = pr2_count;
                SevenSeg_DisplayDigit(digit2);
                ConfirmLED_Pulse();

                state = STATE_CHECK_CODE;
            }
            break;

        case STATE_CHECK_CODE:
            if((digit1 == secret_digit1) && (digit2 == secret_digit2))
            {
                state = STATE_UNLOCK;
            }
            else
            {
                state = STATE_ERROR;
            }
            break;

        case STATE_UNLOCK:
            Relay_On();
            __delay_ms(2000);
            Relay_Off();
            ResetEntry();
            break;

        case STATE_ERROR:
            Buzzer_ErrorTone();
            ResetEntry();
            break;

        default:
            ResetEntry();
            break;
    }
}
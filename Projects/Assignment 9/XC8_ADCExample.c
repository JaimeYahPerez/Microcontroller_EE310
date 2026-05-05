/*
 * ---------------------
 * Title: ADC to Digital Converters
 * ---------------------
 * 
 * Program Details:
 *      This program is designed to serve as a measure of the noise level in a
 * room. The readings are done by a microphone and ADC catching sound as analog 
 * inputs and converting them into digital signals. Which are then displayed to
 * and LCD screen as a percentage reading of either too quiet or too loud of a
 * noise reading. and Interrupt switch using IOC has also been incorporated to 
 * stop readings. The microphone normal range is too large to provide proper 
 * readings, and so has been adjusted to scale down the readings in the code to
 * a more reasonable range.
 * 
 * Inputs/Outputs
 * LCD wiring, 4-bit mode:
 * LCD VSS -> GND
 * LCD VDD -> +5V / VTG
 * LCD V0  -> contrast potentiometer middle pin
 * LCD RS  -> RB0
 * LCD RW  -> GND
 * LCD E   -> RB1
 * LCD D4  -> RD4
 * LCD D5  -> RD5
 * LCD D6  -> RD6
 * LCD D7  -> RD7
 * LCD A   -> +5V through resistor if needed
 * LCD K   -> GND
 *
 * Microphone module:
 * MIC VCC -> +5V / VTG
 * MIC GND -> GND
 * MIC AO  -> RA1 / ANA1
 * MIC DO  -> not used
 *
 * Interrupt:
 * Button: RC2 -> button -> GND
 * Red LED: RC3 -> resistor -> LED -> GND
 * 
 * Setup: C-simulator
 * PIC18F47K42
 * Date: May 4th, 2025
 * Compiler: Xc8, 3.10
 * Author: Jaime Yah-Perez
 * Versions:
 *      V1.0: first implementation of the LCD/ADC voltage reader w/ potentiometer
 *      V1.1: Microphone replacement of potentiometer, and sound level reading 
 *      V1.2: Microphone reading scale adjusted, and IOCC interrupt integration
 */

#include <xc.h>
#include <stdint.h>
#include <stdio.h>

#define _XTAL_FREQ 4000000UL

// --------------------------------------------------
// CONFIG BITS
// --------------------------------------------------

#pragma config FEXTOSC = OFF
#pragma config RSTOSC = HFINTOSC_1MHZ

#pragma config CLKOUTEN = OFF
#pragma config PR1WAY = ON
#pragma config CSWEN = ON
#pragma config FCMEN = ON

#pragma config MCLRE = EXTMCLR
#pragma config PWRTS = PWRT_OFF
#pragma config MVECEN = OFF
#pragma config IVT1WAY = ON
#pragma config LPBOREN = OFF
#pragma config BOREN = SBORDIS

#pragma config BORV = VBOR_2P45
#pragma config ZCD = OFF
#pragma config PPS1WAY = ON
#pragma config STVREN = ON
#pragma config DEBUG = OFF
#pragma config XINST = OFF

#pragma config WDTCPS = WDTCPS_31
#pragma config WDTE = OFF

#pragma config WDTCWS = WDTCWS_7
#pragma config WDTCCS = SC

#pragma config BBSIZE = BBSIZE_512
#pragma config BBEN = OFF
#pragma config SAFEN = OFF
#pragma config WRTAPP = OFF

#pragma config WRTB = OFF
#pragma config WRTC = OFF
#pragma config WRTD = OFF
#pragma config WRTSAF = OFF
#pragma config LVP = ON

#pragma config CP = OFF

// --------------------------------------------------
// LCD PIN DEFINITIONS
// --------------------------------------------------

#define LCD_RS LATBbits.LATB0
#define LCD_EN LATBbits.LATB1

#define LCD_D4 LATDbits.LATD4
#define LCD_D5 LATDbits.LATD5
#define LCD_D6 LATDbits.LATD6
#define LCD_D7 LATDbits.LATD7

// --------------------------------------------------
// MICROPHONE SETTINGS
// --------------------------------------------------

#define MIC_SAMPLE_COUNT       300u
#define MIC_SAMPLE_DELAY_US    250u

/*
 * These are relative to the full 0-5V ADC range.
 */
#define MIC_FULL_SCALE_COUNTS 100u
#define TOO_QUIET_PERCENT      1u
#define TOO_LOUD_PERCENT       80u

// --------------------------------------------------
// GLOBAL VARIABLES
// --------------------------------------------------

volatile uint8_t halt_request = 0;

uint16_t mic_min = 0;
uint16_t mic_max = 0;
uint16_t mic_peak_to_peak = 0;
uint8_t sound_percent = 0;

char data[17];

// --------------------------------------------------
// FUNCTION PROTOTYPES
// --------------------------------------------------

void System_Init(void);
void ADC_Init(void);
uint16_t ADC_Read(void);

void IOC_Init(void);
void Halt_For_10_Seconds(void);

void LCD_EnablePulse(void);
void LCD_SendNibble(uint8_t nibble);
void LCD_Command(uint8_t command);
void LCD_Char(char data);
void LCD_String(const char *str);
void LCD_SetCursor(uint8_t row, uint8_t column);
void LCD_Clear(void);
void LCD_ClearLine(uint8_t row);
void LCD_Init(void);

uint16_t Mic_ReadPeakToPeak(void);
uint8_t Mic_GetPercent(uint16_t peak_to_peak_count);

// --------------------------------------------------
// INTERRUPT SERVICE ROUTINE
// --------------------------------------------------

void __interrupt() ISR(void)
{
    if (IOCCFbits.IOCCF2)
    {
        IOCCFbits.IOCCF2 = 0;   // Clear RC2 interrupt flag
        PIR0bits.IOCIF = 0;     // Clear general IOC flag
        halt_request = 1;       
    }
}

// --------------------------------------------------
// LCD FUNCTIONS
// --------------------------------------------------

void LCD_EnablePulse(void)
{
    LCD_EN = 1;
    __delay_us(20);     // slowed down because your LCD needed it
    LCD_EN = 0;
    __delay_us(250);
}

void LCD_SendNibble(uint8_t nibble)
{
    LCD_D4 = (nibble >> 0) & 0x01;
    LCD_D5 = (nibble >> 1) & 0x01;
    LCD_D6 = (nibble >> 2) & 0x01;
    LCD_D7 = (nibble >> 3) & 0x01;

    LCD_EnablePulse();
}

void LCD_Command(uint8_t command)
{
    LCD_RS = 0;

    LCD_SendNibble(command >> 4);
    LCD_SendNibble(command & 0x0F);

    if (command == 0x01 || command == 0x02)
    {
        __delay_ms(3);
    }
    else
    {
        __delay_us(100);
    }
}

void LCD_Char(char data)
{
    LCD_RS = 1;

    LCD_SendNibble(data >> 4);
    LCD_SendNibble(data & 0x0F);

    __delay_us(100);
}

void LCD_String(const char *str)
{
    while (*str)
    {
        LCD_Char(*str++);
    }
}

void LCD_SetCursor(uint8_t row, uint8_t column)
{
    uint8_t address;

    if (row == 1)
    {
        address = 0x80 + (column - 1);
    }
    else
    {
        address = 0xC0 + (column - 1);
    }

    LCD_Command(address);
}

void LCD_Clear(void)
{
    LCD_Command(0x01);
    __delay_ms(3);
}

void LCD_ClearLine(uint8_t row)
{
    LCD_SetCursor(row, 1);
    LCD_String("                ");
    LCD_SetCursor(row, 1);
}

void LCD_Init(void)
{
    __delay_ms(60);

    LCD_RS = 0;
    LCD_EN = 0;

    LCD_SendNibble(0x03);
    __delay_ms(10);

    LCD_SendNibble(0x03);
    __delay_ms(10);

    LCD_SendNibble(0x03);
    __delay_ms(10);

    LCD_SendNibble(0x02);
    __delay_ms(10);

    LCD_Command(0x28);      // 4-bit, 2-line, 5x8
    LCD_Command(0x0C);      // display on, cursor off
    LCD_Command(0x06);      // increment cursor
    LCD_Clear();
}

// --------------------------------------------------
// ADC FUNCTIONS
// --------------------------------------------------

void ADC_Init(void)
{
    // RA1 / ANA1 analog input
    TRISAbits.TRISA1 = 1;
    ANSELAbits.ANSELA1 = 1;

    // ADC references: VREF+ = VDD, VREF- = VSS
    ADREFbits.PREF = 0b00;
    ADREFbits.NREF = 0;

    // Select ANA1
    ADPCH = 0x01;

    // Right justified ADC result
    ADCON0bits.FM = 1;

    // ADC internal clock
    ADCON0bits.CS = 1;

    // Acquisition time
    ADACQL = 0x20;
    ADACQH = 0x00;

    ADRESH = 0x00;
    ADRESL = 0x00;

    ADCON0bits.ON = 1;
}

uint16_t ADC_Read(void)
{
    ADCON0bits.GO = 1;
    while (ADCON0bits.GO);

    return ((uint16_t)ADRESH << 8) | ADRESL;
}

// --------------------------------------------------
// MICROPHONE FUNCTIONS
// --------------------------------------------------

uint16_t Mic_ReadPeakToPeak(void)
{
    uint16_t sample;
    uint16_t min_value = 4095;
    uint16_t max_value = 0;

    for (uint16_t i = 0; i < MIC_SAMPLE_COUNT; i++)
    {
        sample = ADC_Read();

        if (sample < min_value)
        {
            min_value = sample;
        }

        if (sample > max_value)
        {
            max_value = sample;
        }

        __delay_us(MIC_SAMPLE_DELAY_US);
    }

    mic_min = min_value;
    mic_max = max_value;

    return max_value - min_value;
}

uint8_t Mic_GetPercent(uint16_t peak_to_peak_count)
{
    uint32_t percent;

    percent = ((uint32_t)peak_to_peak_count * 100UL) / MIC_FULL_SCALE_COUNTS;

    if (percent > 100UL)
    {
        percent = 100UL;
    }

    return (uint8_t)percent;
}

// --------------------------------------------------
// IOC INTERRUPT SETUP
// --------------------------------------------------

void IOC_Init(void)
{
    // RC2 = interrupt button input
    // RC3 = red HALT LED output

    ANSELCbits.ANSELC2 = 0;
    ANSELCbits.ANSELC3 = 0;

    TRISCbits.TRISC2 = 1;     // button input
    TRISCbits.TRISC3 = 0;     // LED output

    LATCbits.LATC3 = 0;

    // Weak pull-up on RC2
    WPUCbits.WPUC2 = 1;

    // Interrupt-on-change on falling edge
    IOCCPbits.IOCCP2 = 0;     // no rising edge
    IOCCNbits.IOCCN2 = 1;     // falling edge enabled
    IOCCFbits.IOCCF2 = 0;     // clear RC2 flag

    PIR0bits.IOCIF = 0;
    PIE0bits.IOCIE = 1;

    INTCON0bits.GIE = 1;
}

// --------------------------------------------------
// HALT MODE
// --------------------------------------------------

void Halt_For_10_Seconds(void)
{
    LCD_Clear();
    LCD_SetCursor(1, 1);
    LCD_String("SYSTEM HALTED");

    LCD_SetCursor(2, 1);
    LCD_String("ADC Paused");

    /*
     * 20 toggles x 500 ms = 10 seconds.
     */
    for (uint8_t i = 0; i < 20; i++)
    {
        LATCbits.LATC3 ^= 1;
        __delay_ms(500);
    }

    LATCbits.LATC3 = 0;

    LCD_Clear();
    LCD_SetCursor(1, 1);
    LCD_String("Sound Level:");
}

// --------------------------------------------------
// SYSTEM INITIALIZATION
// --------------------------------------------------

void System_Init(void)
{
    OSCFRQbits.HFFRQ = 0b0010;   // 4 MHz internal oscillator

    // LCD control pins: RB0, RB1
    ANSELBbits.ANSELB0 = 0;
    ANSELBbits.ANSELB1 = 0;

    TRISBbits.TRISB0 = 0;
    TRISBbits.TRISB1 = 0;

    LATBbits.LATB0 = 0;
    LATBbits.LATB1 = 0;

    // LCD data pins: RD4-RD7
    ANSELDbits.ANSELD4 = 0;
    ANSELDbits.ANSELD5 = 0;
    ANSELDbits.ANSELD6 = 0;
    ANSELDbits.ANSELD7 = 0;

    TRISDbits.TRISD4 = 0;
    TRISDbits.TRISD5 = 0;
    TRISDbits.TRISD6 = 0;
    TRISDbits.TRISD7 = 0;

    LATDbits.LATD4 = 0;
    LATDbits.LATD5 = 0;
    LATDbits.LATD6 = 0;
    LATDbits.LATD7 = 0;

    // Normal push-pull outputs
    ODCONBbits.ODCB0 = 0;
    ODCONBbits.ODCB1 = 0;

    ODCONDbits.ODCD4 = 0;
    ODCONDbits.ODCD5 = 0;
    ODCONDbits.ODCD6 = 0;
    ODCONDbits.ODCD7 = 0;
}

// --------------------------------------------------
// MAIN PROGRAM
// --------------------------------------------------

void main(void)
{
    System_Init();
    LCD_Init();
    ADC_Init();
    IOC_Init();

    LCD_SetCursor(1, 1);
    LCD_String("Sound Level:");

    while (1)
    {
        if (halt_request)
        {
            halt_request = 0;
            Halt_For_10_Seconds();
        }

        mic_peak_to_peak = Mic_ReadPeakToPeak();
        sound_percent = Mic_GetPercent(mic_peak_to_peak);

        LCD_ClearLine(2);

        if (sound_percent <= TOO_QUIET_PERCENT)
        {
            LCD_String("Too Quiet!");
        }
        else if (sound_percent >= TOO_LOUD_PERCENT)
        {
            LCD_String("Too Loud!");
        }
        else
        {
            sprintf(data, "Level: %3u%%", sound_percent);
            LCD_String(data);
        }

        __delay_ms(150);
    }
}
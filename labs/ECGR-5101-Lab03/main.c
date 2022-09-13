#include <msp430.h> 

/**
 * main.c
 * 09/08/2022
 * Grayson Bass, Sam Xu
 * ECGR 5101 Lab 02
 * Displays hex on a 7-segment display based on the input value from a potentiometer.
 */

// Define the output 7-segement leds pin numbers.
#define LEDa 0x01
#define LEDb 0x02
#define LEDc 0x04
#define LEDd 0x08
#define LEDe 0x10
#define LEDf 0x20
#define LEDg 0x40
#define LEDh 0x80

// Define 7-seg chars to hex values.
#define SEG0 0x40
#define SEG1 0xF9
#define SEG2 0x24
#define SEG3 0x30
#define SEG4 0x19
#define SEG5 0x12
#define SEG6 0x02
#define SEG7 0x78
#define SEG8 0x00
#define SEG9 0x10
#define SEGA 0x20
#define SEGB 0x03
#define SEGC 0x27
#define SEGD 0x21
#define SEGE 0x6
#define SEGF 0x0E


// Function prototypes.
unsigned int readAnalog();
unsigned short display7Seg();

unsigned int ADCValue = 0; // Holds the adc value for A0.


// Define analog input pin for P1.0
#define POT 0x01

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	// Setup 7-segment pins.
	// Set all port 2 pins to output and enable pull up resistor.
	P2DIR |= 0xFF;
	P2SEL &= ~BIT6;
	P2OUT |= 0xFF;

    // Setup adc0 for potentiometer.
    P1SEL |= POT; // Set pin to analog.
    ADC10AE0 = POT; // Select channel A0.
    ADC10CTL1 = INCH_0 + ADC10DIV_3; // Select Channel A0, ADC10CLK/3
    ADC10CTL0 = ADC10SHT_3 + MSC + ADC10ON;

    // Sampling and conversion start.
    ADC10CTL0 |= ENC + ADC10SC;

    while(1) {
        ADCValue = readAnalog();
        display7Seg(ADCValue);
    }

	return 0;
}

// Reads the A0 analog pin and returns the digital value.
unsigned int readAnalog() {
    // Sampling and conversion start.
    ADC10CTL0 |= ENC + ADC10SC;

    // Return the value held in ADC10MEM
    return ADC10MEM;
}

// Displays a passed in value to a 7-segment display.
unsigned short int display7Seg(unsigned int ADCValue) {

    // If the value is out of range display nothing.
    if(ADCValue > 1023) {
        P2OUT |= 0xFF; // Set all the leds to off.
        return 0;
    }
    P2OUT |= 0xFF; // Flush the current bits.

    // Display the values on the 7-segment display.
    if(ADCValue < 64)
        P2OUT &= SEG0;
    else if(ADCValue > 64 && ADCValue < 128)
        P2OUT &= SEG1;
    else if(ADCValue > 128 && ADCValue < 192)
        P2OUT &= SEG2;
    else if(ADCValue > 192 && ADCValue < 256)
        P2OUT &= SEG3;
    else if(ADCValue > 256 && ADCValue < 320)
        P2OUT &= SEG4;
    else if(ADCValue > 320 && ADCValue < 384)
        P2OUT &= SEG5;
    else if(ADCValue > 384 && ADCValue < 448)
        P2OUT &= SEG6;
    else if(ADCValue > 448 && ADCValue < 512)
        P2OUT &= SEG7;
    else if(ADCValue > 512 && ADCValue < 576)
        P2OUT &= SEG8;
    else if(ADCValue > 576 && ADCValue < 640)
        P2OUT &= SEG9;
    else if(ADCValue > 640 && ADCValue < 704)
        P2OUT &= SEGA;
    else if(ADCValue > 704 && ADCValue < 768)
        P2OUT &= SEGB;
    else if(ADCValue > 768 && ADCValue < 832)
        P2OUT &= SEGC;
    else if(ADCValue > 832 && ADCValue < 896)
        P2OUT &= SEGD;
    else if(ADCValue > 896 && ADCValue < 960)
        P2OUT &= SEGE;
    else if(ADCValue > 960 && ADCValue < 1024)
        P2OUT &= SEGF;

    return 0;
}

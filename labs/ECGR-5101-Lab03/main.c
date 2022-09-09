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

// Define masks for the pins.
#define LEDMASKa 0xFE
#define LEDMASKb 0xFD
#define LEDMASKc 0xFB
#define LEDMASKd 0xEF
#define LEDMASKe 0xDF
#define LEDMASKf 0xBF
#define LEDMASKg 0x7F

// Function prototypes.
unsigned int readAnalog();

// Define analog input pin for P1.1
#define POT BIT1

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	// Setup 7-segment pins.
	// Set all port 2 pins to output and enable pull up resistor.
	P2DIR |= 0xFF;
    P2REN |= 0xFF;
    P2OUT |= 0xFF;

    // Setup adc0 for potentiometer.
    P1DIR &= ~POT;
    P1SEL |= POT; // Set pin to analog.
    ADC10AE0 = POT; // Select channel A0.
    ADC10CTL1 = INCH_0 + ADC10DIV_3; // Select Channel A0, ADC10CLK/3
    ADC10CTL0 = ADC10SHT_3 + MSC + ADC10ON;

    unsigned int ADC_value; // Holds the adc value for A0.

    while(1) {
        ADC_value = readAnalog();
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


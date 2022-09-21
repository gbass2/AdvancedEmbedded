#include <msp430.h> 
#include <stdlib.h>

/************************************************************************************
 * main.c
 * 09/22/2022
 * Grayson Bass, Sam Xu
 * ECGR 5101 Lab 03
 * Displays 0-1023 on 4 7-segment display based on the input value from a potentiometer.
 ************************************************************************************/

// Define the pin numbers 0-7 to segments of the display from segments a-h.
// h being the dot on the display.
#define LED_a 0x01
#define LED_b 0x02
#define LED_c 0x04
#define LED_d 0x08
#define LED_e 0x10
#define LED_f 0x20
#define LED_g 0x40
#define LED_h 0x80

// Define the hex values needed to display each digit or character on the 7-segment.
// 7-Segment display used is common anode so the bits are inverted.
#define SEG_0 0x40
#define SEG_1 0xF9
#define SEG_2 0x24
#define SEG_3 0x30
#define SEG_4 0x19
#define SEG_5 0x12
#define SEG_6 0x02
#define SEG_7 0x78
#define SEG_8 0x00
#define SEG_9 0x10

// Function prototypes.
void setupADC(); // Setup the adc pin.
unsigned int readAnalog(); // Returns a 10-bit adc value.
void parseADC(unsigned int, unsigned short*); // Splits the adc value into 4 separate integers based on place-value.
unsigned short display7Seg(unsigned int, unsigned short); // Drives the pins to display a passed in int (0-9) to a 7-segment display.
unsigned int prevValue = 0;

// Define 1.0 to be the pin connected to the potentiometer.
#define POT 0x01

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    // Setup 7-segment pins.
    // Set all port 2 pins to outputs.
    P2DIR |= 0xFF;
    P2SEL &= ~BIT6;
    P2OUT |= 0xFF;

    // Setup 4 pins for selecting the segments to use.
    // Set pins 1.1-1.4 to output and the rest as input.
    P1DIR |= 0x1E;

    // Setup adc pin.
    setupADC();

    unsigned short splitValue[4]; // Holds each place-value of ADCValue in a separate integer.
    unsigned int ADCValue = 0; // Holds the digital value for A0.
    unsigned short threashold = 2; // Threashold to 

    while(1) {
        ADCValue = readAnalog(); // Get the digital value

        // If the current and previous adc value is less than or equal to 2 then don't change the value.
        if(abs(ADCValue - prevValue) < threashold) {
            ADCValue = prevValue;
        }

        if(ADCValue < threashold)
            ADCValue = 0;
        else if(ADCValue > (1023-threashold-1))
            ADCValue = 1023;

        // Splits the adc value into 4 separate integers based on place-value.
        parseADC(ADCValue, splitValue); 

        // Display the adc value on the 7-segments. Don't show leading zeros.
        if(ADCValue >  999) {
            display7Seg(splitValue[0], 0); // Display the digit/char associated with the digital value.
            __delay_cycles(1600);
            display7Seg(splitValue[1], 1); // Display the digit/char associated with the digital value.
            __delay_cycles(1600);
            display7Seg(splitValue[2], 2); // Display the digit/char associated with the digital value.
            __delay_cycles(1600);
            display7Seg(splitValue[3], 3); // Display the digit/char associated with the digital value.
        } else if(ADCValue <= 999 && ADCValue > 99) {
            display7Seg(splitValue[0], 0); // Display the digit/char associated with the digital value.
            __delay_cycles(1600);
            display7Seg(splitValue[1], 1); // Display the digit/char associated with the digital value.
            __delay_cycles(1600);
            display7Seg(splitValue[2], 2); // Display the digit/char associated with the digital value.
        } else if(ADCValue <=  99 && ADCValue >= 9) {
            display7Seg(splitValue[0], 0); // Display the digit/char associated with the digital value.
            __delay_cycles(1600);
            display7Seg(splitValue[1], 1); // Display the digit/char associated with the digital value.
        } else {
            display7Seg(splitValue[0], 0); // Display the digit/char associated with the digital value.
        }
         prevValue = ADCValue;
    }

    return 0;
}

// Setup the adc pin.
void setupADC() {

    // Setup A0 for the potentiometer.
    P1SEL |= POT; // Set pin to analog.
    ADC10AE0 = POT; // Select channel A0.
    ADC10CTL1 = INCH_0 + ADC10DIV_3; // Select Channel A0, ADC10CLK/3
    ADC10CTL0 = ADC10SHT_3 + MSC + ADC10ON;

    // Sampling and conversion start.
    ADC10CTL0 |= ENC + ADC10SC;

}

// Reads the A0 analog pin and returns the digital value.
unsigned int readAnalog() {
    unsigned short i;
    unsigned int temp = 0;

    for(i=0; i<50; i++) {
        // Sampling and conversion start.
        ADC10CTL0 |= ENC + ADC10SC;

        
        temp += ADC10MEM;
        __delay_cycles(100);
    }

    return temp/50;

}

// Displays the corresponding passed in digit 0-9 to a 7-segment display.
// Selects the 7-segment to drive based on passed select value 0-3.
 unsigned short display7Seg(unsigned int segValue, unsigned short select) {

    P2OUT |= 0xFF; // Flush the current bits.

    // If the value is out of range display nothing.
    if(segValue > 10) {
        P2OUT |= 0xFF; // Set all the leds to off.
        return 0;
    }

    if(select == 0)
        P1OUT = BIT1;
    else if(select == 1)
        P1OUT = BIT2;
    else if(select ==2)
        P1OUT = BIT3;
    else if(select == 3)
        P1OUT = BIT4;
    else
        return 0;

    // Display the values on the 7-segment display.
    if(segValue == 0)
        P2OUT &= SEG_0; // Display 0.
    else if(segValue == 1)
        P2OUT &= SEG_1; // Display 1.
    else if(segValue == 2)
        P2OUT &= SEG_2; // Display 2.
    else if(segValue == 3)
        P2OUT &= SEG_3; // Display 3.
    else if(segValue ==4 )
        P2OUT &= SEG_4; // Display 4.
    else if(segValue == 5)
        P2OUT &= SEG_5; // Display 5.
    else if(segValue == 6)
        P2OUT &= SEG_6; // Display 6.
    else if(segValue == 7)
        P2OUT &= SEG_7; // Display 7.
    else if(segValue == 8)
        P2OUT &= SEG_8; // Display 8.
    else if(segValue == 9)
        P2OUT &= SEG_9; // Display 9.

    return 0;
}

void parseADC(unsigned int ADCValue, unsigned short* splitValue){
    splitValue[0] = ADCValue % 10;
    splitValue[1] = (ADCValue / 10) % 10;
    splitValue[2] = (ADCValue/100) % 10;
    splitValue[3] = (ADCValue/1000) % 10;
}



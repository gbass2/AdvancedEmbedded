#include <msp430.h> 

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
unsigned *parseInt(unsigned int); // Splits the adc value into 4 separate integers based on place-value.
unsigned short display7Seg(unsigned int, unsigned short); // Drives the pins to display a passed in int (0-9) to a 7-segment display.

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
    unsigned short select; // Holds which 7-segement to drive.
    unsigned int ADCValue = 0; // Holds the digital value for A0.

    while(1) {
        ADCValue = readAnalog(); // Get the digital value
        display7Seg(ADCValue, select); // Display the digit/char associated with the digital value.
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

    // Sampling and conversion start.
    ADC10CTL0 |= ENC + ADC10SC;

    // Return the value held in ADC10MEM
    return ADC10MEM;
}

// Displays the corresponding passed in digital value to a 7-segment display.
// Selects the 7-segment to drive based on passed value 0-3.
unsigned short display7Seg(unsigned int ADCValue, unsigned short select) {

    // If the value is out of range display nothing.
    if(ADCValue > 1023) {
        P2OUT |= 0xFF; // Set all the leds to off.
        return 0;
    }

    P1OUT &= BIT2;

    P2OUT |= 0xFF; // Flush the current bits.

    // Display the values on the 7-segment display.
//    if(ADCValue == 0)
//        P2OUT &= SEG_0; // Display 0.
//    else if(ADCValue == 1)
//        P2OUT &= SEG_1; // Display 1.
//    else if(ADCValue == 2)
//        P2OUT &= SEG_2; // Display 2.
//    else if(ADCValue == 3)
//        P2OUT &= SEG_3; // Display 3.
//    else if(ADCValue ==4 )
//        P2OUT &= SEG_4; // Display 4.
//    else if(ADCValue == 5)
//        P2OUT &= SEG_5; // Display 5.
//    else if(ADCValue == 6)
//        P2OUT &= SEG_6; // Display 6.
//    else if(ADCValue == 7)
//        P2OUT &= SEG_7; // Display 7.
//    else if(ADCValue == 8)
//        P2OUT &= SEG_8; // Display 8.
//    else if(ADCValue == 9)
//        P2OUT &= SEG_9; // Display 9.

    // Display the values on the 7-segment display.
        // Leaves 1 integer value between each 7-segment digit to hand potential
        // oscillation between 2 digits.
        if(ADCValue < 64)
            P2OUT &= SEG_0; // Display 0.
        else if(ADCValue > 64 && ADCValue < 128)
            P2OUT &= SEG_1; // Display 1.
        else if(ADCValue > 128 && ADCValue < 192)
            P2OUT &= SEG_2; // Display 2.
        else if(ADCValue > 192 && ADCValue < 256)
            P2OUT &= SEG_3; // Display 3.
        else if(ADCValue > 256 && ADCValue < 320)
            P2OUT &= SEG_4; // Display 4.
        else if(ADCValue > 320 && ADCValue < 384)
            P2OUT &= SEG_5; // Display 5.
        else if(ADCValue > 384 && ADCValue < 448)
            P2OUT &= SEG_6; // Display 6.
        else if(ADCValue > 448 && ADCValue < 512)
            P2OUT &= SEG_7; // Display 7.
        else if(ADCValue > 512 && ADCValue < 576)
            P2OUT &= SEG_8; // Display 8.
        else if(ADCValue > 576 && ADCValue < 640)
            P2OUT &= SEG_9; // Display 9.
        else if(ADCValue > 640 && ADCValue < 704)


    return 0;
}

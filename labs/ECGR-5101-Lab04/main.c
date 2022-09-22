#include <msp430.h> 
#include <stdlib.h>

/************************************************************************************
 * main.c
 * 09/22/2022
 * Grayson Bass, Sam Xu
 * ECGR 5101 Lab 03
 * Displays 0-1023 using 4 7-segment display based on the input value from a potentiometer.
 ************************************************************************************/

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
void setupADC();                                            // Setup the adc pin connected to the potentiometer.
unsigned int readAnalog();                                  // Returns a 10-bit adc value.
unsigned short display7Seg(unsigned short, unsigned short); // Drives the pins to display a passed in int (0-9) to a 7-segment display.
void parseADC(unsigned int, unsigned short*);               // Splits the adc value into 4 separate integers based on place-value.


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
    
    unsigned int adcValue = 0;      // Holds the current adc value for A0.
    unsigned short splitValue[4];   // Holds each place-value of the adc value in separate integers.
    unsigned int prevValue = 0;     // Holds the previous adc value
    unsigned short threshold = 3;  // threshold to prevent oscillation. 

    while(1) {
        adcValue = readAnalog();    // Get the digital value

        // If the current adc value - previous adc value is less than 3 then don't update the value to be displayed.
        if(abs(adcValue - prevValue) < threshold)  {
            adcValue = prevValue;
        } 

        // If previous adc value is to be ie kept then the values on both ends of the extreme cannot be reached.
        // So, if they are close to the beginning or end then show 1023 or 0.a
        if(adcValue < threshold)
            adcValue = 0;
        else if(adcValue > (1023-threshold))
            adcValue = 1023;

        // Splits the adc value into 4 separate integers based on place-value.
        parseADC(adcValue, splitValue); 

        // Display the adc value on the 7-segments. Does not show leading zeros.
        // Delay is added to allow the first 3 segments to have a long on period.
        // The least significant display's digit is displayed first.
        if(adcValue >  999) {
            display7Seg(splitValue[0], 0); // Display the digit/char associated with the digital value.
            __delay_cycles(1600);
            display7Seg(splitValue[1], 1); // Display the digit/char associated with the digital value.
            __delay_cycles(1600);
            display7Seg(splitValue[2], 2); // Display the digit/char associated with the digital value.
            __delay_cycles(1600);
            display7Seg(splitValue[3], 3); // Display the digit/char associated with the digital value.
        } else if(adcValue <= 999 && adcValue > 99) {
            display7Seg(splitValue[0], 0); // Display the digit/char associated with the digital value.
            __delay_cycles(1600);
            display7Seg(splitValue[1], 1); // Display the digit/char associated with the digital value.
            __delay_cycles(1600);
            display7Seg(splitValue[2], 2); // Display the digit/char associated with the digital value.
        } else if(adcValue <=  99 && adcValue >= 9) {
            display7Seg(splitValue[0], 0); // Display the digit/char associated with the digital value.
            __delay_cycles(1600);
            display7Seg(splitValue[1], 1); // Display the digit/char associated with the digital value.
        } else {
            display7Seg(splitValue[0], 0); // Display the digit/char associated with the digital value.
        }

        prevValue = adcValue; // Set the previous adc value to the current.
    }

    return 0;
}

/************************************************************************************
 * Name:        SetupADC
 * Description: Sets up the adc channel and pin A0
 * Accepts:     None
 * Returns:     None
 ************************************************************************************/

void setupADC() {

    // Setup A0 for the potentiometer.
    P1SEL |= POT;                       // Set pin to analog.
    ADC10AE0 = POT;                     // Select channel A0.
    ADC10CTL1 = INCH_0 + ADC10DIV_3;    // Select Channel A0, ADC10CLK/3
    ADC10CTL0 = ADC10SHT_3 + MSC + ADC10ON;

    // Sampling and conversion start.
    ADC10CTL0 |= ENC + ADC10SC;

}

/************************************************************************************
 * Name:        readAnalog
 * Description: Samples the A0 analog pin and returns the digital value.
 * Accepts:     None
 * Returns:     An integer holding the adc value of pin A0.
 ************************************************************************************/
unsigned int readAnalog() {
    unsigned short i;
    unsigned int adcValue = 0;

    // Sample the adc value 50 times.
    for(i=0; i<50; i++) {

        // Sampling and conversion start.
        ADC10CTL0 |= ENC + ADC10SC;

        
        adcValue += ADC10MEM;
        __delay_cycles(100);
    }

    return adcValue/50; // Return the average of the values.

}

/************************************************************************************
 * Name:        display7Seg
 * Description: Displays the corresponding passed in digit 0-9 to a 7-segment display.
 *              Selects the 7-segment to drive based on passed select value 0-3.
 * Accepts:     An integer value to be displayed on the 7-segment.
 *              An integer value to select which display to drive.
 * Returns:     An integer holding the adc value of pin A0.
 ************************************************************************************/
 unsigned short display7Seg(unsigned short segValue, unsigned short select) {

    P2OUT |= 0xFF; // Flush the current bits.

    // If the value is out of range display nothing.
    if(segValue > 10) {
        P2OUT |= 0xFF; // Set all the leds to off.
        return 0;
    }

    // Set the 7-segment display the digit will be displayed
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

/************************************************************************************
 * Name:        parseADC
 * Description: Takes a integer (0-1023) and splits it into 
 *              4 integers by its place-value.
 *              
 * Accepts:     An integer (0-1023)
 *              An array of integers size 4 to store the split values.
 * Returns:     None
 ************************************************************************************/
void parseADC(unsigned int adcValue, unsigned short* splitValue){
    splitValue[0] = adcValue % 10;          // 1's place.
    splitValue[1] = (adcValue / 10) % 10;   // 10's place.
    splitValue[2] = (adcValue/100) % 10;    // 100's palce.
    splitValue[3] = (adcValue/1000) % 10;   // 1000's plce.
}



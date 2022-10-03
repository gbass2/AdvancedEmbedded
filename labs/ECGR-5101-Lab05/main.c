#include <msp430.h> 
#include <stdlib.h>

/************************************************************************************
 * Grayson Bass, Sam Xu             ** Lab 05 **              ** 10/06/2022 **
 * ECGR 5101 Advanced Embedded Lab 05
 * Equipment and Software:
 *      - MSP430G2553
 *      - LDQ-M3604RI 7-Segement display
 *      - Accelerometer
 *      - Code Composer Studio
 * Description: 
 ************************************************************************************/

// Define the hex values needed to display each digit or character on the 7-segment.
// 7-Segment display used is common anode so the bits are inverted.
#define SEG_0 0xC0
#define SEG_1 0xF9
#define SEG_2 0xA4
#define SEG_3 0xB0
#define SEG_4 0x99
#define SEG_5 0x92
#define SEG_6 0x82
#define SEG_7 0xF8
#define SEG_8 0x80
#define SEG_9 0x90
#define SEG_DOT ~BIT7

// Function prototypes.
void setupADC();                                                  // Setup the adc pin connected to the potentiometer.
unsigned int readAnalog(unsigned short);                          // Returns a 10-bit adc value.
unsigned short displayOne7Seg(unsigned char, unsigned short);     // Drives the pins to display a passed in int (0-9) to one 7-segment display.
void display7Seg(unsigned char*, unsigned int, unsigned short);   // Displays the corresponding passed in digits to all 4 7-segment display.
void parseADC(unsigned int, unsigned char*);                      // Splits the adc value into 4 separate integers based on place-value.

#define ACC_X BIT5 // Define 1.5 to be the pin connected to the accelerameter x axis.
#define ACC_Y BIT6 // Define 1.6 to be the pin connected to the accelerameter y axis.
#define ACC_Z BIT7 // Define 1.7 to be the pin connected to the accelerameter z axis.

unsigned int adc[3]; // Holds the adc values for x,y,z axis of accelerameter. Used for multiple sample and conversion.

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    // Setup 7-segment pins.
    // Set all port 2 pins to outputs.
    P2DIR |= 0xFF;
    P2SEL &= ~BIT6;
    P2SEL &= ~BIT7;
    P2OUT |= 0xFF;

    // Setup 4 pins for selecting the segments to use.
    // Set pins 1.1-1.4 to output and the rest as input.
    P1DIR |= 0x1E;
    P1OUT |= 0xFF;

     __enable_interrupt();

    unsigned int adcValue = 0;      // Holds the current adc value for A0.
    unsigned char digits[4];        // Holds each place-value of the adc value in separate chars.
    unsigned int prevValue = 0;     // Holds the previous adc value
    unsigned short threshold = 3;   // threshold to prevent oscillation. 
    unsigned short axis = 1;        // axis x=1, y=2, z=3. axis x by default.
    
    // Setup adc pins.
    setupADC();

    while(1) {
        // Read adc.
        if(axis == 1) {
            adcValue = readAnalog(5);    // Get the digital value for x axis connected to pin 1.5.
        } else if (axis == 2) { 
            adcValue = readAnalog(6);    // Get the digital value for y axis connected to pin 1.6.
        } else if (axis == 3) { 
            adcValue = readAnalog(7);    // Get the digital value for z axis connected to pin 1.7.
        }

        // If the current adc value - previous adc value is less than 2 then don't update the value to be displayed.
        if(abs(adcValue - prevValue) < threshold)  {
            adcValue = prevValue;
        } 

        // If previous adc value is to be ie kept then the values on both ends of the extreme cannot be reached.
        // So, if they are close to the beginning or end then show 1023 or 0.
        if(adcValue < threshold)
            adcValue = 0;
        else if(adcValue >= (1023-threshold))
            adcValue = 1023;

        // Splits the adc value into 4 separate integers based on place-value.
        parseADC(adcValue, digits); 

        // Display the split integers on each corresponding 7-segment dispaly.
        // Pass the adc value to check for leading zeros and not display them.
        // Pass in the decmimal point to be displayed based on axis used.
        display7Seg(digits, adcValue, axis);

        prevValue = adcValue; // Set the previous adc value to the current.
    }

    return 0;
}

/************************************************************************************
 * Function Name:                   ** SetupADC **
 * Description: Sets up the ADC channels A5-A7 for multi sample and conversion.
 * Input:       No Input
 * Return:      Void
 ************************************************************************************/

void setupADC() {

    // Setup the ADC channels.
    P1SEL |= ACC_X;                             // Set pin to analog.
    P1SEL |= ACC_Y;                             // Set pin to analog.
    P1SEL |= ACC_Z;                             // Set pin to analog.

    ADC10AE0 = ACC_Z + ACC_Y + ACC_X;           // Select A5-A7              
    ADC10CTL1 = INCH_7 + ADC10DIV_3 + CONSEQ_3; // Select highest channel A7 and select repeat-sequence-of-channels mode.
    ADC10CTL0 = ADC10SHT_3 + MSC + ADC10ON;     // Select clock speed, enable ADC10, and select longest sample and hold time.
    ADC10DTC1 = 3;                              // 3 channels for multi sample and hold.

    // Sampling and conversion start.
    ADC10CTL0 |= ENC + ADC10SC;
}

/************************************************************************************
 * Function Name:                 ** readAnalog **        
 * Description: Samples the A0 analog pin and returns the digital value.
 * Input:       Unsigned short
 * Returns:     Unsigned int
 ************************************************************************************/
unsigned int readAnalog(unsigned short select) {
    unsigned short i;
    unsigned int adcValue = 0;

    // Sample the adc value 30 times.
    for(i=0; i<20; i++) {

        // Sampling and conversion start.
        ADC10CTL0 &= ~ENC;
        while (ADC10CTL1 & ADC10BUSY);               // Wait if ADC10 core is active
        ADC10CTL0 |= ENC + ADC10SC;
        ADC10SA = (unsigned int)adc;

        // Select channel to recieve data from. z,y, or z axis.
        if(select == 5)
            adcValue += adc[2];
        else if(select == 6)
            adcValue += adc[1];
        else if(select == 7)
            adcValue += adc[0];

        __delay_cycles(50);
    }

   
    return adcValue/ 40; // Return the average of adcValue1 and adcValue2.

}

/****************************************************************************************
 * Function Name:                 ** displayOne7Seg **
 * Description: Displays the corresponding passed in digit 0-9 to one 7-segment display.
 *              Selects the 7-segment to drive based on passed select value 0-3.
 * Input:       Unsigned char segValue, unisgned short select
 *         
 * Returns:     unsigned short
 ****************************************************************************************/
 unsigned short displayOne7Seg(unsigned char segValue, unsigned short select) {
    P2OUT |= 0xFF; // Flush the current bits.

    // Set the 7-segment display the digit will be displayed to.
    if(select == 0) {
        P1OUT |= BIT1;
        P1OUT &= ~(BIT2 + BIT3 + BIT4);
    } else if(select == 1) {
        P1OUT |= BIT2;
        P1OUT &= ~(BIT1 + BIT3 + BIT4);
    } else if(select == 2) {
        P1OUT |= BIT3;
        P1OUT &= ~(BIT1 + BIT2 + BIT4);
    } else if(select == 3) {
        P1OUT |= BIT4;
        P1OUT &= ~(BIT1 + BIT2 + BIT3);
    } else {
        return 0;
    }

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
    else if(segValue == '.')
        P2OUT &= SEG_DOT; // Display dot.
    

    return 0;
}

/*****************************************************************************************
 * Function Name:                 ** display7Seg **
 * Description: Displays the corresponding passed in digits to all 4 7-segment display.
 *              Uses the adc value to determine which 7-seg has leading zeros.
 *              Displays decmimal point based on passed value 0-4. 0 being off.
 * Input:       unsigned char *digits, unsigned int adcValue, unsigned short dotDisplay.
 *         
 * Returns:     void
 *****************************************************************************************/
void display7Seg(unsigned char* digits, unsigned int adcValue, unsigned short dotDisplayed) {
    // Display the adc value on the 7-segments. Does not show leading zeros.
    // Delay is added to allow the segments to have a long on period.
    // The least significant display's digit is displayed first.
    if(adcValue >  999) {
        displayOne7Seg(digits[0], 0); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);
        displayOne7Seg(digits[1], 1); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);
        displayOne7Seg(digits[2], 2); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);
        displayOne7Seg(digits[3], 3); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);
    } else if(adcValue <= 999 && adcValue > 99) {
        displayOne7Seg(digits[0], 0); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);
        displayOne7Seg(digits[1], 1); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);
        displayOne7Seg(digits[2], 2); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);
    } else if(adcValue <=  99 && adcValue >= 9) {
        displayOne7Seg(digits[0], 0); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);
        displayOne7Seg(digits[1], 1); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);

    } else {
        displayOne7Seg(digits[0], 0); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);
    }

    // Display decimal based based on passed in segment to display on.
    switch(dotDisplayed) {
        // Left most .
        case 1:
            displayOne7Seg('.', 3);
            break;
        // Left middle  .
        case 2:
            displayOne7Seg('.', 2);
            break;
        // Right middle  .
        case 3:
            displayOne7Seg('.', 1);
            break;
        // Right most .
        case 4:
            displayOne7Seg('.', 0);
            break;
    }

}

/************************************************************************************
 * Function Name:                  ** parseADC **
 * Description: Takes a integer (0-1023) and splits it into 
 *              4 integers by its place-value.
 * Input:       unsigned int, unsigned* char
 * Returns:     Void
 ************************************************************************************/
void parseADC(unsigned int adcValue, unsigned char* digits){
    digits[0] = adcValue % 10;          // 1's place.
    digits[1] = (adcValue / 10) % 10;   // 10's place.
    digits[2] = (adcValue/100) % 10;    // 100's palce.
    digits[3] = (adcValue/1000) % 10;   // 1000's plce.
}
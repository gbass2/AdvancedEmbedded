#include <msp430.h>

/************************************************************************************
 * Grayson Bass, Sam Xu             ** Lab 06 **              ** 10/20/2022 **
 * ECGR 5101 Advanced Embedded Lab 06
 * Equipment and Software:
 *      - MSP430G2 Launcpad
 *      - (2) MSP430G2452
 *      - LDQ-M3604RI 7-Segement display
 *      - Potentiometer
 *      - Code Composer Studio
 * Description:
 *      Displays 0-1023 raw ADC value using 4 7-segment display based on the
 *      input value from a potentiometer. µ-controller 0 recieves the adc input and
 *      sends it to µ-controller 1 to be displayed.
 * Notes:
 ************************************************************************************/

// Define the hex values needed to display each digit or character on the 7-segment.
// 7-Segment display used is common anode so the bits are inverted.
#define SEG_0_P1 0x27
#define SEG_0_P2 0xF6
#define SEG_1_P2 0xF6
#define SEG_2_P1 0x67
#define SEG_2_P2 0xF5
#define SEG_3_P1 0x6F
#define SEG_3_P2 0xF4
#define SEG_4_P1 0xBF
#define SEG_4_P2 0xF4
#define SEG_5_P1 0x2F
#define SEG_5_P2 0xFC
#define SEG_6_P1 0x27
#define SEG_6_P2 0xFC
#define SEG_7_P1 0x7F
#define SEG_7_P2 0xF6
#define SEG_8_P1 0x27
#define SEG_8_P2 0xF4
#define SEG_9_P1 0x2F
#define SEG_9_P2 0xF4
#define SEG_X 0x89
#define SEG_Y 0x91
#define SEG_Z 0xA4
#define SEG_DOT ~BIT7
#define SEG_DASH ~BIT6

// Define 1.0 to be the pin connected to the potentiometer.
#define POT BIT3

// Define 2.6 as the hardware flag to determine which µ controller to use.
#define HFLAG BIT0

// UART Pins
#define TXD BIT2
#define RXD BIT1

// Define the state machine.
// Each µ-controller will have 2 states
enum states {ReadADC, SendUART, RecieveUART, DisplayValue};
enum states state;

unsigned char digits[5];       // Holds each place-value of the adc value in separate chars.
 
// Function prototypes.
void setupADC();                                                      // Setup the adc pin connected to the potentiometer.
unsigned int readAnalog();                                            // Returns a 10-bit adc value.
unsigned short displayOne7Seg(unsigned char, unsigned short);         // Drives the pins to display a passed in int (0-9) to one 7-segment display.
void displayRaw7Seg(unsigned char*, unsigned short);                  // Displays the corresponding passed in digits to all 4 7-segment display.
void parseADC(unsigned int, unsigned char*);                          // Splits the adc value into 4 separate integers based on place-value.
void setupPins();                                                     // Sets the digital input and output pins for P1 and P2 for µ-controller 1.
void setupUART();                                                     // Setup uart for both µ-controllers.
unsigned int handleOSC(unsigned int, unsigned int);                   // Handles the oscillation of the digits.
unsigned short chipSelect();                                          // Return the state of which microcontroller is used.

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;       // stop watchdog timer

    // Setup local variables.
    unsigned int adcValue = 0;      // Holds the current adc value for A0.
    unsigned int prevValue = 0;     // Holds the previous adc value
    unsigned short chip = 0;        // Define which µ-controller is selected.

    // Determine which µ-controller is running.
    chip = chipSelect();

    // Setup UART.
    setupUART();

    // Enable interrupts.
    __enable_interrupt();   

    // If µ-controller 0, run the following:
    if(chip == 0) {
        // Setup adc pins.
        setupADC();

        // state = SendUART;
        state = ReadADC;

        while(1) {
            
            if (state == ReadADC) {
                 // Get the digital value for the potentiometer
                 adcValue = readAnalog();     

                // Prepare ADC value by handling oscillation.
                adcValue = handleOSC(adcValue, prevValue);

                 // Splits the adc value into 4 separate integers based on place-value.
                 parseADC(adcValue, digits);
                 
                // Set the previous adc value to the current.
                prevValue = adcValue;
                state = SendUART;
            }

            if (state == SendUART) {                
                // Send UART.
                IE2 |= UCA0TXIE;                          // Enable the Transmit interrupt
                state = ReadADC;
            }
        }
    }

    // // If µ-controller 1, run the following:
    if (chip == 1) {

        // Setup digital pins
        setupPins();

        state = RecieveUART;
        while(1) {

            if(state == RecieveUART) {
                // Get the 4 chars to be displayed.
                IE2 |= UCA0RXIE;                          // Enable the Receive interrupt.
                state = DisplayValue;
            } 
            
            if(state == DisplayValue) {
                // Display the split integers on each corresponding 7-segment dispaly.
                displayRaw7Seg(digits, 0);
                
                state = RecieveUART;
            }
        }
    }

    return 0;
}

/************************************************************************************
 * Function Name:                   ** chipSelect **
 * Description: Define the microcontroller for the code to run.
 * Input:       No Input
 * Return:      unsigned short
 ************************************************************************************/
unsigned short chipSelect(){    
    // Setup pin 1.5 as chip select pin
    P1REN |= HFLAG; // Enable pullup/pulldown resistors for P2.6
    P1OUT |= HFLAG; // Set P2.6 to have pull up resistors
    P1IE |= HFLAG;  // Enable interrupt on P2.6
    P1IES &= ~HFLAG; // Set interrupt flag on the rising edge of logic level on P2.6

    unsigned short chip = (P1IN & 0x01);

    return chip;
}

/************************************************************************************
 * Function Name:                   ** SetupADC **
 * Description: Sets up the adc channel and pin A3.
 * Input:       No Input
 * Return:      Void
 ************************************************************************************/

void setupADC() {

    // Setup A0 for the potentiometer.
    P1SEL |= POT;                       // Set pin to analog.
    ADC10AE0 = POT;                     // Select channel A33
    ADC10CTL1 = INCH_3 + ADC10DIV_3;    // Select Channel A3, ADC10CLK/3
    ADC10CTL0 = ADC10SHT_3 + MSC + ADC10ON;

    // Sampling and conversion start.
    ADC10CTL0 |= ENC + ADC10SC;
}


/************************************************************************************
 * Function Name:                 ** readAnalog **
 * Description: Samples the A5 analog pin and returns the digital value.
 * Input:       No Input
 * Returns:     Unsigned int
 ************************************************************************************/
unsigned int readAnalog() {
    unsigned int adcValue = 0;

    // Sampling and conversion start.
    ADC10CTL0 &= ~ENC;
    while (ADC10CTL1 & ADC10BUSY);          // Wait if ADC10 core is active

    // Sampling and conversion start.
    ADC10CTL0 |= ENC + ADC10SC;

    __delay_cycles(10);

    // Select channel to recieve data from ADC10MEM.
    adcValue = ADC10MEM;

    return adcValue;

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
    P1OUT |= 0xFF; // Flush the current bits.


    // Set the 7-segment display the digit will be displayed to.
    if(select == 0) {
        P2OUT |= BIT2;
        P2OUT &= ~(BIT4 + BIT5 + BIT7);
    } else if(select == 1) {
        P2OUT |= BIT4;
        P2OUT &= ~(BIT2 + BIT5 + BIT7);
    } else if(select == 2) {
        P2OUT |= BIT5;
        P2OUT &= ~(BIT2+ BIT4 + BIT7);
    } else if(select == 3) {
        P2OUT |= BIT7;
        P2OUT &= ~(BIT2 + BIT4 + BIT5);
    } else {
        return 0;
    }

    // Display the passed digit on the 7-segment display.
    if(segValue == 0) {
        P1OUT &= SEG_0_P1; // Display 0.
        P2OUT &= SEG_0_P2; // Display 0.
    }
    else if(segValue == 1) {
        P2OUT &= SEG_1_P2; // Display 1;
    }     
    else if(segValue == 2) {
        P1OUT &= SEG_2_P1; // Display 2.
        P2OUT &= SEG_2_P2; // Display 2.
    }
    else if(segValue == 3) {
        P1OUT &= SEG_3_P1; // Display 3.
        P2OUT &= SEG_3_P2; // Display 3.
    }
    else if(segValue ==4 ) {
        P1OUT &= SEG_4_P1;    // Display 4.
        P2OUT &= SEG_4_P2;    // Display 4.
    }
    else if(segValue == 5) {
        P1OUT &= SEG_5_P1;    // Display 5.
        P2OUT &= SEG_5_P2;    // Display 5.
    }
    else if(segValue == 6) {
        P1OUT &= SEG_6_P1;    // Display 6.
        P2OUT &= SEG_6_P2;    // Display 6.
    }
    else if(segValue == 7) {
        P1OUT &= SEG_7_P1;    // Display 7.
        P2OUT &= SEG_7_P2;    // Display 7.
    }
    else if(segValue == 8) {
        P1OUT &= SEG_8_P1;    // Display 8.
        P2OUT &= SEG_8_P2;    // Display 8.
    }
    else if(segValue == 9) {
        P1OUT &= SEG_9_P1;    // Display 9.
        P2OUT &= SEG_9_P2;    // Display 9.
    }
    else if(segValue == '.')
        P2OUT &= SEG_DOT; // Display dot.
    else if(segValue == '-')
        P2OUT &= SEG_DASH; // Display dash -
    else if(segValue == 'X')
        P2OUT &= SEG_X; // Display X
    else if(segValue == 'Y')
        P2OUT &= SEG_Y; // Display Y
        else if(segValue == 'Z')
        P2OUT &= SEG_Z; // Display Z

    return 0;
}

/*****************************************************************************************
 * Function Name:                 ** displayRaw7Seg **
 * Description: Displays the corresponding passed in digits to all 4 7-segment display.
 *              Uses the adc value to determine which 7-seg has leading zeros.
 *              Displays decmimal point based on passed value 0-4. 0 being off.
 *              Raw value 0-1023.
 * Input:       unsigned char *data, unsigned int adcValue, unsigned short dotDisplay.
 *
 * Returns:     void
 *****************************************************************************************/
void displayRaw7Seg(unsigned char* data, unsigned short dotDisplayed) {
    // Display the adc value on the 7-segments. Does not show leading zeros.
    // Delay is added to allow the segments to have a long on period.
    // The least significant display's digit is displayed first.
    if(data[3] !=0) {
        displayOne7Seg(data[0], 0); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);
        displayOne7Seg(data[1], 1); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);
        displayOne7Seg(data[2], 2); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);
        displayOne7Seg(data[3], 3); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);
    } else if(data[3] == 0 && data[2] != 0) {
        displayOne7Seg(data[0], 0); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);
        displayOne7Seg(data[1], 1); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);
        displayOne7Seg(data[2], 2); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);
    } else if(data[2] == 0 && data[1] != 0) {
        displayOne7Seg(data[0], 0); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);
        displayOne7Seg(data[1], 1); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);

    } else {
        displayOne7Seg(data[0], 0); // Display the digit/char associated with the digital value.
        __delay_cycles(1600);
    }

    // Display decimal based on passed in segment to display on.
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
 * Description: Takes an integer (0-1023) and splits it into
 *              4 integers by its place-value.
 *              Stores the 4 integers in an array.
 * Input:       unsigned int adcValue, unsigned* char data
 * Returns:     Void
 ************************************************************************************/
void parseADC(unsigned int adcValue, unsigned char* data){
    data[0] = adcValue % 10;          // 1's place.
    data[1] = (adcValue / 10) % 10;   // 10's place.
    data[2] = (adcValue/100) % 10;    // 100's palce.
    data[3] = (adcValue/1000) % 10;   // 1000's plce.
    data[4] = '\r';                   // Add char to denote end of string.
}

/************************************************************************************
 * Function Name:              ** setupPins **
 * Description: Sets up the digital input and output pins for µ-controller 1.
 * Input:       No input
 * Returns:     void
 ************************************************************************************/
void setupPins() {
    // Setup 7-segment pins.
    // Set all port 2 pins to outputs except for 1.3.
    P2DIR |= 0xFF;
    P2SEL &= ~BIT6;
    P2SEL &= ~BIT7;
    P2OUT |= 0xFF;

    // Setup 4 pins for selecting the segments to use.
    // Set pins 1.4, 1.5, 1.6, 1.7 to output and the rest as input.
    P1DIR |= 0xFF;
    P1OUT |= 0xFF;
}

/************************************************************************************
 * Function Name:              ** setupUART **
 * Description: Sets up UART for both µ-controllers.
 * Input:       No input
 * Returns:     void
 ************************************************************************************/
void setupUART() {
    DCOCTL = 0;                               // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_1MHZ;                    // Set DCO
    DCOCTL = CALDCO_1MHZ;
    P1SEL |= RXD + TXD;                       // P1.1 UCA0RXD input
    P1SEL2 |= RXD + TXD;                      // P1.2 UCA0TXD output
    UCA0CTL1 |= UCSSEL_2;                     // SMCLK
    UCA0BR0 = 104;                            // 1MHz 9600
    UCA0BR1 = 0;                              // 1MHz 9600
    UCA0MCTL = UCBRS0;                        // Modulation UCBRSx = 1
    UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
}

/************************************************************************************
 * Function Name:              ** handleOSC **
 * Description: Handles the oscillation that may occur with the adc.
 * Input:       unsigned int adcValue, unsigned int prevValue
 * Returns:     unsigned int
 ************************************************************************************/
unsigned int handleOSC(unsigned int adcValue, unsigned int prevValue) {
    unsigned short threshold = 10;   // threshold to prevent oscillation.

    if (adcValue > 900)
        threshold = 15;

    // // If the current adc value - previous adc value is less than 2 then don't update the value to be displayed.
    if(abs(adcValue - prevValue) <= threshold)  {
        adcValue = prevValue;
    }

    // If previous adc value is to be ie kept then the values on both ends of the extreme cannot be reached.
    // So, if they are close to the beginning or end then show 1023 or 0.
    if(adcValue < (threshold+3))
        adcValue = 0;
    else if(adcValue >= (1023-threshold)-3)
        adcValue = 1023;

    return adcValue;
}


/************************************************************************************
 * Function Name:              ** USCI0TX_ISR **
 * Description: Sets the Tx buffer to the characters needed to send.
 * Input:       No Input
 * Returns:     void
 ************************************************************************************/
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
{
    // Send start character.
    while (!(IFG2&UCA0TXIFG));                // USCI_A0 TX buffer ready?
    UCA0TXBUF = '(';

    unsigned short i = 0;

    // Loop until the end of the array and send the characters.
    while(digits[i] != '\r') {
        while (!(IFG2&UCA0TXIFG));            // USCI_A0 TX buffer ready?
        UCA0TXBUF = digits[i];                // TX next character
        i++;
    }

    // Send stop character.
    while (!(IFG2&UCA0TXIFG));                // USCI_A0 TX buffer ready?
    UCA0TXBUF = ')';

    IE2 &= ~UCA0TXIE;          // Disable USCI_A0 TX interrupt
}

/************************************************************************************
 * Function Name:              ** USCI0RX_ISR **
 * Description: Sets an array from incoming characters over UART.
 * Input:       No Input
 * Returns:     void
 ************************************************************************************/
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
    // Check for starting character.
    while (!(IFG2&UCA0RXIFG));                  // USCI_A0 TR buffer ready?
    if(UCA0RXBUF == '(') {
        unsigned short i = 0;
        
        // start recieving the message.
        while (!(IFG2&UCA0RXIFG));
        unsigned char currChar = UCA0RXBUF;

        // Loop until stop character is found and add data to digits array.
        while(currChar != ')') {
            while (!(IFG2&UCA0RXIFG));          // USCI_A0 TR buffer ready?
            digits[i] = currChar;               // TX -> RXed character
            currChar = UCA0RXBUF;
            i++;
        }
        digits[i++] = '\r';
    }

    IE2 &= ~UCA0RXIE;                           // Disable the Recieve interrupt 
    // IE2 |= UCA0TXIE;                            // Enable the Send interrupt. Used to loop msg back. 
}



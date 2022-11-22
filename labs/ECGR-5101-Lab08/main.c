#include <msp430.h>

/************************************************************************************
 * Grayson Bass, Sam Xu             ** Lab 08 **              ** 11/22/2022 **
 * ECGR 5101 Advanced Embedded Lab 08
 * Equipment and Software:
 *      - MSP430G2 Launcpad
 *      - (2) MSP430G2452
 *      - LDQ-M3604RI 7-Segement display
 *      - HC-SR0 Ultrasonic sensor
 *      - Speaker
 *      - Code Composer Studio
 * Description:
 *      Reads distance from an ultrasonic sensor on one msp430 and sends the
 *      read distance to a second msp430 over uart to be displayed on
 *      4-7 segment displays. Uses a speaker to play sound at 5 different
 *      frequencies depending on the read distance.
 * Notes:
 *      Resource used for ultrasonic sensor:
 *      https://github.com/AlejandroEsquivel/msp430-ultrasonic-distance/tree/master
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
#define SEG_DOT ~BIT5
#define SEG_DASH ~BIT1

#define ACC_X BIT5 // P1.4 accelerameter x axis.
#define ACC_Y BIT4 // P1.5 accelerameter y axis.
#define ACC_Z BIT3 // P1.6  accelerameter z axis.
#define HFLAG BIT0 // P1.0 as the hardware flag to determine which µ controller to use.
#define FREQ_CPU 1000000L // Cpu frequency.
#define SPEAKER BIT1
#define TRIG BIT7
#define ECHO BIT1
#define SW1 BIT3
#define SW2 BIT4

// UART Pins
#define TXD BIT2
#define RXD BIT1

// Define the state machines for the µ-controllers and the buttons.
enum mainStates {ReadADC, ReadUS, SendUART, RecieveUART, DisplayUSValue, DisplayACCValue};
enum mainStates mainState;

// Holds the chars to send over uart.
unsigned char digits[5];

// Variables for measuring distance.
volatile unsigned long startTime;
volatile unsigned long endTime;
volatile unsigned long deltaTime;
volatile unsigned int oneDistance;

// Function prototypes.
unsigned short displayOne7Seg(unsigned char, unsigned short);         // Drives the pins to display a passed in int (0-9) to one 7-segment display.
void displayRaw7Seg(unsigned char*, unsigned short);                  // Displays the corresponding passed in digits to all 4 7-segment display.
void displayScaled7Seg(unsigned char*, unsigned short);               // Displays a scaled ADC value from -30 to 30 in Gs.
void parseADC(unsigned int, unsigned char*);                          // Splits the adc value into 4 separate integers based on place-value.
void setupPinsTx();                                                   // Sets the digital input and output pins for P1 and P2 for µ-controller 1.
void setupPinsRx();                                                   // Sets the digital input and output pins for P1 and P2 for µ-controller 2.
unsigned short chipSelect();                                          // Return the state of which microcontroller is used.
void measureOne();                                                    // Measure from the ultrasonic in cm.
unsigned int measure();                                               // Measure multiple ultrasonic values and take the median.
void playSound(unsigned int);                                         // Play sound at a specified frequency.
void sort(unsigned int array[], int);                                 // Helper function to sort the contents of an array.

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;       // stop watchdog timer

    // Setup local variables.
    unsigned short chip = 0;        // Define which µ-controller is selected.

    // Determine which µ-controller is running.
    chip = chipSelect();

    // Enable interrupts.
    __enable_interrupt();

    // If µ-controller 0, run the following:
    if(chip == 0) {
        // Setup local variables.
        unsigned int distance = 0;      // Holds the median distance of multiple measurements.

        // Setup digital pins and uart.
        setupPinsTx();

        mainState = ReadUS;

        while(1) {

            if (mainState == ReadUS) {
                // Measure from US.
                distance = measure();

                // Splits the distance value into 4 separate integers based on place-value.
                parseADC(distance, digits);

                mainState = SendUART;
            }

            if (mainState == SendUART) {
                // Send UART.
                IE2 |= UCA0TXIE;                          // Enable the Transmit interrupt

                mainState = ReadUS;
            }
        }
    }

    // If µ-controller 1, run the following:
    if (chip == 1) {
        // Setup digital pins and uart.
        setupPinsRx();

        mainState = RecieveUART;

        while(1) {

            if(mainState == RecieveUART) {
                // Get the 4 chars to be displayed.
                IE2 |= UCA0RXIE;                          // Enable the Receive interrupt.
                mainState = DisplayUSValue;
            }

            if(mainState == DisplayUSValue) {
                // Display the split integers on each corresponding 7-segment display.
                displayRaw7Seg(digits, 0);

                mainState = RecieveUART;
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
        P1OUT &= SEG_DOT; // Display dot.
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
        __delay_cycles(200);
        displayOne7Seg(data[1], 1); // Display the digit/char associated with the digital value.
        __delay_cycles(200);
        displayOne7Seg(data[2], 2); // Display the digit/char associated with the digital value.
        __delay_cycles(200);
        displayOne7Seg(data[3], 3); // Display the digit/char associated with the digital value.
        __delay_cycles(200);
    } else if(data[3] == 0 && data[2] != 0) {
        displayOne7Seg(data[0], 0); // Display the digit/char associated with the digital value.
        __delay_cycles(200);
        displayOne7Seg(data[1], 1); // Display the digit/char associated with the digital value.
        __delay_cycles(200);
        displayOne7Seg(data[2], 2); // Display the digit/char associated with the digital value.
        __delay_cycles(200);
    } else if(data[2] == 0 && data[1] != 0) {
        displayOne7Seg(data[0], 0); // Display the digit/char associated with the digital value.
        __delay_cycles(200);
        displayOne7Seg(data[1], 1); // Display the digit/char associated with the digital value.
        __delay_cycles(200);

    } else {
        displayOne7Seg(data[0], 0); // Display the digit/char associated with the digital value.
        __delay_cycles(200);
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
 * Function Name:              ** setupPinsTx **
 * Description: Sets up the digital input and output pins for µ-controller 1.
 * Input:       No input
 * Returns:     void
 ************************************************************************************/
void setupPinsTx() {

    // Set button and ultrasonic trigger directions.
    P2DIR |= 0x92;
    P2SEL &= ~BIT6;
    P2SEL &= ~BIT7;

    // Setup uart.
    DCOCTL = 0;                               // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_1MHZ;                    // Set DCO
    DCOCTL = CALDCO_1MHZ;
    P1SEL |= TXD;
    P1SEL2 |= TXD;                            // P1.2 UCA0TXD output
    UCA0CTL1 |= UCSSEL_2;                     // SMCLK
    UCA0BR0 = 104;                            // 1MHz 9600
    UCA0BR1 = 0;                              // 1MHz 9600
    UCA0MCTL = UCBRS0;                        // Modulation UCBRSx = 1
    UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**

    P2OUT &= ~TRIG; // Set the trigger pin to initally low.
    P1SEL |= ECHO;  // Set ECHO Pin as CCI0A (Capture Input signal).

    // Timer setup for ultrasonic.
    BCSCTL2 &= ~(DIVS_3); // SMCLK = DCO = 1MHz

    // Stop timer before modifying the configuration.
    TACTL = MC_0;
    CCTL0 |= CM_3 + SCS + CCIS_0 + CAP + CCIE;

    // Select SMCLK with no divisions, continuous mode.
    TACTL |= TASSEL_2 + MC_2 + ID_0;

    // Setup pwm for speaker
    BCSCTL1   = CALBC1_1MHZ;                    // 1MHz DCO
    DCOCTL    = CALDCO_1MHZ;                    // 1MHz DCO
    P2DIR    |= SPEAKER;                           // P2.1 output direction

    TA1CCTL1  = OUTMOD_7;                       // reset/set for PWM
    TA1CTL    = TASSEL_2 | ID_0 | MC_1 | TACLR; // SMCLK, CLK divider 1, up-mode, clear
}

/************************************************************************************
 * Function Name:              ** setupPinsRx **
 * Description: Sets up the digital input and output pins for µ-controller 2.
 * Input:       No input
 * Returns:     void
 ************************************************************************************/
void setupPinsRx() {
    // Setup 7-segment pins.
    // Set all port pins to outputs except for 1.3.
    P2DIR |= 0xFF;
    P2SEL &= ~BIT6;
    P2SEL &= ~BIT7;
    P2OUT |= 0xFF;

    P1DIR |= 0xFF;
    P1OUT |= 0xFF;

    // Setup uart.
    DCOCTL = 0;                               // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_1MHZ;                    // Set DCO
    DCOCTL = CALDCO_1MHZ;
    P1SEL |= RXD;                       // P1.1 UCA0RXD input
    P1SEL2 |= RXD;
    UCA0CTL1 |= UCSSEL_2;                     // SMCLK
    UCA0BR0 = 104;                            // 1MHz 9600
    UCA0BR1 = 0;                              // 1MHz 9600
    UCA0MCTL = UCBRS0;                        // Modulation UCBRSx = 1
    UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
}

/************************************************************************************
 * Function Name:              ** playSound **
 * Description: Plays the speaker at a specified frequency.
 * Input:       unsigned int freq
 * Returns:     void
 ************************************************************************************/
void playSound(unsigned int freq) {
    P2SEL    |= SPEAKER;                           // P2.1 to TA1.1

    int period = FREQ_CPU/freq;   // period of the frequency to be played when mcu at 1MHz.

    TA1CCR0   = period;                         // Set the period
    TA1CCR1   = period/2;                       // 50% duty cycle
}


/************************************************************************************
 * Function Name:              ** measureOne **
 * Description: Enables the trigger for measuring with ultrasonic sensor
 * Input:       No Input
 * Returns:     void
 ************************************************************************************/
void measureOne() {
    // Send ultrasonic pulse.
    // Reset timer.
    TACTL |= TACLR;

    // Enable TRIGGER.
    P2OUT |= TRIG;

    // Send pulse for 10us.
    __delay_cycles(10);

//    // Disable TRIGGER.
    P2OUT &= ~TRIG;

    // wait 10ms until next measurement.
    __delay_cycles(20000);
}

/************************************************************************************
 * Function Name:              ** measure **
 * Description: Measure multiple distances from ultrasonic sensor and return the median.
 * Input:       No Input
 * Returns:     unsigned int
 ************************************************************************************/
unsigned int measure() {

    unsigned int distance[11];
    unsigned short i;

    // Get the median distance
    for(i=0; i<11; i++) {
        measureOne();
        distance[i] = oneDistance;
    }

    sort(distance,11);

    // Play sound.
    if(distance[6] == 5)
        playSound(100);
    else if(distance[6] == 10)
        playSound(500);
    else if(distance[6] == 15)
            playSound(1000);
    else if(distance[6] == 20)
            playSound(2000);
    else if(distance[6] == 25)
            playSound(3000);
    else
        P2SEL &= ~SPEAKER;

    return distance[6];
}

/************************************************************************************
 * Function Name:              ** sort **
 * Description: Helper function to sort an array.
 * Input:       unsigned int array[], int size
 * Returns:     void
 ************************************************************************************/
void sort(unsigned int array[], int size) {
  unsigned short step;
  // loop to access each array element
  for (step = 0; step < size - 1; ++step) {
      unsigned short i;
    // loop to compare array elements
    for (i = 0; i < size - step - 1; ++i) {

      // compare two adjacent elements
      // change > to < to sort in descending order
      if (array[i] > array[i + 1]) {

        // swapping occurs if elements
        // are not in the intended order
        int temp = array[i];
        array[i] = array[i + 1];
        array[i + 1] = temp;
      }
    }
  }
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

    IE2 &= ~UCA0TXIE;                         // Disable USCI_A0 TX interrupt
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

    IE2 &= ~UCA0RXIE;                           // Disable the Receive interrupt
}

/************************************************************************************
 * Function Name:              ** TA1_ISR **
 * Description: ISR for the echo pin.
 * Input:       No Input
 * Returns:     void
 ************************************************************************************/
#pragma vector = TIMER0_A0_VECTOR
__interrupt void TA1_ISR(void) {
    switch (TAIV) {

    // Timer overflow
    case 10:

        break;

    // Otherwise Capture Interrupt
    default:

        // Read the CCI bit (ECHO signal) in CCTL0
        // If ECHO is HIGH then start counting (rising edge)

        if (CCTL0 & CCI) {
            startTime = CCR0;
        }

        // If ECHO is LOW then stop counting (falling edge)
        else {

            endTime = CCR0;
            deltaTime = endTime - startTime;
            unsigned int dist = (unsigned int)(deltaTime/58);

            // Only update the distance if it is valid.
            if (dist >= 2.0 && dist <= 400)
                oneDistance = dist;
        }
        break;
    }

    TACTL &= ~CCIFG; // reset the interrupt flag
}

#include <msp430.h>

/**
 * blink.c
 * ECGR 5101 Lab 01
 * Blinks the onboard LED with a 1 second interval.
 */

/* Number of cycles to delay based on 1MHz MCLK */
#define LED_DELAY_CYCLES 1000000

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;       // stop watchdog timer
    P1DIR |= 0x01;                  // configure P1.0 as output

    while(1)
    {
        P1OUT ^= 0x01;              // toggle P1.0

        /* Wait for LED_DELAY_CYCLES cycles */
        __delay_cycles(LED_DELAY_CYCLES);
    }
}


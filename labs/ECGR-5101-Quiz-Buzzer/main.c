#include "msp430.h"
#define FREQ_CPU 1000000L

void playSound(unsigned int freq) {
    BCSCTL1   = CALBC1_1MHZ;                    // 1MHz DCO
    DCOCTL    = CALDCO_1MHZ;                    // 1MHz DCO

    P1SEL    |= 0x04;                           // P1.2 to TA0.1
    P1DIR    |= 0x04;                           // P1.2 output direction

    TA0CCTL1  = OUTMOD_7;                       // reset/set for PWM

    int period = FREQ_CPU/freq;   // period of the frequency to be played when mcu at 1MHz.

    TA0CCR0   = period;                         // Set the period
    TA0CCR1   = period/2;                       // 50% duty cycle
    TA0CTL    = TASSEL_2 | ID_0 | MC_1 | TACLR; // SMCLK, CLK divider 1, up-mode, clear
}

void main( void ) {
    WDTCTL = WDTPW | WDTHOLD;       // stop watchdog timer

   while(1) {
       playSound(500);
       __delay_cycles(500000);
       P1SEL    &= ~0x04;                           // P1.2 to TA0.1
       __delay_cycles(500000);
   }
}

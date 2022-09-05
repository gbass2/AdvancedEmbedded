#include <msp430.h>

/**
 * main.c
 * ECGR 5101 Lab 02
 * Blink LED on gpio at a certian frequency.
 * When button is held, LED is turned off.
 * https://github.com/alfy7/MSP430-Launchpad-Examples/blob/master/12_Button_Proper_Debouncing.c
 */

/* Number of cycles to delay based on 1MHz MCLK */
#define LED_DELAY_CYCLES 500000 // 0.5 seconds.

#include <msp430.h>

int count=0,state; //Declare required variables

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer

    P1DIR&=~BIT3; //Set P1.3 as input
    P1REN|=BIT3; //Enable pullup/pulldown resistors for P1.3
    P1OUT|=BIT3; //Set P1.3 to have pull up resistors

    P1IE|=BIT3; //Enable interrupt on P1.3
    P1IES|=BIT3; //Set interrupt flag on the falling edge of logic level on P1.3

    TACCR0=1000; //Make the timer count from 0 to 10000, which will take  ~0.001 seconds
    __enable_interrupt(); //Enable maskable interrupts

    P1DIR|=BIT0; //Set P1.0 and P1.6 as output
    P1OUT&=~BIT0; //Initially turn off the LED

    P2DIR|=BIT0 //Set P2.0 as output

   while(1) //Run code forever
   {
       P2OUT^=BIT0; //Toggle LED on P2.0
       __delay_cycles(LED_DELAY_CYCLES); //Delay for a while

   }

    return 0;
}

#pragma vector=PORT1_VECTOR
__interrupt void P1_Function()
{
    count=0; //Reset count
    TACTL|=TASSEL_2+MC_1+TAIE; //Start Timer0 with SMCLK clock source, UP mode and enable overflow interrupt
    state=(P1IN&BIT3)>>3; //Save the state of the switch
    P1IE&=~BIT3; //Disable interrupt on P1.3, now the Timer will take care of Debouncing
    P1IFG&=~BIT3; // Reset Port1 interrupt flag
}

#pragma vector=TIMER0_A1_VECTOR
__interrupt void TMR0()
{
    if(TAIV==TA0IV_TAIFG)   //Check if Timer overflow caused the interrupt
                            //This would be required in projects where multiple interrupts have
                            //the same interrupt vector. Here it is only optional.
        {
            if(state==((P1IN&BIT3)>>3)) //If the state of the LED is the same
                count++; //Increment the counter variable
            else
            {
                count=0; //If not same, reset the counter variable
                state=((P1IN&BIT3)>>3); //And save the present state of the switch
            }
        if(count==10) //If the state has been consistently the same
            {
            if(state==0) //If the switch was pressed
                P1OUT^=BIT0; //Toggle the LED
            P1IE|=BIT3; //We have handled the debouncing, now we again enable interrupt on P1.3, for it to again detect switch bounce
            TACTL=0; //Stop the Timer
            TACTL|=TACLR; //Clear the Timer counter
            }

            TACTL&=~(TAIFG); //Reset the interrupt flag
        }
}




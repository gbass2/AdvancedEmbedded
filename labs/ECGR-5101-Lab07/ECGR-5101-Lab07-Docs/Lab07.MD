# Lab 07


## Description:
- Reads ADC value from a 10k-ohm potentiometer connected as an input and dipslays the results on another microcontroller over UART. The second microcontroller displays 4 characters of the read ADC value to 4-7 segment displays.
- Need to be single file
    - Read dedicated GPIO Pin output to determine the which MCU to read or display ADC value.

## MSP430G2553 ADC Intput µ<sub>1</sub> 
- Reads input from ADC and sends the result to another MSP (µ<sub>2</sub>) for display.
    - States:
        - ReadADC: 
            - Reads input from a poteniometer. Outputs 4 characters of the place values of the adc integer (0-1023).
            - Handles potential oscillation between two adc values.
        - SendUART: 
            - Sends the 4 adc characters over UART.
    
    - Wiring
        - VCC
        - GND
        - p1.0 Poteniometer
        - p1.2 UART TX
        - p1.3 Hardware flag
    
## MSP430G2553 Output µ<sub>2</sub> 
- Displays ADC Value on 4-7 segment displays recieved from another MSP (µ<sub>1</sub>).
    - States:
        - DisplayADC: 
            - Displays the recieved adc value on 4-7 segment displays.
        - RecieveUART:
            - Recieves the 4 characters to display on the 4-7 segment displays. 
            - Each display can display 0-9 and the adc value displayed on all 4 displays can range from 0-1023.
    
    - Wiring
        - VCC
        - GND
        - p1.1 UART RX
        - P1.3 Hardware flag
        - p1.4-p1.7 7-segment display selector pins.
        - p2.0-p2.7 7-semgment segment pins. (Same as previous labs)
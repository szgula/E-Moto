/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/


#include "project.h"
#include <stdio.h>

/* Switch debounce delay in milliseconds */
#define SWITCH_DEBOUNCE_UNIT   (1u)

/* Number of debounce units to count delay, before consider that switch is pressed */
#define SWITCH_DEBOUNCE_PERIOD (10u)

/* Switch state defines */
#define SWITCH_PRESSED         (0u)
#define SWITCH_RELEASED        (1u)

/* Function prototypes */
CY_ISR_PROTO(ISR_CAN);
CY_ISR(BlinkerInterruptHandler);
static uint8_t ReadSwSwitch(void);
void LightsControlles(uint8_t sw_switches);
void HornController(uint8_t sw_switches);

/* Global variable used to store switch state */
uint8 switchState = SWITCH_RELEASED;
uint8 leftBlinkerStatus = 0x0;
uint8 rightBlinkerStatus = 0x0;
uint8 warningLightsStatus = 0x0;
uint8_t isOn = 0x0;

/* Global variable used to store ISR flag */
volatile uint8 isrFlag = 0u;

#define NUMBER_OF_BLINKERS (2u)
typedef struct BLINKERS{
    uint8_t activeBlinkers[NUMBER_OF_BLINKERS];                     //list of active blinkers
    uint8_t maskOfBlinkers[NUMBER_OF_BLINKERS];                     //blinkers masks
    void (*blinkers_write)(uint8);              //pointers to writing functions
    uint8 (*blinkers_read)(void);               //pointers to reading functions
    
} BLINKERS_T;

/* Global variable used to store active blinkers */
BLINKERS_T blinkersData;

int main(void)
{
    CyDelay(2000);
    CyGlobalIntEnable;
    blinkerISR_StartEx(BlinkerInterruptHandler);
    Blinker_Timer_Start();
    CAN_Start();
    PWM_Start();
    //CyIntSetVector(CAN_ISR_NUMBER, ISR_CAN);
    
    uint8_t lightsOut = 0x0;
    isOn = 0x0;
    outputPins_Write(0);
    
    
    volatile uint8_t input_1, input_2, input_2_row, input_3, pwm_value;
    for (;;) {
        
        input_1 = (~inputPins_Read() & 0x7F) << 2;
        input_2 = ~inputPins2_Read() << 4;
        input_2_row = input_2;
        input_3 = stop_Read();
        lightsOut = 0x0;
        //lightsOut |= outputPins_headLight_INTR;
        lightsOut |= outputPins_ReadDataReg() & (outputPins_leftBlinker_INTR | outputPins_rightBlinker_INTR);
        
        if (input_1 & inputPins_nightLights_INTR) {  
            pwm_value = 255;
        }
        else if (input_1 & inputPins_headLights_INTR) {
            pwm_value = 50;
        } else {
        pwm_value = 0;
        }
        PWM_WriteCompare(pwm_value);
        
        if (input_1 & inputPins_reset_INTR) {
            warningLightsStatus = ~warningLightsStatus;
        }
        if (!warningLightsStatus) {
            if (input_2 & inputPins2_leftBlinker_INTR) {
                leftBlinkerStatus = 0x1;
                rightBlinkerStatus = 0x0;
            } else if (input_2 & inputPins2_rightBlinker_INTR) {
                rightBlinkerStatus = 0x1;
                leftBlinkerStatus = 0x0;
            } else {
                rightBlinkerStatus = 0x0;
                leftBlinkerStatus = 0x0;
                lightsOut &= ~outputPins_leftBlinker_INTR & ~outputPins_rightBlinker_INTR;
            }
        }
        
        if (stop_Read()) {}
        
        
        outputPins_Write(lightsOut);
        CAN_TX_DATA_BYTE1(CAN_TX_MAILBOX_ButtonsStatus) = input_1;
        CAN_TX_DATA_BYTE2(CAN_TX_MAILBOX_ButtonsStatus) = input_2_row;
        CAN_TX_DATA_BYTE3(CAN_TX_MAILBOX_ButtonsStatus) = input_3;
        CAN_TX_DATA_BYTE4(CAN_TX_MAILBOX_ButtonsStatus) = isOn;
        CAN_TX_DATA_BYTE5(CAN_TX_MAILBOX_ButtonsStatus) = 0u;
        CAN_TX_DATA_BYTE6(CAN_TX_MAILBOX_ButtonsStatus) = 0u;
        CAN_TX_DATA_BYTE7(CAN_TX_MAILBOX_ButtonsStatus) = 0u;
        CAN_TX_DATA_BYTE8(CAN_TX_MAILBOX_ButtonsStatus) = 0u;
        CAN_SendMsgButtonsStatus();
        
        if (isrFlag != 0u)
        {   
            /* Clear the isrFlag */
            isrFlag = 0u;
        }
        
        CyDelay(100);
        
        
    }
}



/*******************************************************************************
* Function Name: ISR_CAN
********************************************************************************
*
* Summary:
*  This ISR is executed at a Receive Message event and set the isrFlag.
*
* Parameters:
*  None.
*
* Return:
*  None.
*
*******************************************************************************/
CY_ISR(ISR_CAN)
{
    /* Clear Receive Message flag */
    CAN_INT_SR_REG.byte[1u] = CAN_RX_MESSAGE_MASK;

    /* Set the isrFlag */
    isrFlag = 1u;    

    /* Acknowledges receipt of new message */
    CAN_RX_ACK_MESSAGE(CAN_RX_MAILBOX_BUTTONS);
}

CY_ISR(BlinkerInterruptHandler)
{
	/* Read Status register in order to clear the sticky Terminal Count (TC) bit 
	 * in the status register. Note that the function is not called, but rather 
	 * the status is read directly.
	 */
   	Blinker_Timer_STATUS;
    
	/* Increment the Counter to indicate the keep track of the number of 
     * interrupts received */
    //InterruptCnt++;
    
    uint8_t mask = 0x0;
    uint8_t output = outputPins_ReadDataReg();
    uint8_t output_row = output;
    if (leftBlinkerStatus | warningLightsStatus) {
        mask |= outputPins_leftBlinker_INTR;
    }
    if (rightBlinkerStatus | warningLightsStatus) {
        mask |= outputPins_rightBlinker_INTR;
    }
    
    if(leftBlinkerStatus | rightBlinkerStatus | warningLightsStatus) {
        output ^= mask;
        outputPins_Write(output);
        isOn = ~isOn;
    } else if (output_row == output) {
      isOn = 0x0;  
    }
}



/* [] END OF FILE */

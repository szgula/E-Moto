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

/* Global variable used to store ISR flag */
volatile uint8 isrFlag = 0u;

CY_ISR(ISR_CAN);


int main(void)
{
    CyDelay(2000);
    CyGlobalIntEnable;
    CAN_Start();
    //outputPins_Write(0);
    PWM_Start();
    CyIntSetVector(CAN_ISR_NUMBER, ISR_CAN);
    CyGlobalIntEnable;

    volatile uint8_t blinkerStatusReceived = 0x0, isOn = 0, blinkersOut = 0x0, blinkerStatusReceived_2 = 0x0;
    blinkers_Write(0xFF);

    for(;;)
    {
        if (stop_Read() || ~leg_input_Read()) {
               PWM_WriteCompare(255);
        } else {
            PWM_WriteCompare(25);
        }
        
        
        if (isrFlag != 0u)
        {
            blinkerStatusReceived_2 = CAN_RX_DATA_BYTE1(CAN_RX_MAILBOX_Button_Status);
            blinkerStatusReceived = (CAN_RX_DATA_BYTE2(CAN_RX_MAILBOX_Button_Status) >> 4) & 0x3;
            isOn = CAN_RX_DATA_BYTE4(CAN_RX_MAILBOX_Button_Status);
            
            blinkersOut = (blinkerStatusReceived & 0x1) << 1;            //in 4bit out 2bit
            blinkersOut |= (blinkerStatusReceived & 0x2) >> 1;
            blinkersOut &=isOn;
            
            blinkers_Write(blinkersOut);     
            
            if (blinkerStatusReceived_2 & 0x10) {
                buzzer_Write(0xFF);
            } else {
                buzzer_Write(0x0);
            }
            
            /* Clear the isrFlag */
            isrFlag = 0u;
        }
        
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
    CAN_RX_ACK_MESSAGE(CAN_RX_MAILBOX_Button_Status);
}

/* [] END OF FILE */

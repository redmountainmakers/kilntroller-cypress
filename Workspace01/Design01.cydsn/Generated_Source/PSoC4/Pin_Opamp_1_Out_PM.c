/*******************************************************************************
* File Name: Pin_Opamp_1_Out.c  
* Version 2.20
*
* Description:
*  This file contains APIs to set up the Pins component for low power modes.
*
* Note:
*
********************************************************************************
* Copyright 2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#include "cytypes.h"
#include "Pin_Opamp_1_Out.h"

static Pin_Opamp_1_Out_BACKUP_STRUCT  Pin_Opamp_1_Out_backup = {0u, 0u, 0u};


/*******************************************************************************
* Function Name: Pin_Opamp_1_Out_Sleep
****************************************************************************//**
*
* \brief Stores the pin configuration and prepares the pin for entering chip 
*  deep-sleep/hibernate modes. This function must be called for SIO and USBIO
*  pins. It is not essential if using GPIO or GPIO_OVT pins.
*
* <b>Note</b> This function is available in PSoC 4 only.
*
* \return 
*  None 
*  
* \sideeffect
*  For SIO pins, this function configures the pin input threshold to CMOS and
*  drive level to Vddio. This is needed for SIO pins when in device 
*  deep-sleep/hibernate modes.
*
* \funcusage
*  \snippet Pin_Opamp_1_Out_SUT.c usage_Pin_Opamp_1_Out_Sleep_Wakeup
*******************************************************************************/
void Pin_Opamp_1_Out_Sleep(void)
{
    #if defined(Pin_Opamp_1_Out__PC)
        Pin_Opamp_1_Out_backup.pcState = Pin_Opamp_1_Out_PC;
    #else
        #if (CY_PSOC4_4200L)
            /* Save the regulator state and put the PHY into suspend mode */
            Pin_Opamp_1_Out_backup.usbState = Pin_Opamp_1_Out_CR1_REG;
            Pin_Opamp_1_Out_USB_POWER_REG |= Pin_Opamp_1_Out_USBIO_ENTER_SLEEP;
            Pin_Opamp_1_Out_CR1_REG &= Pin_Opamp_1_Out_USBIO_CR1_OFF;
        #endif
    #endif
    #if defined(CYIPBLOCK_m0s8ioss_VERSION) && defined(Pin_Opamp_1_Out__SIO)
        Pin_Opamp_1_Out_backup.sioState = Pin_Opamp_1_Out_SIO_REG;
        /* SIO requires unregulated output buffer and single ended input buffer */
        Pin_Opamp_1_Out_SIO_REG &= (uint32)(~Pin_Opamp_1_Out_SIO_LPM_MASK);
    #endif  
}


/*******************************************************************************
* Function Name: Pin_Opamp_1_Out_Wakeup
****************************************************************************//**
*
* \brief Restores the pin configuration that was saved during Pin_Sleep().
*
* For USBIO pins, the wakeup is only triggered for falling edge interrupts.
*
* <b>Note</b> This function is available in PSoC 4 only.
*
* \return 
*  None
*  
* \funcusage
*  Refer to Pin_Opamp_1_Out_Sleep() for an example usage.
*******************************************************************************/
void Pin_Opamp_1_Out_Wakeup(void)
{
    #if defined(Pin_Opamp_1_Out__PC)
        Pin_Opamp_1_Out_PC = Pin_Opamp_1_Out_backup.pcState;
    #else
        #if (CY_PSOC4_4200L)
            /* Restore the regulator state and come out of suspend mode */
            Pin_Opamp_1_Out_USB_POWER_REG &= Pin_Opamp_1_Out_USBIO_EXIT_SLEEP_PH1;
            Pin_Opamp_1_Out_CR1_REG = Pin_Opamp_1_Out_backup.usbState;
            Pin_Opamp_1_Out_USB_POWER_REG &= Pin_Opamp_1_Out_USBIO_EXIT_SLEEP_PH2;
        #endif
    #endif
    #if defined(CYIPBLOCK_m0s8ioss_VERSION) && defined(Pin_Opamp_1_Out__SIO)
        Pin_Opamp_1_Out_SIO_REG = Pin_Opamp_1_Out_backup.sioState;
    #endif
}


/* [] END OF FILE */

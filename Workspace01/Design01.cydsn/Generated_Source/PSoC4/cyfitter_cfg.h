/*******************************************************************************
* File Name: cyfitter_cfg.h
* 
* PSoC Creator  3.3 SP1
*
* Description:
* This file provides basic startup and mux configration settings
* This file is automatically generated by PSoC Creator.
*
********************************************************************************
* Copyright (c) 2007-2015 Cypress Semiconductor.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
********************************************************************************/

#ifndef CYFITTER_CFG_H
#define CYFITTER_CFG_H

#include "cytypes.h"

extern void cyfitter_cfg(void);

/* Analog Set/Unset methods */
extern void SetAnalogRoutingPumps(uint8 enabled);
extern void AMux_1_Set(uint8 channel);
extern void AMux_1_Unset(uint8 channel);


#endif /* CYFITTER_CFG_H */

/*[]*/
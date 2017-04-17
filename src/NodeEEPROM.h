// **********************************************************************************
// OpenNode library for OpenMiniHub IoT
// **********************************************************************************
// Copyright Martins Ierags (2017), martins.ierags@gmail.com
// http://openminihub.com/
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************
#ifndef NodeEEPROM_h
#define NodeEEPROM_h

// EEPROM variable sizes, in bytes
#define SIZE_IS_RFM69HW                                         (1)     //!< Size RFM69 radio version
#define SIZE_NETWORK_ID                                     	(1)     //!< Size parent node ID
#define SIZE_FREQUENCY                                          (1)     //!< Size parent node ID
#define SIZE_ENCRYPTKEY                                     	(16)    //!< Size parent node ID
#define SIZE_NODE_ID                                            (1)     //!< Size node ID
#define SIZE_SIGNING_REQUIREMENT_TABLE          				(32)    //!< Size signing requirement table
#define SIZE_NODE_ID_TABLE          							(32)    //!< Size node id table

#define EEPROM_START 0
#define EEPROM_IS_RFM69HW EEPROM_START
#define EEPROM_NETWORK_ID (EEPROM_IS_RFM69HW + SIZE_IS_RFM69HW)
#define EEPROM_FREQUENCY (EEPROM_NETWORK_ID + SIZE_NETWORK_ID)
#define EEPROM_ENCRYPTKEY (EEPROM_FREQUENCY + SIZE_FREQUENCY)
#define EEPROM_NODE_ID (EEPROM_ENCRYPTKEY + SIZE_ENCRYPTKEY)
#define EEPROM_SIGNING_REQUIREMENT_TABLE (EEPROM_NODE_ID + SIZE_NODE_ID)
#define EEPROM_NODE_ID_TABLE (EEPROM_SIGNING_REQUIREMENT_TABLE + SIZE_NODE_ID_TABLE)

#endif // NodeEEPROM_h
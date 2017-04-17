// **********************************************************************************
// OpenNode library for OpenMiniHub IoT
// **********************************************************************************
// Copyright Martins Ierags (2017), martins.ierags@gmail.com
// http://openminihub.com/
// **********************************************************************************
// Make your default network configuration there
// **********************************************************************************
#ifndef NodeConfig_h
#define NodeConfig_h

#define GATEWAYID     	0
#define NODEID       	5
#define FREQUENCY   	RF69_868MHZ // RF69_433MHZ RF69_868MHZ RF69_915MHZ
#define NETWORKID   	100
#define ENCRYPTKEY    	"sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!

#ifdef __AVR_ATmega1284P__
  #define LED           15 // OpenNode (Moteino) MEGAs have LEDs on D15
  #define FLASH_SS      23 // and FLASH SS on D23
#else
  #define LED           9 // OpenNode (Moteino) have LEDs on D9
  #define FLASH_SS      8 // and FLASH SS on D8
#endif

#endif //NodeConfig_h
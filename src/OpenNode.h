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
#ifndef OpenNode_h
#define OpenNode_h


#ifndef CONFIG_MAX_CONTACTS
   #define CONFIG_MAX_CONTACTS (255)
#endif

#include <RFM69.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include "OpenProtocol.h"
#include "NodeContact.h"
#include "NodeConfig.h"
#include "NodeEEPROM.h"

//PIN for include button
#define buttonPin 14
//include time out in seconds
#define includeTimeOut 30

class RFM69;
class NodeContact;
class OpenProtocol;

struct mPayload {
  unsigned char senderNode=0;
  unsigned char contactId=0;
  unsigned char messageType=0;
  bool isAck=0;
  unsigned char valueType=0;
  int rssi = 0;
  char value[59]={0};  //max_payload (61-3) +1 (string terminator)
};

typedef enum {
  P_FALSE          = 0,
  P_VALID          = 1,
  P_NONCE_RESPONSE = 2,
  P_NONCE_REQUEST  = 3,
  P_ID_REQUEST     = 4,
  P_ID_RESPONSE    = 5
} PayloadData_t;

void bprintf(const char *fmt, ... );
void blink(int DELAY_MS);
void resetNode();

#define doSign(node) (~mSign[node>>3]&(1<<node%8))
#define setSign(node) (mSign[node>>3]&=~(1<<node%8))
#define clearSign(node) (mSign[node>>3]|=(1<<node%8))

// #ifdef IS_GATEWAY
  #define isNode(node) (~mNodeIdTable[node>>3]&(1<<node%8))
  #define setNode(node) (mNodeIdTable[node>>3]&=~(1<<node%8))
  #define clearNode(node) (mNodeIdTable[node>>3]|=(1<<node%8))
// #endif

#define readConfig(__pos) eeprom_read_byte((uint8_t*)(__pos))
#define writeConfig(__pos, __val) eeprom_update_byte((uint8_t*)(__pos), (__val))
#define readConfigBlock(__buf, __pos, __length) eeprom_read_block((void*)(__buf), (void*)(__pos), (__length))
#define writeConfigBlock(__buf, __pos, __length) eeprom_update_block((void*)(__buf), (void*)(__pos), (__length))

class OpenNode
{
public:
  OpenNode(RFM69 *radio, unsigned char button = buttonPin, unsigned char gateway = 0);
  static OpenNode *node();

  void initRadio(unsigned char nodeID = 1, bool readFromEEPROM = true, bool updateConfig = true);
  void saveRadioConfig();
  void updateKey(const char* key);
  bool checkButton();
  bool requestConfig();
  unsigned char newNodeID();
  void writeNodeIdTable() { writeConfigBlock(&mNodeIdTable, EEPROM_NODE_ID_TABLE, 32); };
  void readNodeIdTable() { readConfigBlock(&mNodeIdTable, EEPROM_NODE_ID_TABLE, 32); };
  unsigned char *getNodeIdTable() { return mNodeIdTable; };

  bool send(unsigned char destination, bool signedMsg);
  bool sendPing();
  bool sendHello(const char *name, const char *version);
  bool sendAllContactReport();
  bool sendMessage(char *input, bool signedMsg = true);
  bool sendPayload(unsigned char contactId, ContactData_t contactData, bool signedMsg = true);
  void setPayload(const char* input);
  void setPayload(float input, unsigned char decimals=2);
  void setPayload(bool input);
  void setPayload(unsigned char input);
  void signPayload(const char* input);

  // PayloadData_t dumpPayload(int src_node, int dst_node, int rssi, bool ack, unsigned char* payload, int payload_size, mPayload *msg);
  PayloadData_t dumpPayload(mPayload *msg);

  unsigned long run();

  void enterIncludeMode() { initRadio(mNodeID, false, false); mIsIncludeMode = true; mIncludeTime = millis(); };
  void exitIncludeMode() { Serial.println("Exit include"); initRadio(mNodeID); mIsIncludeMode = false;};
  // bool isIncludeMode() { return mIsIncludeMode; };

  RFM69 *getRadio() { return mRadio; };
  unsigned char getGateway() { return mGateway; };
  bool addContact(NodeContact *contact);
  void presentContact(unsigned char contactId, ContactType_t contactType);
  NodeContact* getContact(unsigned int index) { return mContacts[index]; };
  unsigned int numContacts() { return mNumContacts; };
  unsigned char getNodeID() { return mNodeID; };
  unsigned char getNetworkID() { return mNetworkID; };
  unsigned char getFrequency() { return mFrequency; };
  char *getEncryptKey() { return mEncryptKey; };
  // bool sendContactReport(unsigned char contactId, ContactData_t contactData, unsigned char destination = mGateway);
  bool hasUpdate(unsigned char node);
  void enableUpdate(unsigned char node);
  void disableUpdate(unsigned char node);
  bool waitForUpdate() { return mWaitForUpdate; };
  void disableWaitForUpdate() { mWaitForUpdate = false; };

private:
  RFM69 *mRadio;
  unsigned char mGateway;
  bool mIsIncludeMode;
  bool mWaitForUpdate;
  unsigned long mIncludeTime;
  unsigned int mNumContacts;
  NodeContact *mContacts[CONFIG_MAX_CONTACTS];
  unsigned char mSign[32]; // = {255};
  char mEncryptKey[16];
  unsigned char mNodeID;
  unsigned char mFrequency;
  unsigned char mNetworkID;
  unsigned char mNodeIdTable[32];
  unsigned char mHasUpdate[32];
  unsigned int debounceDelay = 1000;    // the debounce time for button
  unsigned char mButton = 14;
};

#endif // OpenNode_h

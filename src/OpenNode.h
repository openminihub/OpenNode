// **********************************************************************************
// OpenNode library for OpenMiniHub IoT
// **********************************************************************************
// Copyright Martins Ierags (2016), martins.ierags@gmail.com
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
#ifndef OpenNone_h
#define OpenNone_h


#ifndef CONFIG_MAX_CONTACTS
   #define CONFIG_MAX_CONTACTS (255)
#endif

#include <RFM69.h>
#include "OpenProtocol.h"
#include "NodeContact.h"
#include "NodeConfig.h"

class RFM69;
class NodeContact;
class OpenProtocol;

struct mPayload {
  unsigned char senderNode=0;
  unsigned char contactId=0;
  unsigned char messageType=0;
  bool isAck=0;
  unsigned char valueType=0;
  char value[59]={0};  //max_payload (61-3) +1 (string terminator)
};

typedef enum {
  P_FALSE          = 0,
  P_VALID          = 1,
  P_NONCE_RESPONSE = 2,
  P_NONCE_REQUEST  = 3
} PayloadData_t;

void bprintf(const char *fmt, ... );
void blink(byte PIN, int DELAY_MS);
// bool dumpPayload(int src_node, int dst_node, int rssi, bool ack, char* payload, int payload_size, mPayload *msg);

#define doSign(node) (~mSign[node>>3]&(1<<node%8))
#define setSign(node) (mSign[node>>3]&=~(1<<node%8))
#define clearSign(node) (mSign[node>>3]|=(1<<node%8))
// #define clearSign(node) (mSign[node>>3]&=~(1<<node%8))
// #define setSign(node) (mSign[node>>3]|=(1<<node%8))

class OpenNode
{
public:
  OpenNode(RFM69 *radio, unsigned char gateway);

  static OpenNode *node();
  
  bool sendPing();
  bool sendHello(const char *name, const char *version);
  bool sendAllContactReport();
  bool sendMessage(char *input);
  bool sendPayload(unsigned char contactId, ContactData_t contactData);
  void setPayload(const char* input);
  void setPayload(float input, unsigned char decimals=2);
  void setPayload(bool input);
  void setPayload(unsigned char input);
  void signPayload(const char* input);

  PayloadData_t dumpPayload(int src_node, int dst_node, int rssi, bool ack, unsigned char* payload, int payload_size, mPayload *msg);


  unsigned long run();

  // void enterIncludeMode();
  // bool isIncludeMode() { return mIsIncludeMode; };

  RFM69 *getRadio() { return mRadio; };
  unsigned char getGateway() { return mGateway; };
  bool addContact(NodeContact *contact);
  void presentContact(unsigned char contactId, ContactType_t contactType);
  NodeContact* getContact(unsigned int index) { return mContacts[index]; };
  unsigned int numContacts() { return mNumContacts; };
  // bool sendContactReport(unsigned char contactId, ContactData_t contactData, unsigned char destination = mGateway);

private:
  RFM69 *mRadio;
  unsigned char mGateway;
  // bool mIsIncludeMode;
  unsigned int mNumContacts;
  NodeContact *mContacts[CONFIG_MAX_CONTACTS];
  unsigned char mSign[32] = {255};
};

#endif // OpenNone_h

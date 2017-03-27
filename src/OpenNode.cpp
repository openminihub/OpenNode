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
// #include <RFM69.h>
#include "OpenNode.h"
#include "NodeContact.h"
#include "NodeType.h"

#define PRINTF_BUF_LEN  (40)
void bprintf(const char *fmt, ... )
{ 
  char buf[PRINTF_BUF_LEN];
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, PRINTF_BUF_LEN, fmt, args);
  va_end (args);
  Serial.print(buf);
}

void blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}

static OpenNode *gOpenNode = NULL;

OpenNode::OpenNode(RFM69 *radio, unsigned char gateway)
{
  gOpenNode = this;
  mRadio = radio;
  mGateway = gateway;
  mNumContacts = 0;
  memset(&mSign,255,sizeof(mSign)); //Init binary array with "0"
}

bool OpenNode::addContact(NodeContact *contact)
{
  if (mNumContacts <= CONFIG_MAX_CONTACTS) {
    mContacts[mNumContacts++] = contact;
  }
}

OpenNode *OpenNode::node()
{
  return gOpenNode;
}

unsigned long OpenNode::run()
{
  unsigned long sleepInterval = 0xffffffff;
  for (unsigned char i=0; i<mNumContacts; i++) {
    if (mContacts[i]->isEnqueued()) {
      mContacts[i]->sendReport();
    } else {
      unsigned long interval = mContacts[i]->nextTickInterval();
      if (interval > 0 && interval < sleepInterval) {
        sleepInterval = interval;
      }
    }
  }
  for (unsigned char i=0; i<mNumContacts; i++) {
    mContacts[i]->intervalTick(sleepInterval);
  }
  return sleepInterval;
}

bool OpenNode::sendPing()
{
  OpenProtocol::buildPingPacket();
  bool success = this->getRadio()->sendWithRetry(mGateway, (const void*) OpenProtocol::packetData(), OpenProtocol::packetLength());
  this->getRadio()->sleep();
  return success;
}

bool OpenNode::sendHello(const char *name, const char *version)
{
  OpenProtocol::buildInternalPacket(I_SKETCH_NAME, name);
  bool success = this->getRadio()->sendWithRetry(mGateway, (const void*) OpenProtocol::packetData(), OpenProtocol::packetLength());
  // this->getRadio()->sleep();

  OpenProtocol::buildInternalPacket(I_SKETCH_VERSION, version);
  success = this->getRadio()->sendWithRetry(mGateway, (const void*) OpenProtocol::packetData(), OpenProtocol::packetLength());
  this->getRadio()->sleep();
  return success;
}

bool OpenNode::sendAllContactReport()
{
  for(unsigned char j=0; j<mNumContacts; j++) {
    mContacts[j]->sendReport();
  }
  return true;
}

bool OpenNode::sendPayload(unsigned char contactId, ContactData_t contactData)
{
  OpenProtocol::buildValuePacket(contactId, contactData);
  bool success = this->getRadio()->sendWithRetry(mGateway, (const void*) OpenProtocol::packetData(), OpenProtocol::packetLength());
  this->getRadio()->sleep();
  return success;
}

bool OpenNode::sendMessage(char *inputString)
{
  unsigned char targetId = OpenProtocol::buildMessagePacket(inputString);
  bool success = true;
  if (OpenProtocol::packetAck())
    success = this->getRadio()->sendWithRetry(targetId, (const void*)OpenProtocol::packetData(), OpenProtocol::packetLength());
  else
    this->getRadio()->send(targetId, (const void*)OpenProtocol::packetData(), OpenProtocol::packetLength());

  this->getRadio()->sleep();
  return success;
}

void OpenNode::setPayload(const char* input)
{
  OpenProtocol::setPayloadValue(input);
}

void OpenNode::setPayload(float input, unsigned char decimals)
{
  char str_temp[MAX_PAYLOAD_LEN-3];
  dtostrf(input, 1, decimals, str_temp);
  OpenProtocol::setPayloadValue(str_temp);
}

void OpenNode::setPayload(bool input)
{
  OpenProtocol::setPayloadValue(input ? "1":"0");
}

void OpenNode::setPayload(unsigned char input)
{
  char str_temp[4];
  itoa(input, str_temp, 10);
  OpenProtocol::setPayloadValue(str_temp);
}

void OpenNode::signPayload(const char* input)
{
  OpenProtocol::signPayload(input);
}

void OpenNode::presentContact(unsigned char contactId, ContactType_t contactType)
{
  OpenProtocol::buildPresentPacket(contactId, contactType);
  bool success = this->getRadio()->sendWithRetry(mGateway, (const void*) OpenProtocol::packetData(), OpenProtocol::packetLength());
  this->getRadio()->sleep();
  // return success;
}

PayloadData_t OpenNode::dumpPayload(int src_node, int dst_node, int rssi, bool ack, unsigned char* payload, int payload_size, mPayload *msg)
{
  // Serial.println("<-TRAFFIC NOTICED");
  if (payload_size > 2 && payload[kPacketType] < 5) {
    unsigned long validTime;
    if (payload[kContactId] == 0xff && payload[kPacketType] == C_INTERNAL && payload[kPacketSubType] == I_NONCE_REQUEST) {
      // Serial.println("<-SIGN [1]: GOT NONCE REQUEST");
      validTime = millis();
      // Serial.print("->SIGN [2]: SIGN="); Serial.println(validTime);
      OpenProtocol::buildNoncePacket(&validTime); //(const char *)nonce);  //(unsigned long *)
      this->getRadio()->send(src_node, (const void*) OpenProtocol::nonceData(), 7);
      setSign(src_node);
      return P_NONCE_REQUEST;
    }
    msg->senderNode = src_node;
    msg->contactId = payload[kContactId];
    msg->messageType = payload[kPacketType];
    msg->isAck = ack;
    msg->valueType = payload[kPacketSubType];
    for(int i=0; i<payload_size-kPacketPayload; i++) {
      msg->value[i] = payload[kPacketPayload+i];
    }

    if (payload[kPacketSubType] == I_NONCE_RESPONSE) {
      return P_NONCE_RESPONSE;
    }

    if (doSign(src_node)) {
      validTime = (unsigned long)payload[payload_size-1] << 24 | (unsigned long)payload[payload_size-2] << 16 | (unsigned long)payload[payload_size-3] << 8 | (unsigned long)payload[payload_size-4];
      // Serial.print("->MSG: SIGN="); Serial.println(validTime);
      // Serial.print("->MSG: TIME="); Serial.println(millis() - validTime);
      if ((millis() - validTime) < (RF69_TX_LIMIT_MS * 3)) { //3x retry sending time
        // Serial.println("->MSG: SIGN OK");
        msg->value[payload_size-kPacketPayload-4] = 0;
      } else {
        // Serial.println("SIGN FAILED");
        memset(&msg,0,sizeof(msg));
        return P_FALSE;
      }
      clearSign(src_node);
    } else {
      // Serial.println("<-MSG: NOT SIGNED");
      msg->value[payload_size-kPacketPayload] = 0;
    }
    return P_VALID;
  } else {
    memset(&msg,0,sizeof(msg));
  }
  return P_FALSE;
}

// bool OpenNode::sendContactReport(unsigned char contactId, ContactData_t contactData, unsigned char destination)
// {
  // OpenProtocol::buildValuePacket(contactId, contactData);
//   bool success = this->getRadio()->sendWithRetry(destination, (const void*) OpenProtocol::packetData(), OpenProtocol::packetLength());
//   this->getRadio()->sleep();
//   return success;
// }

// void OpenNode::enterIncludeMode()
// {
//   bprintf("OpenNode entering Include mode\n");
//   // TODO
// }

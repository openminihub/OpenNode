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
#include <RFM69.h>
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

static OpenNode *gOpenNode = NULL;

OpenNode::OpenNode(RFM69 *radio, unsigned char gateway)
{
  gOpenNode = this;
  mRadio = radio;
  mGateway = gateway;
  mNumContacts = 0;
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
  OpenProtocol::buildHelloPacket(I_SKETCH_NAME, name);
  bool success = this->getRadio()->sendWithRetry(mGateway, (const void*) OpenProtocol::packetData(), OpenProtocol::packetLength());
  // this->getRadio()->sleep();

  OpenProtocol::buildHelloPacket(I_SKETCH_VERSION, version);
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

bool OpenNode::sendPayload(unsigned char *contactId, ContactData_t *contactData)
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

void OpenNode::setPayload(unsigned char* input)
{
  OpenProtocol::setPayloadValue((const char*)input);
}

void OpenNode::presentContact(unsigned char contactId, ContactType_t contactType)
{
  OpenProtocol::buildPresentPacket(contactId, contactType);
  bool success = this->getRadio()->sendWithRetry(mGateway, (const void*) OpenProtocol::packetData(), OpenProtocol::packetLength());
  this->getRadio()->sleep();
  // return success;
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

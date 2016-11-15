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
#include "OpenProtocol.h"
#include "OpenNode.h"
#include "NodeContact.h"
#include <Arduino.h>

static unsigned char buf[MAX_PAYLOAD_LEN];

unsigned char* OpenProtocol::mPacketBuffer = (unsigned char*)buf;
unsigned char OpenProtocol::mPacketLength = 0;
unsigned char OpenProtocol::mPacketCounter = 0;
bool OpenProtocol::mAck = true;

OpenProtocol::OpenProtocol()
{

}

unsigned char* OpenProtocol::packetData()
{
  return OpenProtocol::mPacketBuffer;
}

unsigned int OpenProtocol::packetLength()
{
  return OpenProtocol::mPacketLength;
}

bool OpenProtocol::packetAck()
{
  return OpenProtocol::mAck;
}

// Packet assembly stuff
void OpenProtocol::setPayloadValue(const char* payload)
{
  unsigned char length = payload == NULL ? 0 : min(strlen(payload), MAX_PAYLOAD_LEN-3);
  strncpy( (char*)mPacketBuffer+kPacketPayload, payload, length);
  OpenProtocol::setPayloadSize(length);
}


void OpenProtocol::buildHelloPacket(ContactInternal_t contactInternal, const char *message)
{
  OpenProtocol::mPacketBuffer[kContactId] = 0xff; //internal contact
  OpenProtocol::mPacketBuffer[kPacketType] = C_INTERNAL;
  OpenProtocol::mPacketBuffer[kPacketSubType] = contactInternal;
  OpenProtocol::setPayloadValue(message);
}

void OpenProtocol::buildPresentPacket(unsigned char contactId, ContactType_t contactType)
{
  OpenProtocol::mPacketBuffer[kContactId] = contactId;
  OpenProtocol::mPacketBuffer[kPacketType] = C_PRESENTATION;
  OpenProtocol::mPacketBuffer[kPacketSubType] = contactType;
  OpenProtocol::mPacketLength = kPacketPayload;
}

void OpenProtocol::buildValuePacket(unsigned char *contactId, ContactData_t *contactData)
{
  OpenProtocol::mPacketBuffer[kContactId] = *contactId;
  OpenProtocol::mPacketBuffer[kPacketType] = C_SET;
  OpenProtocol::mPacketBuffer[kPacketSubType] = *contactData;
  OpenProtocol::mPacketLength = kPacketPayload;
}

void OpenProtocol::buildPingPacket()
{
  OpenProtocol::mPacketBuffer[kContactId] = 0xff; //internal contact
  OpenProtocol::mPacketBuffer[kPacketType] = C_INTERNAL;
  OpenProtocol::mPacketBuffer[kPacketSubType] = I_PING;
  OpenProtocol::mPacketLength = kPacketPayload;
}

void OpenProtocol::buildContactValuePacket(NodeContact *contact)
{
  OpenProtocol::mPacketBuffer[kContactId] = contact->id();
  OpenProtocol::mPacketBuffer[kPacketType] = C_SET;
  OpenProtocol::mPacketBuffer[kPacketSubType] = contact->data();
}

unsigned char OpenProtocol::buildMessagePacket(char *inputString)
{
  char *str, *p, *value=NULL;
  unsigned char targetId = 0;
  unsigned char bvalue[64];
  unsigned char blen = 0;
  int i = 0;
  unsigned char command = 0;
  unsigned char ack = 0;
  unsigned char sendStringSize = 0; //initialize here. Maybe payload will not present.

  // Extract message data coming on serial line
  for (str = strtok_r(inputString, ";", &p); // split using semicolon
    str && i < 6; // loop while str is not null an max 5 times
    str = strtok_r(NULL, ";", &p) // get subsequent tokens
      ) {
    switch (i) {
      case 0: // targetId (destination)
        targetId = atoi(str);
        break;
      case 1: // contactId
        OpenProtocol::mPacketBuffer[kContactId] = atoi(str);
        break;
      case 2: // Message type
        OpenProtocol::mPacketBuffer[kPacketType] = atoi(str);
        break;
      case 3: // Should we request ack from destination?
        OpenProtocol::mAck = atoi(str);
        break;
      case 4: // Payload
        value = str;
        sendStringSize = strlen(value);
        for(unsigned char j=0; j<sendStringSize; j++)
          OpenProtocol::mPacketBuffer[kPacketPayload+j] = value[j];
        unsigned char lastCharacter = sendStringSize-1;
        if (value[lastCharacter] == '\n') {
          value[lastCharacter] = 0;
          sendStringSize--;
        }
        break;
    }
    i++;
  }
  OpenProtocol::mPacketLength = kPacketPayload+sendStringSize;

  return targetId;
}
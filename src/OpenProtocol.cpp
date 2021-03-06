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
#include "OpenProtocol.h"
#include "OpenNode.h"
#include "NodeDevice.h"
#include <Arduino.h>

static unsigned char buf[MAX_PAYLOAD_LEN];
static unsigned char nbuf[7];

unsigned char* OpenProtocol::mPacketBuffer = (unsigned char*)buf;
unsigned char OpenProtocol::mPacketLength = 0;
unsigned char OpenProtocol::mPacketCounter = 0;
unsigned char* OpenProtocol::mNonceBuffer = (unsigned char*)nbuf;
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
  //TO DO: if not signed then do not subsctract 4 from maxsize
  unsigned char length = payload == NULL ? 0 : min(strlen(payload), MAX_PAYLOAD_LEN-kPacketPayload-4);
  strncpy( (char*)mPacketBuffer+kPacketPayload, payload, length);
  OpenProtocol::setPayloadSize(length);
}

void OpenProtocol::signPayload(const char* nonce)
{
  strncpy( (char*)mPacketBuffer+mPacketLength, nonce, 4);
  OpenProtocol::setPayloadSize(mPacketLength-kPacketPayload+4); //current total size - start point + nonce size
}

void OpenProtocol::buildInternalPacket(DeviceInternal_t deviceInternal, const char *message)
{
  OpenProtocol::mPacketBuffer[kDeviceId] = 0xff; //internal device
  OpenProtocol::mPacketBuffer[kPacketType] = C_INTERNAL;
  OpenProtocol::mPacketBuffer[kPacketSubType] = deviceInternal;
  if (message) {
    OpenProtocol::setPayloadValue(message);
  }
}

void OpenProtocol::buildNonceRequestPacket()
{
  OpenProtocol::mNonceBuffer[kDeviceId] = 0xff; //internal device
  OpenProtocol::mNonceBuffer[kPacketType] = C_INTERNAL;
  OpenProtocol::mNonceBuffer[kPacketSubType] = I_NONCE_REQUEST;
}

void OpenProtocol::buildNoncePacket(unsigned long *nonce)
{
  OpenProtocol::mNonceBuffer[kDeviceId] = 0xff; //internal device
  OpenProtocol::mNonceBuffer[kPacketType] = C_INTERNAL;
  OpenProtocol::mNonceBuffer[kPacketSubType] = I_NONCE_RESPONSE;
  OpenProtocol::mNonceBuffer[kPacketPayload] = *nonce & 0xff;
  *nonce >>= 8;
  OpenProtocol::mNonceBuffer[kPacketPayload+1] = *nonce & 0xff;
  *nonce >>= 8;
  OpenProtocol::mNonceBuffer[kPacketPayload+2] = *nonce & 0xff;
  *nonce >>= 8;
  OpenProtocol::mNonceBuffer[kPacketPayload+3] = *nonce;
  // Serial.print("NONCE: ");
  // Serial.print((unsigned char)OpenProtocol::mNonceBuffer[kPacketPayload]); Serial.print(" ");
  // Serial.print((unsigned char)OpenProtocol::mNonceBuffer[kPacketPayload+1]); Serial.print(" ");
  // Serial.print((unsigned char)OpenProtocol::mNonceBuffer[kPacketPayload+2]); Serial.print(" ");
  // Serial.println((unsigned char)OpenProtocol::mNonceBuffer[kPacketPayload+3]);
}

void OpenProtocol::buildPresentPacket(unsigned char deviceId, DeviceType_t deviceType)
{
  OpenProtocol::mPacketBuffer[kDeviceId] = deviceId;
  OpenProtocol::mPacketBuffer[kPacketType] = C_PRESENTATION;
  OpenProtocol::mPacketBuffer[kPacketSubType] = deviceType;
  OpenProtocol::mPacketLength = kPacketPayload;
}

void OpenProtocol::buildValuePacket(unsigned char deviceId, DeviceData_t deviceData)
{
  OpenProtocol::mPacketBuffer[kDeviceId] = deviceId;
  OpenProtocol::mPacketBuffer[kPacketType] = C_SET;
  OpenProtocol::mPacketBuffer[kPacketSubType] = deviceData;
  // OpenProtocol::mPacketLength = kPacketPayload;
}

void OpenProtocol::buildPingPacket()
{
  OpenProtocol::mPacketBuffer[kDeviceId] = 0xff; //internal device
  OpenProtocol::mPacketBuffer[kPacketType] = C_INTERNAL;
  OpenProtocol::mPacketBuffer[kPacketSubType] = I_PING;
  OpenProtocol::mPacketLength = kPacketPayload;
}

void OpenProtocol::buildDeviceValuePacket(NodeDevice *device)
{
  OpenProtocol::mPacketBuffer[kDeviceId] = device->id();
  OpenProtocol::mPacketBuffer[kPacketType] = C_SET;
  OpenProtocol::mPacketBuffer[kPacketSubType] = device->data();
}

// #ifdef IS_GATEWAY
void OpenProtocol::buildIdPacket(OpenNode *node)
{
  OpenProtocol::mPacketBuffer[kDeviceId] = 0xff; //internal device
  OpenProtocol::mPacketBuffer[kPacketType] = C_INTERNAL;
  OpenProtocol::mPacketBuffer[kPacketSubType] = I_ID_RESPONSE;
  OpenProtocol::mPacketBuffer[kPacketPayload] = node->getNetworkID();
  OpenProtocol::mPacketBuffer[kPacketPayload+1] = node->getFrequency();
  OpenProtocol::mPacketBuffer[kPacketPayload+2] = node->newNodeID();
  char *encryptKey = node->getEncryptKey();
  for(unsigned char j=0; j<16; j++) {
    OpenProtocol::mPacketBuffer[kPacketPayload+3+j] = encryptKey[j];
  }
  OpenProtocol::mPacketLength = kPacketPayload+19;
}
// #endif

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
      case 1: // deviceId
        OpenProtocol::mPacketBuffer[kDeviceId] = atoi(str);
        break;
      case 2: // Message type
        OpenProtocol::mPacketBuffer[kPacketType] = atoi(str);
        break;
      case 3: // Should we request ack from destination?
        OpenProtocol::mAck = atoi(str);
        break;
      case 4: // Should we request ack from destination?
        OpenProtocol::mPacketBuffer[kPacketSubType] = atoi(str);
        break;
      case 5: // Payload
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
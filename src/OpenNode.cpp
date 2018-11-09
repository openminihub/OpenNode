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
// #include <RFM69.h>
#include "OpenNode.h"
#include "NodeDevice.h"
#include "DeviceType.h"
#include <RFM69registers.h>

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

void blink(int DELAY_MS)
{
  pinMode(LED, OUTPUT);
  digitalWrite(LED,HIGH);
  delay(DELAY_MS);
  digitalWrite(LED,LOW);
}

// Use watchdog to reset the MCU
void resetNode()
{
  wdt_enable(WDTO_15MS);
  while(1);
}

static OpenNode *gOpenNode = NULL;

OpenNode::OpenNode(RFM69 *radio, unsigned char button, unsigned char gateway)
{
  gOpenNode = this;
  mRadio = radio;
  mGateway = gateway;
  mNumDevices = 0;
  mIsIncludeMode = false;
  mWaitForUpdate = false;
  mButton = button;
  memset(&mSign,255,sizeof(mSign)); //Initialize binary array with "1"
  // memset(&mNodeIdTable,255,sizeof(mNodeIdTable)); //Initialize binary array with "1"
  // writeNodeIdTable();
// #ifdef IS_GATEWAY
  readNodeIdTable();
// #endif
  memset(&mHasUpdate,255,sizeof(mHasUpdate)); //Initialize binary array with "1"
  // setxUpdate(5);
  // pinMode(buttonPin, INPUT);
  pinMode(mButton, INPUT);
}

bool OpenNode::addDevice(NodeDevice *device)
{
  if (mNumDevices <= CONFIG_MAX_MESSAGES) {
    mDevices[mNumDevices++] = device;
  }
}

OpenNode *OpenNode::node()
{
  return gOpenNode;
}

unsigned long OpenNode::run()
{
  // if (mNodeID) {  // If not a gateway...
    // if (checkButton()) {
    //   if (requestConfig()) {
    //     //if got network config then reset node
    //     blink(1000);
    //     resetNode();
    //   }
    // }
  // }
  unsigned long sleepInterval = 0xffffffff;
  for (unsigned char i=0; i<mNumDevices; i++) {
    if (mDevices[i]->isEnqueued()) {
      mDevices[i]->sendReport();
    } else {
      unsigned long interval = mDevices[i]->nextTickInterval();
      if (interval > 0 && interval < sleepInterval) {
        sleepInterval = interval;
      }
    }
  }
  for (unsigned char i=0; i<mNumDevices; i++) {
    mDevices[i]->intervalTick(sleepInterval);
  }
  return sleepInterval;
}

void OpenNode::initRadio(unsigned char nodeID, bool readFromEEPROM, bool updateConfig)
{
  unsigned char mIsRFM69HW = readConfig(EEPROM_IS_RFM69HW);
  if ((mIsRFM69HW == RF_OCP_OFF || mIsRFM69HW == RF_OCP_ON) && readFromEEPROM) {
    Serial.println("Reading NODE Config");
    mNetworkID = readConfig(EEPROM_NETWORK_ID);
    mFrequency = readConfig(EEPROM_FREQUENCY);
    mNodeID = readConfig(EEPROM_NODE_ID);
    readConfigBlock(&mEncryptKey, EEPROM_ENCRYPTKEY, 16);
  } else {
    Serial.println("Custom NODE Config");
    mNetworkID = NETWORKID;
    mFrequency = FREQUENCY;
    mNodeID = nodeID;
    updateKey(ENCRYPTKEY);
  } 
  this->getRadio()->initialize(mFrequency, mNodeID, mNetworkID);
  this->getRadio()->encrypt(mEncryptKey);
  if (mIsRFM69HW == RF_OCP_OFF) this->getRadio()->setHighPower();

  if (updateConfig  && !readFromEEPROM) {
      Serial.println("Saving NODE Config");
      saveRadioConfig();
  }
  this->getRadio()->sleep();
}

void OpenNode::updateKey(const char* key)
{
  for (uint8_t i = 0; i < 16; i++) {
    mEncryptKey[i]=key[i];
  }
}

bool OpenNode::hasUpdate(unsigned char node)
{
  if (~mHasUpdate[node>>3]&(1<<node%8)) 
    return true;
  else
    return false;
}

void OpenNode::enableUpdate(unsigned char node)
{
  mHasUpdate[node>>3]&=~(1<<node%8);
}

void OpenNode::disableUpdate(unsigned char node)
{
  mHasUpdate[node>>3]|=(1<<node%8);
}

void OpenNode::saveRadioConfig()
{
  uint8_t _isRFM69HW = this->getRadio()->readReg(REG_OCP);

  writeConfig(EEPROM_IS_RFM69HW, _isRFM69HW);
  writeConfig(EEPROM_NETWORK_ID, mNetworkID);
  writeConfig(EEPROM_FREQUENCY, mFrequency);
  writeConfig(EEPROM_NODE_ID, mNodeID);
  writeConfigBlock(&mEncryptKey, EEPROM_ENCRYPTKEY, 16);
}

bool OpenNode::checkButton()
{
  unsigned char reading = digitalRead(mButton);
  if (mButton && reading == HIGH) { //LOW
    unsigned long pressedTime = millis();
    Serial.println("* button *");
    digitalWrite(LED, HIGH);
    while ((millis() - pressedTime) < debounceDelay);
      // delay(debounceDelay);
    digitalWrite(LED, LOW);
    return true;
  }
  return false;
}

bool OpenNode::requestConfig()
{
  initRadio(mNodeID, false, false);
  bool waitMsg = true;
  unsigned char retries=3;
  for (unsigned char attempt = 0; attempt < retries; attempt++) {
    OpenProtocol::buildInternalPacket(I_ID_REQUEST, NULL);
    this->getRadio()->send(mGateway, (const void*) OpenProtocol::packetData(), OpenProtocol::packetLength());
    unsigned long now = millis();
    while (waitMsg && millis() - now < RF69_TX_LIMIT_MS) {
      if (this->getRadio()->receiveDone()) {
        mPayload msg;
        PayloadData_t received = this->dumpPayload(&msg);
        if (received == P_ID_RESPONSE) {
          if (this->getRadio()->ACKRequested())
            this->getRadio()->sendACK();
          mNetworkID = msg.value[0];
          mFrequency = msg.value[1];
          mNodeID = msg.value[2];
          for (uint8_t i = 0; i < 16; i++) 
            mEncryptKey[i]=msg.value[3+i];
          saveRadioConfig();
          Serial.println("Saving radio config");
          // initRadio(mNodeID);
          waitMsg = false;
          attempt = retries;
        } else {
          this->getRadio()->sleep();
          return false;
        }
      }
    }
  }
  this->getRadio()->sleep();
  if (waitMsg) {
    return false;
  } else {
    return true;
  }
}

// #ifdef IS_GATEWAY
unsigned char OpenNode::newNodeID()
{ 
  for (unsigned char i = 2; i < 256; i++) 
    if (!isNode(i)) return i; 

  return 1; 
}
// #endif

bool OpenNode::send(unsigned char destination, bool signedMsg)
{
  unsigned char retries=3;
  if (signedMsg) { //Request sing nonce
    bool waitMsg = true;
    // unsigned char retries=3;
    for (unsigned char attempt = 0; attempt < retries; attempt++) {
      // Serial.print("->SIGNED MSG [1]: SENDIG NONCE REQUEST ["); Serial.print(attempt+1); Serial.println("]");
      OpenProtocol::buildNonceRequestPacket();
      this->getRadio()->send(destination, (const void*) OpenProtocol::nonceData(), 3);
      unsigned long now = millis();
      while (waitMsg && millis() - now < RF69_TX_LIMIT_MS) {
        if (this->getRadio()->receiveDone()) {
          mPayload msg;
          PayloadData_t received = this->dumpPayload(&msg);
          if (received == P_NONCE_RESPONSE) {
            // Serial.println("<-SIGNED MSG [2]: MSG = NONCE RESPONSE");
            OpenProtocol::signPayload(msg.value);
            waitMsg = false;
            attempt = retries;
            // Serial.println("->SIGNED MSG [3]: MSG SENT");
          } else {
            // Serial.println("<-SIGNED MSG [2]: MSG = NOT NONCE");
            this->getRadio()->sleep();
            return false;
          }
        }
      }
    }
    if (waitMsg) {
      this->getRadio()->sleep();
      return false;
    }
  }
  bool success = this->getRadio()->sendWithRetry(destination, (const void*) OpenProtocol::packetData(), OpenProtocol::packetLength(), signedMsg ? retries : retries+2);
  //Does ACK has some message?
  if (success) {
    // noInterrupts();  // ? prevent loss of data while copying
    if (this->getRadio()->DATALEN) {
      if (this->getRadio()->DATA[0] == 'U') {
        mWaitForUpdate = true;
        Serial.println("GOT: U");
      }
    }
    // interrupts();
  }
  if (mNodeID != mGateway) {
    this->getRadio()->sleep();
    // blink(100);
  }
  return success;
}

bool OpenNode::sendPing()
{
  OpenProtocol::buildPingPacket();
  return this->send(mGateway, false);
}

bool OpenNode::sendHello(const char *name, const char *version)
{
  OpenProtocol::buildInternalPacket(I_SKETCH_NAME, name);
  bool success = this->send(mGateway, false);

  OpenProtocol::buildInternalPacket(I_SKETCH_VERSION, version);
  success = success & this->send(mGateway, false);

  return success;
}

bool OpenNode::sendInternalMessage(DeviceInternal_t deviceInternal, const char *message)
{
  OpenProtocol::buildInternalPacket(deviceInternal, message);
  return this->send(mGateway, false);
}

bool OpenNode::sendAllDeviceReport()
{
  this->setPayload("\0"); //no need to send data. We are sending only message type
  for(unsigned char j=0; j<mNumDevices; j++) {
    mDevices[j]->sendReport(0, false, false);
  }
  return true;
}

bool OpenNode::sendPayload(unsigned char deviceId, DeviceData_t deviceData, bool signedMsg)
{
  OpenProtocol::buildValuePacket(deviceId, deviceData);
  return this->send(mGateway, signedMsg);
}

bool OpenNode::sendMessage(char *inputString, bool signedMsg)
{
  unsigned char targetId = OpenProtocol::buildMessagePacket(inputString);
  return this->send(targetId, signedMsg);
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

void OpenNode::presentDevice(unsigned char deviceId, DeviceType_t deviceType)
{
  OpenProtocol::buildPresentPacket(deviceId, deviceType);
  bool success = this->send(mGateway, false);
}

// PayloadData_t OpenNode::dumpPayload(int src_node, int dst_node, int rssi, bool ack, unsigned char* payload, int payload_size, mPayload *msg)
PayloadData_t OpenNode::dumpPayload(mPayload *msg)
{
  int payload_size = this->getRadio()->DATALEN;
  unsigned char* payload = (unsigned char* )&this->getRadio()->DATA;
  // if (mIsIncludeMode) {
      // if (millis() - mIncludeTime < (includeTimeOut*1000))
        // Serial.println("THIS exit");
        // exitIncludeMode();
  // }
  // Serial.println("<-TRAFFIC NOTICED");
  if (payload_size > 2 && payload[kPacketType] < 5) {

    // noInterrupts();  // ? prevent loss of data while copying
    int src_node = this->getRadio()->SENDERID;
    // int dst_node = this->getRadio()->TARGETID;
    // bool ack = this->getRadio()->ACK_REQUESTED;

    msg->isAck = this->getRadio()->ACK_REQUESTED;
    msg->rssi = this->getRadio()->RSSI;
    // Serial.println(this->getRadio()->RSSI);

    unsigned long validTime;
    if (payload[kDeviceId] == 0xff && payload[kPacketType] == C_INTERNAL && payload[kPacketSubType] == I_NONCE_REQUEST) {
      // Serial.println("<-SIGN [1]: GOT NONCE REQUEST");
      validTime = millis();
      // Serial.print("->SIGN [2]: SIGN="); Serial.println(validTime);
      OpenProtocol::buildNoncePacket(&validTime); //(const char *)nonce);  //(unsigned long *)
      this->getRadio()->send(src_node, (const void*) OpenProtocol::nonceData(), 7);
      setSign(src_node);
// interrupts();
      return P_NONCE_REQUEST;
    }
// #ifdef IS_GATEWAY
    if (mIsIncludeMode && payload[kDeviceId] == 0xff && payload[kPacketType] == C_INTERNAL && payload[kPacketSubType] == I_ID_REQUEST) {
      Serial.println("Sending network data");
      OpenProtocol::buildIdPacket(this);
      this->getRadio()->setPowerLevel(0);
      bool success = this->getRadio()->sendWithRetry(src_node, (const void*) OpenProtocol::packetData(), OpenProtocol::packetLength());
      this->getRadio()->setPowerLevel(31);
      if (success) {
        setNode(OpenProtocol::mPacketBuffer[kPacketPayload+2]);
        writeNodeIdTable();
        exitIncludeMode();
      }
// interrupts();
      return P_ID_REQUEST;
    }
// #endif
    msg->senderNode = src_node;
    msg->deviceId = payload[kDeviceId];
    msg->messageType = payload[kPacketType];
    // msg->isAck = ack;
    msg->valueType = payload[kPacketSubType];
    for(int i=0; i<payload_size-kPacketPayload; i++) {
      msg->value[i] = payload[kPacketPayload+i];
    }

    if (payload[kPacketSubType] == I_NONCE_RESPONSE) {
      // Serial.print("["); Serial.print((unsigned char)payload_size);  Serial.print("]");
      // Serial.print((unsigned char)msg->senderNode);  Serial.print(";");
      // Serial.print((unsigned char)msg->deviceId);   Serial.print(";");
      // Serial.print((unsigned char)msg->messageType); Serial.print(";");
      // Serial.print((unsigned char)msg->isAck);       Serial.print(";");
      // Serial.print((unsigned char)msg->valueType);   Serial.print(";");
      // for(int i=0; i<payload_size-kPacketPayload; i++) {
      //   Serial.print((unsigned char)msg->value[i]); Serial.print(" ");
      // }
      // Serial.println();
// interrupts();
      return P_NONCE_RESPONSE;
    }

    if (payload[kPacketSubType] == I_ID_RESPONSE) {
// interrupts();
      return P_ID_RESPONSE;
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
// interrupts();
        return P_FALSE;
      }
      clearSign(src_node);
    } else {
      // Serial.println("<-MSG: NOT SIGNED");
      msg->value[payload_size-kPacketPayload] = 0;
    }
// interrupts();
    return P_VALID;
  } else {
    memset(&msg,0,sizeof(msg));
  }
// interrupts();
  return P_FALSE;
}

PayloadData_t OpenNode::waitForMessage(mPayload *msg)
{
  bool waitMsg = true;
  PayloadData_t pld = P_FALSE;
  unsigned long now = millis();
  while (waitMsg && ((millis() - now) < (RF69_TX_LIMIT_MS*4))) {
    if (this->getRadio()->receiveDone()) {
      pld = this->dumpPayload(msg);
      if (this->getRadio()->ACK_REQUESTED) {
        this->getRadio()->sendACK();
      }
      if (pld == P_VALID)
        waitMsg = false;
    }
  }
  return pld;

  /*
  bool waitMsg = true;
  PayloadData_t pld = P_FALSE;
  unsigned long now = millis();
  while (waitMsg && ((millis() - now) < (RF69_TX_LIMIT_MS*3))) {
    if (this->getRadio()->receiveDone()) {
      pld = this->dumpPayload(msg);
      if (this->getRadio()->ACK_REQUESTED) {
        this->getRadio()->sendACK();
      }
      waitMsg = false;
    }
  }
      // Serial.print("got pld=");
      // Serial.println(pld);
  if (pld == P_NONCE_REQUEST) {
    waitMsg = true;
    now = millis();
    // while (waitMsg && millis() - now < RF69_TX_LIMIT_MS*3) {
    while (waitMsg && ((millis() - now) < (RF69_TX_LIMIT_MS*3))) {
      if (this->getRadio()->receiveDone()) {
        pld = this->dumpPayload(msg);
        if (this->getRadio()->ACK_REQUESTED) {
          this->getRadio()->sendACK();
        }
        waitMsg = false;
      }
    }
  }
      // Serial.print("got pld=");
      // Serial.println(pld);
      // Serial.print("got msg=");
      // Serial.println(String(msg->value));
  return pld;
  */
}

// bool OpenNode::sendDeviceReport(unsigned char deviceId, DeviceData_t deviceData, unsigned char destination)
// {
  // OpenProtocol::buildValuePacket(deviceId, deviceData);
//   bool success = this->getRadio()->sendWithRetry(destination, (const void*) OpenProtocol::packetData(), OpenProtocol::packetLength());
//   this->getRadio()->sleep();
//   return success;
// }

// void OpenNode::enterIncludeMode()
// {
//   bprintf("OpenNode entering Include mode\n");
//   // TODO
// }

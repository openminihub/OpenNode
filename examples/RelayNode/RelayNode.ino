
/*
 * Copyright (c) 2018 Martins Ierags (github.com/openminihub)
 */

#include <SPIFlash.h>      //get it here: https://www.github.com/lowpowerlab/spiflash
#include <RFM69_OTA.h>

#define CONFIG_MAX_MESSAGES (1)
#include <OpenNode.h>

#define SW_NAME "RelayNode"
#define SW_VERSION "1.0"

#define RELAY_PIN 6
#define RELAY_DEVICE_ID 1

#define CLOSED 0
#define OPEN   1

SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for windbond 4mbit flash

RFM69 radio;
OpenNode node(&radio);

//define Device value getters
bool relayPosition(unsigned char id);
bool relayStatus=CLOSED;
bool relayStatusPrev=relayStatus;

//define the relay device
NodeDevice powerRelay(RELAY_DEVICE_ID, V_STATUS, relayPosition);

//received message
struct mPayload msg;

bool relayPosition(unsigned char id)
{
  node.setPayload(relayStatus);

  return true;
}

void setup()
{
  Serial.begin(115200);

  if (flash.initialize())
    Serial.println("SPI Flash Init OK!");
  else
    Serial.println("SPI Flash Init FAIL!");  

  node.initRadio(); //(6, false, true);  //(nodeid, readFromEEPROM, updateConfig)
  node.sendHello(SW_NAME, SW_VERSION);
  node.presentDevice(RELAY_DEVICE_ID, S_BINARY);

  powerRelay.setSigned(false); //use this line if you do not want to sign the message
  powerRelay.sendReport();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, relayStatus);  // Initialize relay status
  
  Serial.println("Init done");
}

void processRelay(String msg_value)
{
  relayStatusPrev = relayStatus;
  relayStatus = msg_value == "1" ? true : false;
  if (powerRelay.sendReport())
    digitalWrite( RELAY_PIN, (relayStatus) ? HIGH : LOW);
  else
    relayStatus = relayStatusPrev;
}

void processReceivedData()
{
  if (msg.messageType == C_SET)
    if (msg.valueType == V_STATUS) 
      processRelay(String(msg.value));
}

void loop()
{
    if (radio.receiveDone()) {
      CheckForWirelessHEX(radio, flash);
      PayloadData_t pld=node.dumpPayload(&msg);
      if (radio.ACK_REQUESTED) {
        radio.sendACK();
      }
      if (pld == P_VALID) {
        blink(10);
        processReceivedData();
      }
    }
    node.run();  
}
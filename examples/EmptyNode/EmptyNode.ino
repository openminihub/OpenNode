
/*
 * Copyright (c) 2018 Martins Ierags (github.com/openminihub)
 */

#include <SPIFlash.h>       //get it here: https://github.com/LowPowerLab/SPIFlash
#include <RFM69_OTA.h>      //get it here: https://www.github.com/LowPowerLab/RFM69

#define CONFIG_MAX_MESSAGES (0)
#include <OpenNode.h>

#define SW_NAME "EmptyNode"
#define SW_VERSION "1.0"

SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for windbond 4mbit flash

RFM69 radio;
OpenNode node(&radio);

struct mPayload msg;      //received message structure

void setup()
{
  Serial.begin(115200);

  if (flash.initialize())
    Serial.println("SPI Flash Init OK!");
  else
    Serial.println("SPI Flash Init FAIL!");  

  node.initRadio();
/* Uncomment only if you have HW radio chip  
  radio.setHighPower();
  node.saveRadioConfig();
*/
  node.sendHello(SW_NAME, SW_VERSION);
  Serial.println("NODE init done");
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
    }
  }
  node.run();  
}
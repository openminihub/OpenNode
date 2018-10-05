
// **********************************************************************************
// OpenNode gateway example for OpenMiniHub IoT
// **********************************************************************************
// Copyright Martins Ierags (2018), martins.ierags@gmail.com
// http://openminihub.com/
// **********************************************************************************
#include <RFM69_OTA.h>      //get it here: https://github.com/lowpowerlab/RFM69

#define CONFIG_MAX_MESSAGES (0)
#define IS_GATEWAY (TRUE)
#include <OpenNode.h>

#define SERIAL_BAUD   115200

#define SW_NAME "OpenMiniHub Gateway"
#define SW_VERSION "2.0"


RFM69 radio;
OpenNode node(&radio);
struct mPayload msg;
byte targetID=0;          //NodeID for wireless programming

void setup() {
  Serial.begin(115200);

  node.initRadio(0);  //NodeID=0 (gateway)
//  radio.setHighPower(); //must include this only for RFM69HW/HCW!

//  bprintf("\n");
//  bprintf("0;255;3;0;14;%s\n", SW_NAME);
//  bprintf("0;255;3;0;12;%s\n", SW_VERSION);
}


void processSerial()
{
  byte inputLen = 0;
  char inData[65]; // Allocate some space for the string 64+end
  byte payload[65];
  Serial.setTimeout(10);
  inputLen = Serial.readBytesUntil('\n', inData, 64);
  inData[inputLen]=0;//null-terminate it
  Serial.setTimeout(0);
  String inputstr=String(inData);
  if (inputLen > 0) {
    blink(5);
    // wireless update part
    if (inputLen==4 && inData[0]=='F' && inData[1]=='L' && inData[2]=='X' && inData[3]=='?') {
      if (targetID==0)
        Serial.println("TO?");
      else
        CheckForSerialHEX((byte*)inData, inputLen, radio, targetID, TIMEOUT, ACK_TIME, DEBUG_MODE);
    }
    else if (inputLen>3 && inputLen<=6 && inData[0]=='T' && inData[1]=='O' && inData[2]==':')
    {
      byte newTarget=0;
      for (byte i = 3; i<inputLen; i++) //up to 3 characters for target ID
        if (inData[i] >=48 && inData[i]<=57)
          newTarget = newTarget*10+inData[i]-48;
        else
        {
          newTarget=0;
          break;
        }
        if (newTarget>0)
        {
          targetID = newTarget;
          Serial.print("TO:");
          Serial.print(newTarget);
          Serial.println(":OK");
        }
        else
        {
          Serial.print(inData);
          Serial.println(":INV");
        }
    }
    
    byte colonIndex = inputstr.indexOf(";");
    if (inData[0] == '*' && inData[1] == 'i')
      node.enterIncludeMode();
    else if (inData[0] == '*' && inData[1] == 'p') {
      char encryptKey[16];
      for (uint8_t i = 0; i < 16; i++) {
        encryptKey[i]=inData[2+i];
      }
      node.updateKey(encryptKey);
      node.saveRadioConfig();
      radio.encrypt(encryptKey);
      bprintf("* password changed *\n");
    } else if (inData[0] == '*' && inData[1] == 'u') {
      bprintf("* update enabled *\n");
      byte updateNode=0;
      for (byte i = 2; i<inputLen; i++) //up to 3 characters for target ID
        if (inData[i] >=48 && inData[i]<=57)
          updateNode = updateNode*10+inData[i]-48;
      Serial.println((unsigned char)updateNode);
      node.enableUpdate(updateNode);
    } else if (colonIndex>0) {
      node.sendMessage(inData);
    }
  }
}
void outputSerial(mPayload *msg)
{
    Serial.print((unsigned char)msg->senderNode);  Serial.print(";");
    Serial.print((unsigned char)msg->deviceId);   Serial.print(";");
    Serial.print((unsigned char)msg->messageType); Serial.print(";");
    Serial.print((unsigned char)msg->isAck);       Serial.print(";");
    Serial.print((unsigned char)msg->valueType);   Serial.print(";");
    int i=0;
    while (msg->value[i] > 0)
      Serial.print(msg->value[i++]);
     Serial.print(" [RSSI:");Serial.print((int)msg->rssi); Serial.print("]");
    Serial.println();
}

void loop()
{
  if (Serial.available())
    processSerial();
  
  if (radio.receiveDone()) {
    PayloadData_t receivedMsg = node.dumpPayload(&msg);
    if (radio.ACK_REQUESTED) {
      if (node.hasUpdate(radio.SENDERID)) {
        radio.sendACK("U", 1);
        targetID = radio.SENDERID;
        Serial.print("TO:");
        Serial.print(targetID);
        Serial.println(":OK");
        node.disableUpdate(targetID);
      } else
        radio.sendACK();
    }
    if (receivedMsg == P_VALID) {
      blink(5);
      outputSerial(&msg);
    }
  }
//  node.run();
}
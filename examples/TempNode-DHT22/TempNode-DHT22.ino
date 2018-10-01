// **********************************************************************************
// OpenNode DHT22 example for OpenMiniHub IoT, Node and Contact types from mysensors.org
// **********************************************************************************
// Copyright Martins Ierags (2018), martins.ierags@gmail.com
// http://openminihub.com/
// **********************************************************************************
#include <DHT.h>  
#include <LowPower.h>       //get library from: https://github.com/lowpowerlab/lowpower
#include <SPIFlash.h>       //get it here: https://www.github.com/lowpowerlab/spiflash
#include <RFM69_OTA.h>

#define CONFIG_MAX_MESSAGES (3)
#include <OpenNode.h>

#define SW_NAME "DHT22-Light"
#define SW_VERSION "1.3"

#define HUMIDITY_SENSOR_DIGITAL_PIN 6
#define HUMIDITY_SENSOR_POWER_PIN   5
#define HUMIDITY_SENSOR_TYPE        DHT22

#define LIGHT_SENSOR_ANALOG_PIN     A1
#define LIGHT_SENSOR_POWER_PIN      A2

// Device value getters
bool temperatureValue(unsigned char id);
bool humidityValue(unsigned char id);

DHT dht(HUMIDITY_SENSOR_DIGITAL_PIN, HUMIDITY_SENSOR_TYPE);
float lastTemp;
float lastHIC;
float lastHum;

SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for windbond 4mbit flash

RFM69 radio;
OpenNode node(&radio);
NodeDevice dTemperature(1, V_TEMP, temperatureValue, k1Minute);
NodeDevice dHumidity(2, V_HUM, humidityValue, k1Minute);

void readDHT()
{
  digitalWrite(HUMIDITY_SENSOR_POWER_PIN, HIGH); // turn DHT22 sensor on
  pinMode(HUMIDITY_SENSOR_DIGITAL_PIN, INPUT);
  dht.begin();
  sleepSeconds(2);  // 0.5 Hz sampling rate (once every 2 seconds)
//  LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF);
//  LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF);
  float temp = dht.readTemperature();
  if (isnan(temp)) {
      Serial.println("Failed reading temperature from DHT");
  } else if (temp != lastTemp) {
    lastTemp = temp;
    Serial.print("T: ");
    Serial.println(temp);
  }
  
  float humidity = dht.readHumidity();
  if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT");
  } else if (humidity != lastHum) {
      lastHum = humidity;
      Serial.print("H: ");
      Serial.println(humidity);
  }

  digitalWrite(HUMIDITY_SENSOR_POWER_PIN, LOW); // turn DHT22 sensor off
  pinMode(HUMIDITY_SENSOR_DIGITAL_PIN, OUTPUT); //turn data pin as output
  digitalWrite(HUMIDITY_SENSOR_DIGITAL_PIN, LOW); // turn DHT22 data pin to low
}

bool temperatureValue(unsigned char id)
{
  node.setPayload(lastTemp);

  return true;
}

bool humidityValue(unsigned char id)
{
  node.setPayload(lastHum);

  return true;
}

void setup()
{
  pinMode(HUMIDITY_SENSOR_POWER_PIN, OUTPUT);
//  pinMode(HUMIDITY_SENSOR_DIGITAL_PIN, INPUT);
  pinMode(LIGHT_SENSOR_POWER_PIN, OUTPUT);
  pinMode(LIGHT_SENSOR_ANALOG_PIN, INPUT);

  Serial.begin(115200);
  Serial.println("Serial init done");

  if (flash.initialize())
    Serial.println("SPI Flash Init OK!");
  else
    Serial.println("SPI Flash Init FAIL!");  

  node.initRadio(); //NodeID=1, do not read config from EEPROM
//  node.initRadio(5, false); //NodeID=1, do not read config from EEPROM
  node.sendHello(SW_NAME, SW_VERSION);

//  node.sendHello(SW_NAME, SW_VERSION);

  node.presentDevice(1, S_TEMP);
  node.presentDevice(2, S_HUM);

  Serial.println( readVcc(), DEC );
}

void loop()
{
  readDHT();
  unsigned long sleepTime = node.run();
  if (node.waitForUpdate()) {
    Serial.println("Updating...");
    CheckForWirelessHEX(radio, flash);
    Serial.println("Update failed");
    node.disableWaitForUpdate();
  }

  node.setPayload(lastTemp);
  dTemperature.sendReport(99, true, false);

  sleepSeconds(sleepTime);
}

void sleepSeconds(unsigned long sleepTime)
{
  unsigned long cycleCount = sleepTime / 8;
  byte remainder = sleepTime % 10;
  for(unsigned int i=0; i<cycleCount; i++)
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  if (bitRead(remainder,2) > 0)
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
  if (bitRead(remainder,1) > 0)
    LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
  if (bitRead(remainder,0) > 0)
    LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
}

long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1125300L / result; // Back-calculate AVcc in mV
  return result;
}

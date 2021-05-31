/*
  IoTSoundSensor. Copyright (c) 2021 Pod Group Ltd. http://podgroup.com
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 3 as
  published by the Free Software Foundation
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Description:
    - Post arbitrary JSON telemetry data to Pod IoT Platform API (See below).
    - Uses Pod ENO SIM built-in TLS1.3-PSK to request HTTPS POST /v1/data/{DEVICEID}

  More information:
    - Pod IoT Platform: https://iotsim.podgroup.com/v1/docs/#/
    - Arduino Project Hub Article: https://create.arduino.cc/projecthub/kostiantynchertov/zero-touch-provisioning-based-on-tls-1-3-a07359

  Usage:
    - Open Board Manager and install "Arduino SAMD Boards (32-bits ARM Cortex-M0+).
    - Open Library Manager and install: "MKRGSM", "ArduinoUniqueID".
    - Select Board "Arduino MKR 1400 GSM".
    - Optionally configure the JSON payload (See below).

  Requirements:
    - Arduino MKR 1400 GSM board.
    - Pod ENO SIM Card (ask for yours, emails below).
    - Configuration for the SIM Card with the following format (at Pod IoT Platform):

  Authors:
   - Kostiantyn Chertov <kostiantyn.chertov@podgroup.com>
   - J. Félix Ontañón <felix.ontanon@podgroup.com>
*/

// --------- BEGINING OF CONFIGURABLE FIRMWARE PARAMETERS SECTION ---------

// The arbitrary JSON to encrypt and deliver to the cloud
// Simulated measurement from DHT-22 sensor
// {"temperature": 21.5}
#define LEN_DATA  21
const char DATA_ITEM[LEN_DATA] = { '{', '"', 't', 'e', 'm', 'p', 'e', 'r', 'a', 't', 'u', 'r', 'e', '"', ':', '2', '1 ', '.', '5', '}' }; 

// --------- END OF CONFIGURABLE FIRMWARE PARAMETERS SECTION ---------

#include <MKRGSM.h>
#include <ArduinoUniqueID.h>
#include <ArduinoLowPower.h>

#include "safe2.h"

Safe2 safe2(&SerialGSM);

#define MODEM_BAUD_RATE  9600
#define LOG_BAUD_RATE  115200

byte dataBuf[LEN_DATA];

#define LEN_DEVICE_ID_MAX  16
byte id[LEN_DEVICE_ID_MAX];

// Auxiliar function to print the device ID
// as generated by ArduinoUniqueID
void logHex(byte * buf, short len) {
  byte b;
  for (short i=0; i<len; i++) {
    b = buf[i];
    if (b < 0x10) {
      Serial.print("0");
    }
    Serial.print(b, HEX);
  }
  Serial.println();
}

void setup() {
  byte res;
  
  Serial.begin(LOG_BAUD_RATE); 
  while (!Serial) {
    ; // wait for serial port to connect
  }

  // Get Device ID
  short idLen = UniqueIDsize;
  if (idLen > LEN_DEVICE_ID_MAX) {
    idLen = LEN_DEVICE_ID_MAX;
  }
  memcpy(id, (void *)UniqueID, idLen);
  Serial.print("Device ID: ");
  logHex(id, idLen);

  // configure pins
  pinMode(GSM_RESETN, OUTPUT);

  Serial.print("Modem initialization...");
  res = safe2.init(MODEM_BAUD_RATE);
  if (res == RES_OK) {
    Serial.println("OK");
  } else {
    Serial.print("ERROR: ");
    Serial.println(res);
  }

  // reset the ublox module
  digitalWrite(GSM_RESETN, HIGH);
  delay(100);
  digitalWrite(GSM_RESETN, LOW);
  delay(300);

  Serial.print("waiting for modem start...");
  safe2.waitForModemStart();
  Serial.println("OK");

  Serial.print("waiting for network registration...");
  safe2.waitForNetworkRegistration();
  Serial.println("OK");
  
  // put Device ID
  res = safe2.deviceIdSet(id, idLen);
  if (res == RES_OK) {
    Serial.println("Set Device ID: OK");
  } else {
    Serial.println("Set Device ID: ERROR");
  }
  safe2.prepareForSleep();
}

void loop() {
  memcpy(dataBuf, DATA_ITEM, LEN_DATA);

  Serial.print("Put Data: ");
  byte res = safe2.dataSend(dataBuf, LEN_DATA);
  if (res == RES_OK) {
    Serial.println("OK");
  } else {
    Serial.print("ERROR ");
    Serial.println(res);
  }

  Serial.println("waiting");
  byte minutes = 5;
  byte secs;
  while (minutes > 0) {
    for (secs=0; secs < 60; secs++) {
      delay(1000);
    }
    minutes -= 1;
    Serial.print('.');
  }
  Serial.println();
}
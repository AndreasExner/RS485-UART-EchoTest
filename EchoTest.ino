/*

  RS485/UART echo

  Version: 1.0
  Date: 2020-12-15

  This sketch is useful for debugging and testing RS485/UART wirings. 
  Often the remote device is mounted somewhere on a rooftop or another 
  inconvenient place. That's where this tool comes in and offers you a 
  simulation for your lab.

  It was intentionally made for the wind direction sensor:
  https://github.com/AndreasExner/iobroker-IoT-WindSensor

  But it can be used for different purposes as well:

  
  

  MIT License
  
  Copyright (c) 2020 Andreas Exner
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <SoftwareSerial.h>

#define RX D6
#define TX D7
#define RTS D5
#define BAUD_RATE 9600
#define LED D4

int counter=0;

SoftwareSerial RS485;

byte windSensorFrame[] = {0x01, 0x03, 0x04, 0x00, 0x5A, 0x00, 0x00, 0x00, 0x00}; // default answer frame for the wind sensor DO NOT CHANGE
byte customFrame[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // your custom answer frame (CRC will be calculated automatically)

bool useWindSensorFrame = true; // echos a random wind direction if true, a custom frame or a copy of the incoming frame if false
bool useCustomFrame = false; // echos the custom frame if true, a copy of the incoming frame if false 


//######################################### setup ##############################################################

void setup(void) {

  Serial.begin(115200);  
  delay(1000);
  
  pinMode(LED, OUTPUT);

  pinMode(RTS, OUTPUT);
  digitalWrite(RTS, LOW);
  RS485.begin(BAUD_RATE, SWSERIAL_8N1, RX, TX, false, 128, 128);

}

//####################################################################
// Loop
  
void loop(void) {

// read from buffer

  digitalWrite(RTS, LOW);
  int readBufferSize = RS485.available();
  
  if(readBufferSize > 0){
    
    byte readBuffer[readBufferSize];
    Serial.println("RX Buffer Size: " + String(readBufferSize));
    RS485.readBytes(readBuffer, readBufferSize);
  
    Serial.print("RX Buffer : ");
      for ( byte i = 0; i < readBufferSize; i++ ) {
        Serial.print(hex_to_string(readBuffer[i]));
        Serial.print(" ");
      }
      Serial.print("\n");

    delay(10);

// CRC Check

    uint16_t CRC;
    uint8_t CRC_MSB;
    uint8_t CRC_LSB;
    String CRC_Output;
    
    CRC = Calc_CRC(readBuffer, readBufferSize);
    CRC_MSB = CRC >> 8;
    CRC_LSB = CRC;
    CRC_Output = hex_to_string(CRC_LSB) + " " + hex_to_string(CRC_MSB);

    if (readBuffer[readBufferSize -2] == CRC_LSB && readBuffer[readBufferSize -1] == CRC_MSB) {

// send echo

      if (useWindSensorFrame) { // send wind direction frame

        int echoBufferSize = sizeof(windSensorFrame);
 
        uint16_t windDir = getRandomWindDir(); // add random wind direction
        uint8_t windDir_MSB;
        uint8_t windDir_LSB;
        windDir_MSB = windDir >> 8;
        windDir_LSB = windDir;
        windSensorFrame[4] = windDir_LSB;
        windSensorFrame[3] = windDir_MSB;

        CRC = Calc_CRC(windSensorFrame, echoBufferSize);
        CRC_MSB = CRC >> 8;
        CRC_LSB = CRC;
        windSensorFrame[echoBufferSize -2] = CRC_LSB;
        windSensorFrame[echoBufferSize -1] = CRC_MSB;
  
        Serial.println("WindDir " + String(counter));
        Serial.println("TX Buffer size " + String(echoBufferSize) + " bytes");
        
        Serial.print("TX Buffer : ");
        for ( byte i = 0; i < echoBufferSize; i++ ) {
          Serial.print(hex_to_string(windSensorFrame[i]));
          Serial.print(" ");
        }
        Serial.print("\n");
  
        digitalWrite(RTS, HIGH);
        RS485.write(windSensorFrame, echoBufferSize);
        digitalWrite(RTS, LOW); 
      }
      else if (!useWindSensorFrame && useCustomFrame) {  // send custom frame
        
        int echoBufferSize = sizeof(customFrame);
      
        CRC = Calc_CRC(customFrame, echoBufferSize);
        CRC_MSB = CRC >> 8;
        CRC_LSB = CRC;
        customFrame[echoBufferSize -2] = CRC_LSB;
        customFrame[echoBufferSize -1] = CRC_MSB;
  
        Serial.println("TX Buffer size " + String(echoBufferSize) + " bytes");
        
        Serial.print("TX Buffer : ");
        for ( byte i = 0; i < echoBufferSize; i++ ) {
          Serial.print(hex_to_string(customFrame[i]));
          Serial.print(" ");
        }
        Serial.print("\n");
  
        digitalWrite(RTS, HIGH);
        RS485.write(customFrame, echoBufferSize);
        digitalWrite(RTS, LOW); 
      }
      else {  // send read buffer

        Serial.println("TX Buffer size " + String(readBufferSize) + " bytes");
        
        Serial.print("TX Buffer : ");
        for ( byte i = 0; i < readBufferSize; i++ ) {
          Serial.print(hex_to_string(readBuffer[i]));
          Serial.print(" ");
        }
        Serial.print("\n");
  
        digitalWrite(RTS, HIGH);
        RS485.write(readBuffer, readBufferSize);
        digitalWrite(RTS, LOW); 
      }

    }
    else {
      Serial.println("RX CRC ERROR");
      RS485.flush();
    }
    
    Serial.print("\n");
    
  }
  else {
    //Serial.println("No data");
  }
  
  if (counter % 2) {
    digitalWrite(LED, LOW);         
  }
  else {
    digitalWrite(LED, HIGH);
  }
  
  delay(50);
  counter++;
  if (counter > 15) {counter = 0;}
}

//####################################################################

String hex_to_string(uint8_t hex) {
  
  char hex_char[4];
  sprintf(hex_char, "%02x", hex);
  String hex_string = hex_char;
  hex_string.toUpperCase();
  return ("0x" + hex_string);
}

uint16_t Calc_CRC(byte buf[], int len) {

  // calculates the CRC for a modbus frame. Important: the last two bytes are ignored (expect CRC here)
  
  uint16_t crc = 0xFFFF;

  for (int pos = 0; pos < (len - 2); pos++) {  
    crc ^= (uint16_t)buf[pos];          // XOR byte into least sig. byte of crc

    for (int i = 8; i != 0; i--) {    // Loop over each bit
      if ((crc & 0x0001) != 0) {      // If the LSB is set
        crc >>= 1;                    // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                            // Else LSB is not set
        crc >>= 1;                    // Just shift right
    }
  }
  // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
  return crc;
}

int getRandomWindDir() {

  int windDir[] = {22,45,67,90,112,135,157,180,202,225,247,270,292,315,337,360};
  return windDir[counter];
}

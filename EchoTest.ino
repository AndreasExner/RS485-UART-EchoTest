/*

  Serial/UART echo

  Version: 1.0
  Date: 2020-12-15

  This sketch is useful for debugging and testing Serial/UART wirings. 
  Often the remote device is mounted somewhere on a rooftop or another 
  inconvenient place. That's where this tool comes in and offers you a 
  simulation for your lab.

  It was intentionally made for the wind direction sensor:
  https://github.com/AndreasExner/iobroker-IoT-WindSensor

  But it can be used for different purposes as well


  

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

#define RTS D5 // RS485 bus master
#define LED D3

#define SERIAL_BAUD_RATE 9600 //baud rate for RS485
#define DEBUG_BAUD_RATE 115200 //baud rate for FT232 / serial1

int counter=0;
bool debug = true;


byte windSensorFrame[] = {0x01, 0x03, 0x04, 0x00, 0x5A, 0x00, 0x00, 0x00, 0x00}; // default answer frame for the wind sensor DO NOT CHANGE
byte customFrame[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // your custom answer frame (CRC will be calculated automatically)

bool useWindSensorFrame = true; // echos a random wind direction if true, a custom frame or a copy of the incoming frame if false
bool useCustomFrame = false; // echos the custom frame if true, a copy of the incoming frame if false 

int expectedFrameSize = 8; // expected RX frame size


//######################################### setup ##############################################################

void setup(void) {

  Serial1.begin(DEBUG_BAUD_RATE);
  delay(1000);

  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);
  
  
  pinMode(LED, OUTPUT);
  pinMode(RTS, OUTPUT);
  digitalWrite(RTS, LOW);
  digitalWrite(LED, LOW);

  if (debug) {Serial1.println("Started....");}
  

}

//####################################################################
// Loop
  
void loop(void) {

// ----------------------- read from buffer

  digitalWrite(RTS, LOW);
  int readBufferSize;

  int x = 10;
  while (x > 0) {  // enables loop every 50 ms
  
      readBufferSize = Serial.available();
      if (readBufferSize >= expectedFrameSize) {break;}
      delay(5);
      x--;
  }

// ----------------------- proceed whe buffer is big enogh to expect a full frame
 
  if(readBufferSize >= expectedFrameSize) {
  
    digitalWrite(LED, LOW);
    byte readBuffer[readBufferSize];
    Serial.readBytes(readBuffer, readBufferSize);
   
    if (debug) {
      Serial1.println("RX Buffer Size: " + String(readBufferSize));
      Serial1.print("RX Buffer : ");
        for ( byte i = 0; i < readBufferSize; i++ ) {
          Serial1.print(hex_to_string(readBuffer[i]));
          Serial1.print(" ");
        }
        Serial1.println("end");
    }

// ----------------------- trim leading zeros (errors) from frame

    while (readBuffer[0] == 0) {
      for (int i = 1; i < readBufferSize; i++) {
        readBuffer[i - 1] = readBuffer[i];
      }
      readBufferSize--;
    }


    if (debug) {
      Serial1.println("Trimmed frame size: " + String(readBufferSize));
      Serial1.print("Frame : ");
        for ( byte i = 0; i < readBufferSize; i++ ) {
          Serial1.print(hex_to_string(readBuffer[i]));
          Serial1.print(" ");
        }
        Serial1.println("end");
    }

// ----------------------- proceed whe buffer is still big enogh to expect a full frame

    if (readBufferSize >= expectedFrameSize) {
      
// ----------------------- CRC Check
  
      uint16_t CRC = Calc_CRC(readBuffer, expectedFrameSize);
      uint8_t CRC_MSB = CRC >> 8;
      uint8_t CRC_LSB = CRC;
      String CRC_Output = hex_to_string(CRC_LSB) + " " + hex_to_string(CRC_MSB);
      bool CRCCheck;
  
      if (readBuffer[expectedFrameSize -2] == CRC_LSB && readBuffer[expectedFrameSize -1] == CRC_MSB) {
        if(debug) {Serial1.println("CRC OK");}
        CRCCheck = true;
      }
      else {
        if(debug) {Serial1.println("CRC ERROR");}
        CRCCheck = false;
      }
  
// ----------------------- send answer: WindSensorFrame with random direction
       
      double ByteTime = (10000000 / SERIAL_BAUD_RATE); // RTS HIGH time for one byte in µs
      
      if (CRCCheck && useWindSensorFrame && !useCustomFrame) {
    
        int echoBufferSize = sizeof(windSensorFrame);
        
        uint16_t windDir = getRandomWindDir(); // add random wind direction from counter
        uint8_t windDir_MSB;
        uint8_t windDir_LSB;
        windDir_MSB = windDir >> 8;
        windDir_LSB = windDir;
        windSensorFrame[4] = windDir_LSB;
        windSensorFrame[3] = windDir_MSB;
    
        CRC = Calc_CRC(windSensorFrame, echoBufferSize);  // calc CRC for frame
        CRC_MSB = CRC >> 8;
        CRC_LSB = CRC;
        windSensorFrame[echoBufferSize -2] = CRC_LSB;
        windSensorFrame[echoBufferSize -1] = CRC_MSB;
    
        if (debug) {
          Serial1.println("TX Buffer size " + String(echoBufferSize) + " bytes");
          
          Serial1.print("TX Buffer : ");
          for ( byte i = 0; i < echoBufferSize; i++ ) {
            Serial1.print(hex_to_string(windSensorFrame[i]));
            Serial1.print(" ");
          }
          Serial1.println("end");
        }
    
        delay(1);
        digitalWrite(RTS, HIGH);
        Serial.write(windSensorFrame, echoBufferSize);
        delayMicroseconds(echoBufferSize * ByteTime);
        digitalWrite(RTS, LOW); 
      }
    
// ----------------------- send answer: custom frame
         
      if (CRCCheck && !useWindSensorFrame && useCustomFrame) {
                  
        int echoBufferSize = sizeof(customFrame);
      
        CRC = Calc_CRC(customFrame, echoBufferSize); // calc CRC for frame
        CRC_MSB = CRC >> 8;
        CRC_LSB = CRC;
        customFrame[echoBufferSize -2] = CRC_LSB;
        customFrame[echoBufferSize -1] = CRC_MSB;
    
        if (debug) {
          Serial1.println("TX Buffer size " + String(echoBufferSize) + " bytes");
          Serial1.print("TX Buffer : ");
          for ( byte i = 0; i < echoBufferSize; i++ ) {
            Serial1.print(hex_to_string(customFrame[i]));
            Serial1.print(" ");
          }
          Serial1.println("end");
        }
        
        delay(1);
        digitalWrite(RTS, HIGH);
        Serial.write(customFrame, echoBufferSize);
        delayMicroseconds(echoBufferSize * ByteTime);
        digitalWrite(RTS, LOW); 
      }
    
// ----------------------- send answer: echo rx frame
         
      if (CRCCheck && !useWindSensorFrame && !useCustomFrame) {
    
        if (debug) {
          Serial1.println("TX Buffer size " + String(expectedFrameSize) + " bytes");
          
          Serial1.print("TX Buffer : ");
          for ( byte i = 0; i < expectedFrameSize; i++ ) {
            Serial1.print(hex_to_string(readBuffer[i]));
            Serial1.print(" ");
          }
          Serial1.println("end");
        }
        
        delay(1);
        digitalWrite(RTS, HIGH);
        Serial.write(readBuffer, expectedFrameSize);
        delayMicroseconds(expectedFrameSize * ByteTime);
        digitalWrite(RTS, LOW); 
      }
      Serial1.println("---------------");
  
      delay(x * 100);  // flush buffer
      byte flushBuffer[readBufferSize];
      Serial.readBytes(flushBuffer, readBufferSize); 
      
    }
    else {
      if(debug) {Serial1.println("Trimmed frame to short");}
    }
  }

  digitalWrite(LED, HIGH);
 
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

#include <Encoder.h>
#include <SPI.h>

// Change these two numbers to the pins connected to your encoder.
//   Best Performance: both pins have interrupt capability
//   Good Performance: only the first pin has interrupt capability
//   Low Performance:  neither pin has interrupt capability
Encoder myEnc(5, 6);

const byte bulbPattern[] = {
 //hgfedcba
  B01110111, // 0
  B00100100, // 1
  B01011101, // 2
  B01101101, // 3
  B00101110, // 4
  B01101011, // 5
  B01111011, // 6
  B00100101, // 7
  B01111111, // 8
  B01101111, // 9
};
uint8_t pattern1[] = {1, 2, 0, 15, 7, 3, 14, 5, 6};
uint8_t pattern2[] = {11, 10, 21, 23, 22, 9, 8, 12, 13};
uint8_t pattern3[] = {28, 31, 29, 30, 17, 27, 19, 26, 18};
uint8_t pattern4[] = {39, 24, 36, 37, 25, 35, 34, 33, 32};

void displayNumber(long number) {
  byte buffer[5] = {
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
  };

  int thousands = number / 1000;
  int hundreds = number / 100 % 10;
  int tens = number / 10 % 10;
  int units = number % 10;

  for (int i = 0; i <= 7; i++) {
    if (!bitRead(bulbPattern[thousands], i)) {
      int bit = pattern1[i];
      bitClear(buffer[bit / 8], bit % 8);
    }
    if (!bitRead(bulbPattern[hundreds], i)) {
      int bit = pattern3[i];
      bitClear(buffer[bit / 8], bit % 8);
    }
    if (!bitRead(bulbPattern[tens], i)) {
      int bit = pattern2[i];
      bitClear(buffer[bit / 8], bit % 8);
    }
    if (!bitRead(bulbPattern[units], i)) {
      int bit = pattern1[i];
      bitClear(buffer[bit / 8], bit % 8);
    }
  }
  spiWrite(buffer);
}
void spiWrite(byte buffer[]) {
  SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
  SPI.transfer(buffer[0]);
  SPI.transfer(buffer[1]);
  SPI.transfer(buffer[2]);
  SPI.transfer(buffer[3]);
  SPI.transfer(buffer[4]);
  SPI.endTransaction();
}

void setup() {
  delay(1000);
  Serial.begin(9600);
  Serial.println("Basic Encoder Test:");
  SPI.begin();
  pinMode(13, OUTPUT);
  analogWrite(13, 255);
}

long oldPosition = -999;

long lastFlip = millis();

float pulse = 0;

void loop() {
  // Pulse pin 13
  pulse += 0.01;
  analogWrite(13, (int)pulse % 255);

  // Test SPI Write
  // spiWrite(B01011010);

  // Select byte position in buffer
  long newPosition = abs(myEnc.read() / 4) % 100;

  if (newPosition != oldPosition) {
    oldPosition = newPosition;
    lastFlip = millis();
    Serial.println(newPosition);
    // Serial.println("Current buffer:");
    // Serial.println(buffer[0], BIN);
    // Serial.println(buffer[1], BIN);
    // Serial.println(buffer[2], BIN);
    // Serial.println(buffer[3], BIN);
    // Serial.println(buffer[4], BIN);

    // Filp preview
    displayNumber(newPosition);

  }
  // Wait for 1 second, then flip the value;
  // if (millis() > (lastFlip + 1000)) {
  //   lastFlip = millis();
  //   Serial.println("flipping position bit");
  //   int bulb = newPosition / 8;
  //   int bitPos = newPosition % 8;
  //   Serial.print(bulb);
  //   Serial.println(bitPos);

  //   bitToggle(buffer[bulb], bitPos);
  //   Serial.println("Current buffer:");
  //   Serial.println(buffer[0], BIN);
  //   Serial.println(buffer[1], BIN);
  //   Serial.println(buffer[2], BIN);
  //   Serial.println(buffer[3], BIN);
  //   Serial.println(buffer[4], BIN);
  
  //   spiWrite();
  // }
}

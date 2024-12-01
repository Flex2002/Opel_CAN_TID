#include <SPI.h>
#include "mcp2515_can.h"    // https://github.com/Seeed-Studio/Seeed_Arduino_CAN
#include <stdio.h>


#define GID_SHOW_TEXT

/*///////////////////////////////////////////////////////
  Opel/Vauxhall configs
    HSCAN - Dual-wire,   High Speed CAN-BUS,   500 kbps
    MSCAN - Dual-wire,   Medium Speed CAN-BUS, 95 kbps
    SWCAN - Single-wire, Low Speed CAN-BUS,    33.3 kbps
*////////////////////////////////////////////////////////


// SPI Chip Select
constexpr uint8_t SPI_CS_PIN = 10;  // Arduino

constexpr uint32_t SERIAL_SPEED = 115200;
mcp2515_can CAN(SPI_CS_PIN);

byte canMsgLen = 0;
unsigned long canId = 0;
unsigned char canMsg[8] = {0};
unsigned long textSentTime = 0;
unsigned long radioOnSentTime = 0;
bool sendTextHeader = false;
bool sendTextPayload = false;
#if defined(GID_SHOW_TEXT)
unsigned char gidText[][8] = {
  {0x10, 0x6C, 0xC0, 0x00, 0x69, 0x03, 0x10, 0x11},
  {0x21, 0x00, 0x7C, 0x00, 0x20, 0x00, 0x20, 0x00},
  {0x22, 0x20, 0x00, 0x47, 0x00, 0x69, 0x00, 0x64},
  {0x23, 0x00, 0x20, 0x00, 0x54, 0x00, 0x65, 0x00},
  {0x24, 0x73, 0x00, 0x74, 0x00, 0x65, 0x00, 0x72},
  {0x25, 0x00, 0x20, 0x00, 0x20, 0x00, 0x7C, 0x11},
  {0x26, 0x10, 0x00, 0x2F, 0x00, 0x2D, 0x00, 0x2D},
  {0x27, 0x00, 0x2D, 0x00, 0x2D, 0x00, 0x2D, 0x00},
  {0x28, 0x2D, 0x00, 0x2D, 0x00, 0x2D, 0x00, 0x2D},
  {0x29, 0x00, 0x2D, 0x00, 0x2D, 0x00, 0x2D, 0x00},
  {0x2A, 0x2D, 0x00, 0x2D, 0x00, 0x5C, 0x12, 0x10},
  {0x2B, 0x00, 0x5C, 0x00, 0x2D, 0x00, 0x2D, 0x00},
  {0x2C, 0x2D, 0x00, 0x2D, 0x00, 0x2D, 0x00, 0x2D},
  {0x2D, 0x00, 0x2D, 0x00, 0x2D, 0x00, 0x2D, 0x00},
  {0x2E, 0x2D, 0x00, 0x2D, 0x00, 0x2D, 0x00, 0x2D},
  {0x2F, 0x00, 0x2D, 0x00, 0x2F, 0x00, 0x00, 0x00}
};
#endif

int serial_putc(char c, FILE *) 
{
  Serial.write(c);

  return c;
} 

void printf_begin(void)
{
  fdevopen(&serial_putc, 0);
}


void setup() {
  Serial.begin(SERIAL_SPEED);

  printf_begin();
    
  Serial.println("Serial print, I don't like it.\n");
  
  int anInt = 32456;

  char *myChars = "good old printf! ";
  
  printf("%s %d    'K?\n\n", myChars, anInt);

  Serial.println("\nOK!");


  while (CAN.begin(CAN_95K2BPS, MCP_8MHz) != CAN_OK) {
    delay(100);
  }

  byte ret = CAN.setMode(MODE_NORMAL);
  if (ret) {
    Serial.println(F("Couldn't enter normal mode"));
  }

#if defined(GID_SHOW_TEXT)
  sendCanMessage(0x691, (const unsigned char[]){0x41, 0x00, 0x60, 0x02, 0x82, 0x00, 0x00, 0x2E}); // Emulate EHU on
  radioOnSentTime = millis();
#else
  sendCanMessage(0x697, (const unsigned char[]){0x41, 0x00, 0x60, 0x02, 0x82, 0x00, 0x00, 0x2E}); // Emulate UHP on?
  delay(2500);
  sendCanMessage(0x697, (const unsigned char[]){0x47, 0x00, 0x60, 0x00, 0x02, 0x00, 0x00, 0x80});
#endif

/*
  String message = generateGidText(0x10, R"(|   Gid Tester  |)", TextAlign::Sentinel, FontWeight::Sentinel);
  message += generateGidText(0x11, R"(/--------------\)", TextAlign::Sentinel, FontWeight::Sentinel);
  message += generateGidText(0x12, R"(\--------------/)", TextAlign::Sentinel, FontWeight::Sentinel);

  message = createGidCanFrames(0xC000, 0x03, message);


  Serial.print("String Length : ");
  Serial.println(message.length());
  for (byte i = 0; i < message.length(); i++) {
    char charValue = message.charAt(i);
    Serial.printf("0x%02X, ", charValue);
  }
  Serial.println("\n\n\n");
*/
}


void loop() {
#if !defined(GID_SHOW_TEXT)
  sendCanMessage(0x697, (const unsigned char[]){0x47, 0x00, 0x60, 0x00, 0x02, 0x00, 0x00, 0x80});
  delay(2500);
#else
  if ((millis() - radioOnSentTime) >= 2500) {
    sendCanMessage(0x691, (const unsigned char[]){0x41, 0x00, 0x60, 0x02, 0x82, 0x00, 0x00, 0x2E});   // Emulate radio on
    //sendCanMessage(0x691, (const unsigned char[]){0x41, 0x00, 0x60, 0x11, 0x02, 0x00, 0x20, 0x08}); // Emulate radio turning off
    radioOnSentTime = millis();
    Serial.println(F("Emulating radio on"));
  }

  if (((millis() - textSentTime) > 2500) && !sendTextHeader && !sendTextPayload) {
    Serial.println(F("Allow sending text"));
    sendTextHeader = true;
    textSentTime = millis();
  }

  if (sendTextHeader) {
    sendTextHeader = false;
    Serial.println(F("Sending header"));

    memcpy(canMsg, &gidText[0][0], sizeof(canMsg));
    sendCanMessage(0x6C1, canMsg);
  }

  if (sendTextPayload) {
    for (size_t i = 1; i < 16; i++) {
      memcpy(canMsg, &gidText[i][0], sizeof(canMsg));
      sendCanMessage(0x6C1, canMsg);
    }

    sendTextPayload = false;
    Serial.println(F("Payload was sent"));
  }

  if (CAN_MSGAVAIL == CAN.checkReceive()) {
    CAN.readMsgBuf(&canMsgLen, canMsg);
    canId = CAN.getCanId();

    if (0x2C1 == canId) {
      if (0x30 == canMsg[0]) { 
        sendTextPayload = true; // Gid send ACK, now we can send the payload
      }
    }
  }
#endif
}


void sendCanMessage(const int id, const unsigned char (&msg)[8]) {
  CAN.sendMsgBuf(id, 0, 8, msg);

  for (byte i = 0; i < 8; i++) {
    char charValue = msg[i];
    printf("0x%02X, ", charValue);
  }
  Serial.println();
}

/* ****************************** **
** Opel CAN-TID                   **
** Water-Temperature in Dashboard **
**                                **
** Hardware: Arduino Uno/Nano 	  **
** CAN-Shield: MCP2515 8Mhz	      **
** **by Flex2002***************** */ 
/* ******************************************************* **
** Opel / Vauxhall | Corsa D / Astra H		                 **
** HSCAN - Dual-wire,   High Speed CAN-BUS,   500 kbps     **
** MSCAN - Dual-wire,   Medium Speed CAN-BUS, 95.238 kbps  **
** SWCAN - Single-wire, Low Speed CAN-BUS,    33.3 kbps    **
** ******************************************************* */

// ** Declaration and includes** //
#include <SPI.h>
#include "mcp2515_can.h"    					                      //Requires adjustment for 95,238 kbps https://github.com/Seeed-Studio/Seeed_Arduino_CAN
// ** Hardware-Setup **//
constexpr uint8_t CAN1_SPEED = CAN_95K238BPS;	              //Mid-Speed has to be defined in mcp2515_can.h
constexpr uint8_t CAN1_SPI_CS = 10;  			                  //Chip-Select-Pin
mcp2515_can CAN1(CAN1_SPI_CS);					                    //Instantiate CAN object

// **Configuration ** //
constexpr unsigned long radioOnSendInterval = 2500; 	      //Interval for sending "Radio-ON" message (ms)
constexpr unsigned long sendtextSendInterval = 2500; 	      //Interval for sending the display message (ms)
constexpr unsigned long requestSendInterval = 10000; 	      //Interval for sending a request message (ms)
constexpr uint32_t SERIAL_SPEED = 115200;		                //Baud rate for Serial Monitor
// ** Feature Toggles ** //                                 
#define DEBUG                             									//Uncomment to enable debugging output
//#define TEST_MESSAGE 							                        //Uncomment to send predefined test message

// ** DEBUGGING
#ifdef DEBUG
	#include <stdio.h> 							                          
	#define DEBUG_PRINT(x) 	 Serial.print(x)
	#define DEBUG_PRINTLN(x) Serial.println(x)
	#define DEBUG_PRINTF(x, y)	 printf(x, y)
  //Implementierung von printf
	int serial_putc(char c, FILE *) {
	Serial.write(c);
	return c;
	} 
	void printf_begin(void) { fdevopen(&serial_putc, 0); 	}
#else
	#define DEBUG_PRINT(x)
	#define DEBUG_PRINTLN(x)
	#define DEBUG_PRINTF(x)
#endif

// ** CAN Communication Variables ** //
unsigned char canMsgBuffer[8] = {0}; 					              //Buffer for CAN-messages "send" and "recieve"
unsigned long radioOnSentTime = 0;				                  //Timestamp of last "Radio ON" message
unsigned long lastSendTime = 0;                             //Timestamp of last display text message
unsigned long lastrequestTime = 0;                          //Timestamp of last message request
// ** Dynamic Array Structure for Multimessage Handling ** //
struct Array2D {                                            
    unsigned char (*data)[8];                               //Pointer to 2D Array 
    size_t rows;			                                      //Placeholder for rows
    size_t columns;			                                    //Placeholder for colums
};
Array2D msgArray;                                           //Create "dynamic" array for multimessage
size_t sendeZeilen = 0;                                     //Row counter for message array
size_t sendeSpalten = 0;                                    //Coulumn Counter for message Array
size_t BuchstabenAnzahl = 0;                                //Character counter of message text
// ** Flags for Multi-Message Handling ** //
bool sendTextHeader = false;                                //Flag for sending message header
bool sendTextPayload = false;                               //Flag for sending message payload
bool setNullbyte = true;                                    //Flag to insert null-byte during message assembly
// ** Variables for Recieving Messages ** //
unsigned long canId = 0;                                    //ID of received CAN message
byte canMsgLen = 0;                                         //Length of received CAN message
// ** Message Construction Variables ** //
int coolantTemp = 0;                                        // Coolant temperature (stored value)
String textvar = "--";                                      // Placeholder text for temperature display

// ** Function Declarations ** //
void fillMsgArray(Array2D& msgArray, const String& utf8Text);
void fillHeader(Array2D& msgArray);
void clearMsgArray(Array2D& msgArray);

// ** Predefined Test Message (if enabled) ** //
#ifdef TEST_MESSAGE
	unsigned char fixMsg [][8] = {				
	  {0x10, 0x17, 0xC0, 0x00, 0x14, 0x03, 0x10, 0x38},
	  {0x21, 0x00, 0x46, 0x00, 0x6C, 0x00, 0x65, 0x00},
	  {0x22, 0x78, 0x00, 0x32, 0x00, 0x30, 0x00, 0x30},
	  {0x23, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x2A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x2B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x2C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x2D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x2E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x2F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	};
#else {
  unsigned char varMsg [][8] = {  
    {0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x2A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x2B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x2C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x2D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x2E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	  {0x2F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	};
#endif

// ----------------------------------------------------------------------------------------------------------------------------------- // 
// ** Program - Start ** //
void setup () {
  #ifdef DEBUG
    Serial.begin(SERIAL_SPEED);                             //Initialize serial communication for debugging
    printf_begin();                                         //Initialize printf functionality
  #endif

  //Initialize the CAN bus with the specified speed and check for success
  while (CAN1.begin(CAN1_SPEED, MCP_8MHz) != CAN_OK) {   
	  DEBUG_PRINT("CAN1 Initialisierung... ");
	  delay(1000);
  };
  DEBUG_PRINTLN("wurde initialisiert");
  byte ret = CAN1.setMode(MODE_NORMAL);
  if (ret) DEBUG_PRINTLN(F("konnte nicht in Normal-Modus gesetzt werden."));
  
  //Initialize the message array for sending
  #ifdef TEST_MESSAGE
    msgArray.data = fixMsg;
    msgArray.rows = sizeof(fixMsg) / sizeof(fixMsg[0]);
    msgArray.columns = sizeof(fixMsg[0]) / sizeof(fixMsg[0][0]);
  #else
    msgArray.data = varMsg;
    msgArray.rows = sizeof(varMsg) / sizeof(varMsg[0]);
    msgArray.columns = sizeof(varMsg[0]) / sizeof(varMsg[0][0]);
  #endif
  #ifdef DEBUG
    DEBUG_PRINT("msgArray wird initialisiert: ");  
    for (size_t row = 0; row < msgArray.rows; ++row) {
      for (size_t col = 0; col < msgArray.columns; ++col) {
        DEBUG_PRINTF("0x%02X ", msgArray.data[row][col]);    
      }
    }
    DEBUG_PRINTLN("abgeschlossen");
  #endif

  //Simulate a "Radio On" message of CD30MP3 radio system
  DEBUG_PRINT("Sende Initialisierungsnachricht:");
  sendRadioON();
  DEBUG_PRINTLN("gesendet");
}
// ----------------------------------------------------------------------------------------------------------------------------------- //
// ** Loop Function ** //
void loop () {
  //Check if it's time to resend the "Radio-On" message
  if (millis() - radioOnSentTime >= radioOnSendInterval) { sendRadioON(); }
  //Check if it's time to send a (multi)message
  if (((millis() - lastSendTime) >= sendtextSendInterval) && !sendTextHeader && !sendTextPayload) {
    DEBUG_PRINTLN(F("Sendebereitschaft"));
    sendTextHeader = true;
    lastSendTime = millis();
    msgBuilder();                                           //Build the message and array
  }

  //Send the header and wait for confirmation
	if (sendTextHeader) {
    DEBUG_PRINT("Header:");
    memcpy(canMsgBuffer, &msgArray.data[0][0], sizeof(canMsgBuffer));         // Copy the header to the CAN message buffer     
    sendCanMessage(0x6C1, canMsgBuffer);                                      //Send the header of the array
    sendTextHeader = false;                                                   //Reset the header flag
  }
  //Send the rest of the message payload, if confirmation has been received
  if (sendTextPayload) {
    for (size_t row_i = 1; row_i < msgArray.rows; row_i++) {                  // Loop through the rows of the message array                         
      memcpy(canMsgBuffer, &msgArray.data[row_i][0], sizeof(canMsgBuffer)); 
      sendCanMessage(0x6C1, canMsgBuffer);                                    // Send the payload CAN message
    }
    sendTextPayload = false;                                                  // Reset the payload sending flag
    DEBUG_PRINTLN(F("Payload gesendet."));
    clearMsgArray(msgArray);                                                  // Clear the message array for the next use
  }
  
  //Send a request for information 
  if ((millis() - lastrequestTime) >= requestSendInterval) {
    DEBUG_PRINT("Request Daten:");
    canMsgBuffer[0] = 0x07;                                 //Message bytes
    canMsgBuffer[1] = 0xAA;                                 // ?
    canMsgBuffer[2] = 0x03;                                 // ?
    canMsgBuffer[3] = 0x0B;                                 //Mesurement Blocks 0B = coolant temperature
    canMsgBuffer[4] = 0x0B;                                 //next for multiple requests
    canMsgBuffer[5] = 0x0B;
    canMsgBuffer[6] = 0x0B;
    canMsgBuffer[7] = 0x0B;
    sendCanMessage(0x246, canMsgBuffer);                    // Send the request message on CAN bus
    lastrequestTime = millis();
  }

  // Check incoming CAN messages
  if (CAN_MSGAVAIL == CAN1.checkReceive()) {
    CAN1.readMsgBuf(&canMsgLen, canMsgBuffer);
    canId = CAN1.getCanId();                                // Get the ID of the received CAN message
    
    // Handle messages from
    if (0x2C1 == canId) {                                   //0x2C1 Display
      if (0x30 == canMsgBuffer[0]) {                        //Confirmation for payload
        sendTextPayload = true;                             //Set the flag to sen payload
        DEBUG_PRINTLN("SENDE PAYLOAD");
      }
    }
    if (0x546 == canId) {                                   //0x546 Coolant temperature
        coolantTemp = canMsgBuffer[5]-40;                   //calculation for °C 
    }
  }
}
// ----------------------------------------------------------------------------------------------------------------------------------- //

// ** Functions ** //
// Send a CAN message
void sendCanMessage(const int id, const unsigned char (&msg)[8]) {
  CAN1.sendMsgBuf(id, 0, 8, msg);
  for (byte i = 0; i < 8; i++) {                            //Debug print
    char charValue = msg[i];
    DEBUG_PRINTF("0x%02X, ", charValue);
  }
  DEBUG_PRINTLN("gesendet");
}

void sendRadioON() {
  DEBUG_PRINT("Radio 'ein':");
    sendCanMessage(0x691, (const unsigned char[]){0x41, 0x00, 0x60, 0x02, 0x82, 0x00, 0x00, 0x2E});   //Radio "on"
  //sendCanMessage(0x691, (const unsigned char[]){0x41, 0x00, 0x60, 0x11, 0x02, 0x00, 0x20, 0x08});   //Radio "off"
    radioOnSentTime = millis();
}

// Builds the message to be sent
void msgBuilder(){
    #ifdef TEST_MESSAGE
    #else
      DEBUG_PRINTLN("Baue die Nachricht");
      // -- Schriftgröße und Ausrichtung
      //char esc[0];
      //esc[0] = 0x1b;
      //char right_alignm = "[fS_gm";
     
      
      //Message constants
      char combinedText[50]={0};                           //Buffer for message text
      char* baustein1 = "Wasser: ";                        //First part of the message text
      char* baustein2 = "°C";                              //Last part of the message text
      //Message variable
      if (coolantTemp >= 20) {
        textvar = String(coolantTemp);
      }
      else {
        textvar = "--";
      }

      //Build the message text
      strcpy(combinedText, baustein1);
      //strcat(combinedText, esc);
      //strcat(combinedText, right_alignm);                      
      strcat(combinedText, textvar.c_str());                
      strcat(combinedText, baustein2);
      DEBUG_PRINTLN(combinedText);

      //Fill message array and build header
      fillMsgArray(msgArray, combinedText);           
      fillHeader(msgArray);                           
      memset(combinedText, 0, 50);                          //Clear buffer for message text
      
      #ifdef DEBUG                                          //Debug output: Print the message array after building
        DEBUG_PRINTLN("Array nach dem Füllen:");
          for (size_t row_i = 0; row_i < msgArray.rows; ++row_i) {
            for (size_t column_i = 0; column_i < msgArray.columns; ++column_i) {
              DEBUG_PRINTF("0x%02X ", msgArray.data[row_i][column_i]);
            }
            DEBUG_PRINTLN();
          }
      #endif
    #endif
}
//Fills the header section of the message array
void fillHeader(Array2D& msgArray) {
      int  sizedatablock = (sendeZeilen * 7) + sendeSpalten -2; 
      int  sizepayload = sizedatablock - 3;                     
      int  sizechar = BuchstabenAnzahl; 

      msgArray.data[0][1] = sizedatablock;                  //Total size of the data block
      msgArray.data[0][2] = 0xC0;                           //Command 
      msgArray.data[0][3] = 0x00;                           //Command 
      msgArray.data[0][4] = sizepayload;                    //Total size of the payload block
      msgArray.data[0][5] = 0x03;                           //Type: Mainscreen
      msgArray.data[0][6] = 0x10;                           //Type-ID: "Song titel"
      msgArray.data[0][7] = sizechar;                       //Number of text characters 
}
//Fills the message array with UTF-8 text
void fillMsgArray(Array2D& msgArray, const String& utf8Text) {
  size_t textIndex = 0; 				                            // Index of the current character in the text
  sendeZeilen = 0;                      
  BuchstabenAnzahl = 0;
  BuchstabenAnzahl = utf8Text.length();
  setNullbyte = true;

  DEBUG_PRINT("Buchstaben:"); DEBUG_PRINTLN(BuchstabenAnzahl);
	DEBUG_PRINTLN("Array:");
  
  for (size_t row_i = 1; row_i < msgArray.rows; ++row_i) {
    for (size_t column_i = 1; column_i < msgArray.columns; ++column_i) {
      if (setNullbyte) {                                    //Insert 0x00 for separation
        msgArray.data[row_i][column_i] = 0x00;     
        setNullbyte = !setNullbyte;
      } else if (utf8Text[textIndex] != '\0') {
        DEBUG_PRINT(utf8Text[textIndex]);
        unsigned char asciiValue = utf8Text[textIndex];
        
        asciiFilter(asciiValue);                            //Filter for extended ASCII values if necessary
        //if (asciiValue==194) {asciiValue = 0; }               //Handle specific character case °
        DEBUG_PRINTLN(asciiValue);

        char buffer[5] = "";                                //Temp Buffer
        itoa(asciiValue, buffer, 16);

        msgArray.data[row_i][column_i] = asciiValue;        // Add the character to the array

        ++textIndex;
        setNullbyte =! setNullbyte;
      } else {
        DEBUG_PRINTLN("Vorzeitiger Ausstieg");
        sendeZeilen = row_i;
        sendeSpalten = column_i;
        return;
      }
    } 
  }
  DEBUG_PRINTLN("Ende von Array erreicht.");
  sendeZeilen = msgArray.rows;
  sendeSpalten = msgArray.columns; 
}

//Clear the message array
void clearMsgArray(Array2D& msgArray){
  for (size_t row_i = 1; row_i < msgArray.rows; ++row_i) {
    for (size_t column_i = 1; column_i < msgArray.columns; ++column_i) {
      msgArray.data[row_i][column_i] = 0x00;     
    }
  }
}

// Filters ASCII values for invalid or special characters
void asciiFilter(unsigned char &asciiValue){
  if (asciiValue < 128) {                   
    return asciiValue;                      // Standard ASCII values remain unchanged
  } 
  else {
    switch (asciiValue) {
      case 128:
        asciiValue = 128; // Euro symbol
        break;
      case 194:
        asciiValue = 0;   // Skip specific value
        break;
      default:
        asciiValue = 95;  // Replace with underscore (_)
        break;
    }
  }
  return asciiValue;
}

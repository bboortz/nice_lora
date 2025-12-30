#include <SPI.h>
#include <RadioLib.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// LILYGO T3 V1.6.x pin definitions
#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_CS 18
#define LORA_RST 23
#define LORA_IRQ 26

// OLED
#define OLED_SDA 21
#define OLED_SCL 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// LoRa parameters - MUST MATCH TRANSMITTER
#define LORA_FREQ 868.0
#define LORA_BW 125.0
#define LORA_SF 9
#define LORA_CR 7
#define LORA_SYNC 0x12
#define LORA_PREAMBLE 8

#define MAX_LOOP 100
#define MAX_INRX 10

// Create SX1276 instance
SX1276 radio = new Module(LORA_CS, LORA_IRQ, LORA_RST, RADIOLIB_NC);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

enum state_enum {INIT, SETUP_DONE, DO_RX, DO_TX, IN_RX, IN_TX, IDLE, FAILED};
state_enum systemState = INIT;

String user = "Benni2";
int loopCount = 0;
int inRxCount = 0;
int rxPacketCount = 0;
int txPacketCount = 0;


int radiolibState = 0;
int rxState = 0;
int txState = 0;
String systemStatus = "INIT";
String task = "INIT";
String rxStatus = "INIT";
String txStatus = "INIT";
// String rxMessage = "-";
String txMessage = "-";
String currentMethod = "setup";
// int lastSNR = 10;

// Store last 3 received messages
String rxHistory[3] = {"", "", ""};
float snrHistory[3] = {0.0, 0.0, 0.0};


void addToRxHistory(String message, float snr) {
  // Shift older messages down
  rxHistory[2] = rxHistory[1];
  rxHistory[1] = rxHistory[0];
  rxHistory[0] = message;
  
  snrHistory[2] = snrHistory[1];
  snrHistory[1] = snrHistory[0];
  snrHistory[0] = snr;
}

String userConstWidth(String user) {
  int len = user.length();
  
  if (len == 6) {
    return user;
  } else if (len == 5) {
    return " " + user;
  } else if (len == 4) {
    return "  " + user;
  } else if (len == 3) {
    return "   " + user;
  } else if (len == 2) {
    return "    " + user;
  } else {
    return "     " + user;
  }
}

String getProgressStar() {
  static int counter = 0;
  String chars[] = {"/", "-", "\\", "|"};
  
  String result = chars[counter % 4];
  counter++;
  
  return result;
}

String snrToBars(float snr) {
  if (snr > 15) {
    return "|||||||||||";  // Excellent
  } else if (snr > 12) {
    return " ||||||||||";  // Excellent
  } else if (snr > 9) {
    return "  |||||||||";  // Very Good
  } else if (snr > 6) {
    return "   ||||||||";  // Very Good
  } else if (snr > 3) {
    return "    |||||||";  // Good
  } else if (snr > 0) {
    return "     ||||||";  // Good
  } else if (snr > -3) {
    return "      |||||";  // Fair
  } else if (snr > -6) {
    return "       ||||";  // Weak
  } else if (snr > -9) {
    return "        |||";  // Weak
  } else if (snr > -12) {
    return "         ||";  // Poor
  } else {
    return "          |";  // Very Poor
  }
}

String snrToStars(float snr) {
  int stars;
  
  // Determine number of stars based on SNR
  if (snr > 15) {
    stars = 11;
  } else if (snr > 12) {
    stars = 10;
  } else if (snr > 9) {
    stars = 9;
  } else if (snr > 6) {
    stars = 8;
  } else if (snr > 3) {
    stars = 7;
  } else if (snr > 0) {
    stars = 6;
  } else if (snr > -3) {
    stars = 5;
  } else if (snr > -6) {
    stars = 4;
  } else if (snr > -9) {
    stars = 3;
  } else if (snr > -12) {
    stars = 2;
  } else {
    stars = 1;
  }
  
  // Build the string with stars and padding
  String result = "";
  
  // Add leading spaces
  for (int i = 0; i < (11 - stars); i++) {
    result += " ";
  }
  
  // Add stars (using | character)
  for (int i = 0; i < stars; i++) {
    result += "*";
  }
  
  return result;
}

void setSystemState(state_enum stateobj) {
  systemState = stateobj;
  Serial.println("--> " + String(systemState));

  switch ( systemState ) {
    case INIT:
      systemStatus = "INIT";
      break;
    
    case SETUP_DONE:
      systemStatus = "SETUP";
      break;
      
    case DO_RX:
      systemStatus = "RX";
      break;
      
    case DO_TX:
      systemStatus = "TX";
      break;
    
    case IN_RX:
      systemStatus = "RX";
      break;
      
    case IN_TX:
      systemStatus = "TX";
      break;
      
    case IDLE:
      systemStatus = "IDLE";
      break;
      
    case FAILED:
      systemStatus = "FAIL";
      break;
  }
  
}

String getSystemState() {

  switch ( systemState ) {
    case INIT: 
      return "INIT";
    
    case SETUP_DONE:
      return "SETUP_DONE";
      
    case DO_RX:
      return "DO_RX";
      
    case DO_TX:
      return "DO_TX";
    
    case IN_RX:
      return "IN_RX";
      
    case IN_TX:
      return "IN_TX";
      
    case IDLE:
      return "IDLE";
      
    case FAILED:
      return "FAILED";
  }

  return "FAILED";
  
}

int doRadiolibState(int state) {
  if (state != RADIOLIB_ERR_NONE) {
    systemStatus = "FAILED";
    setSystemState(FAILED);
    return 1;
  }

  return 0;
}

void setStatus(state_enum newState, String newTask, String newFunc) {
 setSystemState(newState);
 task = newTask;
 currentMethod = newFunc;
 updateStatus();
}

/*
 * * LoraTopfschlagen *
 * S:
 * H:
 * T:
 * F:
 * C:
 * 
 */
void updateStatus(void) {
  rxStatus = "TBC";
  txStatus = "TBC";

  if (rxState == RADIOLIB_ERR_NONE) {
    rxStatus = "OK";
  } else {
    rxStatus = rxState;
  } 

  if (txState == RADIOLIB_ERR_NONE) {
    txStatus = "OK";
  } else {
    txStatus = txState;
  } 

  String header = "* LoraTopfschlagen *";
  String stateMessage = getProgressStar() + " " + getSystemState() + " " + radiolibState + " " + rxState + " " + txState; 
  // String statusMessage = "S " + systemStatus + " / " + rxStatus + " / " + txStatus;
  String taskMessage = "T " + task;
  
  String functionMessage = "F " + currentMethod;
  String countMessage = "C " + String(loopCount) + " / " + String(rxPacketCount) + " / " + String(txPacketCount) + " / " + String(inRxCount);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(header);
  display.println(stateMessage);
  // display.println(statusMessage);
  // display.println(taskMessage);
  // display.println(functionMessage);
  display.println(countMessage);
  display.println("> " + txMessage);

  // Display last 3 received messages with SNR
  display.println("         15   0  -15");
  for (int i = 0; i < 3; i++) {
    if (rxHistory[i] != "") {
      String user = userConstWidth(rxHistory[i]);
      String bars = snrToStars(snrHistory[i]);
      // display.println("< " + rxHistory[i] + " " + String(snrHistory[i]));
      display.println(" " + user + " |"+ bars);
    }
  }
  
  display.display();
}

void enableRX(void) {
  radiolibState = radio.startReceive();
  if ( doRadiolibState(radiolibState) ) {
    return;
  }
}

void onPacketRX(void) {
  Serial.println("********************************* do rx");
  setSystemState(DO_RX);
  inRxCount++;
}

/*
void afterPacketTX(void) {
  Serial.println("********************************* do tx");
  setSystemState(IDLE);
  
}
*/

void parsePacket(String data, String *user, int *number, float *snr) {
    char buffer[64];
    data.toCharArray(buffer, sizeof(buffer));
    
    char userBuf[32];
    
    // Try to parse all 3 fields first
    int result = sscanf(buffer, "%s %d %f", userBuf, number, snr);
    
    if (result == 3) {
        // Format: "user1 number snr"
        *user = String(userBuf);
    } else if (result == 2) {
        // Format: "user number" (no SNR provided)
        *user = String(userBuf);
        *snr = 0.0;  // or use NAN if you want to mark it as invalid
    } else {
        // Parse error
        *user = "";
        *number = 0;
        *snr = 0.0;
    }
}


void decode_packet(void) {
  Serial.println("********************************* decoding!!");
  if (systemState != DO_RX) {
    return;
  }
  rxPacketCount++;
  setStatus(IN_RX, "receiving...", "decode_packet()");

  String data;
  rxState = radio.readData(data);
  int packetLength = radio.getPacketLength();
  Serial.println("Len: " + String(packetLength) + "\n");
  if (packetLength != 0) {
    setStatus(IN_RX, "decoding...", "decode_packet()");
    
    if (rxState == RADIOLIB_ERR_NONE) {
      //float freqErr = radio.getFrequencyError();
      //float rssi = radio.getRSSI();
      float snr = radio.getSNR();
      // lastSNR = snr;
      
      // OLED output
      String receivedUsername;
      int receivedNum;
      float receivedSnr;
      parsePacket(data, &receivedUsername, &receivedNum, &receivedSnr);
      addToRxHistory(receivedUsername, snr);  // Add this line
      // addToRxHistory(data, snr);  // Add this line
      
      // rxMessage = data.substring(0, 21);
      // Serial.println("MSG: " + rxMessage + "\n");
     
    } else if (rxState == RADIOLIB_ERR_RX_TIMEOUT) {
      // Timeout - no packet, just keep listening
      rxStatus = "TIMEOUT";
      
    } else if (rxState == RADIOLIB_ERR_CRC_MISMATCH) {
      Serial.println("CRC error!");
      rxStatus = "CRC";
      
    } else {
      Serial.printf("Receive failed, error: %d\n", rxState);
      rxStatus = String(rxState);
    }
    updateStatus();
      
  }
  packetLength = 0;
  
  setStatus(IDLE, "idling...", "loop()");
}


void transmit_packet(void) {
  if (systemState != DO_TX) {
    return;
  }
  txPacketCount++;
  setStatus(IN_TX, "transmitting...", "transmit_packet()");

  
  // Build message
  txMessage = user + " " + String(txPacketCount) + " " + String(snrHistory[0]); 

  txState = radio.transmit(txMessage);
  enableRX();
  updateStatus();

  setStatus(IDLE, "idling...", "loop()");
}


void setup_board() {
  currentMethod = "setup_board()";
  task = "INIT";
  systemStatus = "INIT";
  
  delay(1000);
  Serial.begin(115200);
  while (!Serial);
 
  // Init OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    setStatus(systemState, "setup display", currentMethod);
  }

  delay(1000);

  // Init SPI
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

  // setup lora
  setStatus(systemState, "setup lora", currentMethod);
  radiolibState = radio.begin(
    LORA_FREQ,
    LORA_BW,
    LORA_SF,
    LORA_CR,
    LORA_SYNC,
    10,             // output power (not used for RX)
    LORA_PREAMBLE
  );
  if ( doRadiolibState(radiolibState) ) {
    return;
  }
  delay(1000);

  // setup receiver
  setStatus(systemState, "setup receiver", currentMethod);
  radio.setPacketReceivedAction(onPacketRX);
  //radio.setDio1Action(on_packet);
  enableRX();
  delay(1000);

  // setup done
  setStatus(SETUP_DONE, "-", "loop()");
  delay(1000);
  
}

void setup() {
  setup_board();
}



void loop() {
  Serial.println("\n*** STATE: " + String(systemState)); 
  loopCount++;
  updateStatus();
  
  switch ( systemState ) {
    case INIT:
      Serial.println("* INIT");
      setup_board();
      break;
    
    case SETUP_DONE:
      Serial.println("* SETUP_DONE");
      setStatus(IDLE, "-", "loop");
      break;
      
    case DO_RX:
      Serial.println("* DO_RX");
      decode_packet();
      break;
      
    case DO_TX:
      Serial.println("* DO_TX");
      transmit_packet();
      break;
    
    case IN_RX:
      // should never fall into this state
      Serial.println("* IN_RX");
      break;
      
    case IN_TX:
      // should never fall into this state
      Serial.println("* IN_TX");
      break;
      
    case IDLE:
      Serial.println("* IDLE");
      delay(10);
      Serial.println("inRxCount: " + String(inRxCount));
      if (loopCount >= MAX_LOOP || inRxCount >= MAX_INRX) {
        setSystemState(DO_TX);
        loopCount = 0;
        inRxCount = 0;
      }
      break;
 
    case FAILED:
      Serial.printf("Failed! Error code: %d\n", systemState);
      display.println("INIT FAILED!");
      display.display();
      while (1);
  }
  
}

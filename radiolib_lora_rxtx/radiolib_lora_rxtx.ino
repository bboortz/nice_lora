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

#define MAX_INRX 0

// Create SX1276 instance
SX1276 radio = new Module(LORA_CS, LORA_IRQ, LORA_RST, RADIOLIB_NC);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

enum state_enum {INIT, SETUP_DONE, DO_RX, DO_TX, IN_RX, IN_TX, IDLE, FAILED};
int inRxCount = 0;
state_enum state = INIT;

String user = "Benni";
//int inRX = 0;
//int inTX = 0;
int rxPacketCount = 0;
int txPacketCount = 0;
int rxState = 0;
int txState = 0;
String systemStatus = "INIT";
String task = "INIT";
String rxStatus = "INIT";
String txStatus = "INIT";
String rxMessage = "-";
String txMessage = "-";
String currentMethod = "setup";
int lastSNR = 10;


void setState(state_enum stateobj) {
  state = stateobj;
}


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
  String statusMessage = "S: " + systemStatus + " / " + rxStatus + " / " + txStatus;
  String taskMessage = "T: " + task;
  
  String functionMessage = "F: " + currentMethod;
  String countMessage = "C: " + String(rxPacketCount) + " / " + String(txPacketCount) + " / " + String(inRxCount);
//  String inMessage = "I: " + String(inRX) + " / " + String(inTX);

  Serial.println(statusMessage);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(header);
  display.println(statusMessage);
  display.println(taskMessage);
  display.println(functionMessage);
  display.println(countMessage);
  // display.println(inMessage);
  display.println("< " + rxMessage + " " + String(lastSNR));
  display.println("> " + txMessage);
  display.display();
}


void on_packet(void) {
  setState(IN_RX);
  inRxCount++;
}

void decode_packet(void) {
  currentMethod = "decode_packet()";
  systemStatus = "RX";
  updateStatus();
  /*
  while (inTX) {
    delay(10);
  }
  */
  //inRX = 1;
  //systemStatus = "RX";
  //updateStatus();
  
  String data;
  rxPacketCount++;

  task = "receiving...";
  updateStatus();

  rxState = radio.receive(data);
  updateStatus();
  
  // rxState = radio.receive(data);
  display.println("after receive");
  display.display();
  
  if (rxState == RADIOLIB_ERR_NONE) {
    float rssi = radio.getRSSI();
    float snr = radio.getSNR();
    float freqErr = radio.getFrequencyError();
    lastSNR = snr;
    
    // OLED output
    rxMessage = data.substring(0, 21);
    /*
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("LoRa RX RadioLib");
    display.printf("Pkt #%d\n", rxPacketCount);
    display.printf("RSSI: %.0f dBm\n", rssi);
    display.printf("SNR: %.1f dB\n", snr);
    display.printf("Err: %.0f Hz\n", freqErr);
    display.println(data.substring(0, 21));
    
    display.display();
    */
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

  setState(IDLE);
  //inRX = 0;
}


void packet_transmit(void) {
  currentMethod = "packet_transmit()";
  task = "transmit";
  /*while (inRX) {
    delay(10);
  }
  */
  // inTX = 1;
  systemStatus = "TX";
  
  txPacketCount++;
  
  // Build message
  txMessage = user + " " + String(txPacketCount) + " " + String(lastSNR); 
  
  // Transmit
  Serial.printf("> TX Packet #%d: %s ... ", txPacketCount, txMessage.c_str());
  display.println("> " + txMessage);
  display.display();

  txState = radio.transmit(txMessage);
  updateStatus();

  // inTX = 0;
}


void setup() {
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
    display.println("LoRa RXTX RadioLib");
    display.printf("%.1f MHz SF%d\n", LORA_FREQ, LORA_SF);
    display.println("Starting...");
    display.display();
  }

  delay(1000);

  // Init SPI
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

  // Init LoRa
  Serial.print("Initializing LoRa... ");
  int state = radio.begin(
    LORA_FREQ,
    LORA_BW,
    LORA_SF,
    LORA_CR,
    LORA_SYNC,
    10,             // output power (not used for RX)
    LORA_PREAMBLE
  );
  

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("OK!");
  } else {
    Serial.printf("Failed! Error code: %d\n", state);
    display.println("INIT FAILED!");
    display.display();
    while (1);
  }

  // listening
  task = "SETUP RECEIVER";
  updateStatus();
  delay(1000);
  
  //radio.setDio1Action(packet_received);
  radio.setPacketReceivedAction(on_packet);
  state = radio.startReceive();

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("OK!");
  } else {
    Serial.printf("Failed! Error code: %d\n", state);
    display.println("INIT FAILED!");
    display.display();
    while (1);
  }
  
  systemStatus = "SETUP DONE";
  state = SETUP_DONE;
  updateStatus();

  delay(1000);
}



void loop() {
  
  delay(10);

  switch ( state ) {
    case SETUP_DONE:
      state = IN_RX;
      break;
    case IN_RX:
      decode_packet();
      if (inRxCount >= MAX_INRX) {
        packet_transmit();
        inRxCount = 0;
      }
      break;
    case IDLE:
      //if (inRxCount >= MAX_INRX) {
        packet_transmit();
        inRxCount = 0;
      //}
      break;
  }
  
}

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

// LoRa parameters - MUST MATCH RECEIVER
#define LORA_FREQ 868.0
#define LORA_BW 125.0
#define LORA_SF 7
#define LORA_CR 5
#define LORA_SYNC 0x12
#define LORA_PREAMBLE 8
#define LORA_TX_POWER 17

// Transmit interval
#define TX_INTERVAL 5000

// Create SX1276 instance (LILYGO T3 uses SX1276)
SX1276 radio = new Module(LORA_CS, LORA_IRQ, LORA_RST, RADIOLIB_NC);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int packetCount = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  Serial.println("\n\n=== LORA TRANSMITTER (RadioLib) ===");
  Serial.println("Configuration:");
  Serial.printf("  Frequency: %.1f MHz\n", LORA_FREQ);
  Serial.printf("  Bandwidth: %.1f kHz\n", LORA_BW);
  Serial.printf("  Spreading Factor: %d\n", LORA_SF);
  Serial.printf("  Coding Rate: 4/%d\n", LORA_CR);
  Serial.printf("  Sync Word: 0x%02X\n", LORA_SYNC);
  Serial.printf("  TX Power: %d dBm\n", LORA_TX_POWER);
  Serial.println("===================================\n");

  // Init OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("LoRa TX RadioLib");
    display.printf("%.1f MHz SF%d\n", LORA_FREQ, LORA_SF);
    display.println("Starting...");
    display.display();
  }

  // Init SPI
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

  // Init LoRa
  Serial.print("Initializing LoRa... ");
  int state = radio.begin(
    LORA_FREQ,      // frequency (MHz)
    LORA_BW,        // bandwidth (kHz)
    LORA_SF,        // spreading factor
    LORA_CR,        // coding rate
    LORA_SYNC,      // sync word
    LORA_TX_POWER,  // output power (dBm)
    LORA_PREAMBLE   // preamble length
  );

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("OK!");
  } else {
    Serial.printf("Failed! Error code: %d\n", state);
    display.println("INIT FAILED!");
    display.display();
    while (1);
  }

  Serial.println("Transmitting...\n");
}

void loop() {
  packetCount++;
  
  // Build message
  String message = "Paket Nr. " + String(packetCount);
  
  // Transmit
  Serial.printf("TX Packet #%d: %s ... ", packetCount, message.c_str());
  
  int state = radio.transmit(message);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("OK!");
  } else {
    Serial.printf("Failed! Error: %d\n", state);
  }
  
  // OLED output
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("LoRa TX RadioLib");
  display.printf("%.1f MHz SF%d\n", LORA_FREQ, LORA_SF);
  display.println("");
  display.printf("Sent: #%d\n", packetCount);
  display.println(message);
  display.display();
  
  delay(TX_INTERVAL);
}

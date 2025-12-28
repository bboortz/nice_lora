#include <SPI.h>
#include <LoRa.h>
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
#define LORA_FREQ 868E6
#define LORA_BW 125E3
#define LORA_SF 7
#define LORA_CR 5
#define LORA_SYNC 0x12
#define LORA_PREAMBLE 8
#define LORA_TX_POWER 17   // dBm (max 20)

// Transmit interval
#define TX_INTERVAL 5000   // ms between packets

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int packetCount = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  Serial.println("\n\n=== LORA TRANSMITTER ===");
  Serial.println("Configuration:");
  Serial.printf("  Frequency: %.1f MHz\n", LORA_FREQ / 1E6);
  Serial.printf("  Bandwidth: %.1f kHz\n", LORA_BW / 1E3);
  Serial.printf("  Spreading Factor: %d\n", LORA_SF);
  Serial.printf("  Coding Rate: 4/%d\n", LORA_CR);
  Serial.printf("  Sync Word: 0x%02X\n", LORA_SYNC);
  Serial.printf("  TX Power: %d dBm\n", LORA_TX_POWER);
  Serial.printf("  Interval: %d ms\n", TX_INTERVAL);
  Serial.println("========================\n");

  // Init OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("LoRa Transmitter");
    display.printf("%.1f MHz SF%d\n", LORA_FREQ / 1E6, LORA_SF);
    display.println("Starting...");
    display.display();
  }

  // Init LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa init failed!");
    display.println("INIT FAILED!");
    display.display();
    while (1);
  }

  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setCodingRate4(LORA_CR);
  LoRa.setSyncWord(LORA_SYNC);
  LoRa.setPreambleLength(LORA_PREAMBLE);
  LoRa.setTxPower(LORA_TX_POWER);

  Serial.println("Transmitting...\n");
}

void loop() {
  packetCount++;
  
  // Build message
  String message = "Paket Nr. " + String(packetCount);
  
  // Transmit
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();
  
  // Serial output
  Serial.printf("TX Packet #%d: %s\n", packetCount, message.c_str());
  
  // OLED output
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("LoRa Transmitter");
  display.printf("%.1f MHz SF%d\n", LORA_FREQ / 1E6, LORA_SF);
  display.printf("Bandwidth: %d\n", LORA_BW);
  display.println("");
  display.printf("Sent: #%d\n", packetCount);
  display.println(message);
  display.display();
  
  delay(TX_INTERVAL);
}

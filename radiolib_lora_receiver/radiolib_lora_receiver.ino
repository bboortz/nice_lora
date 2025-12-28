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
#define LORA_SF 7
#define LORA_CR 5
#define LORA_SYNC 0x12
#define LORA_PREAMBLE 8

// Create SX1276 instance
SX1276 radio = new Module(LORA_CS, LORA_IRQ, LORA_RST, RADIOLIB_NC);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int packetCount = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  Serial.println("\n\n=== LORA RECEIVER (RadioLib) ===");
  Serial.println("Configuration:");
  Serial.printf("  Frequency: %.1f MHz\n", LORA_FREQ);
  Serial.printf("  Bandwidth: %.1f kHz\n", LORA_BW);
  Serial.printf("  Spreading Factor: %d\n", LORA_SF);
  Serial.printf("  Coding Rate: 4/%d\n", LORA_CR);
  Serial.printf("  Sync Word: 0x%02X\n", LORA_SYNC);
  Serial.println("================================\n");

  // Init OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("LoRa RX RadioLib");
    display.printf("%.1f MHz SF%d\n", LORA_FREQ, LORA_SF);
    display.println("Starting...");
    display.display();
  }

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

  Serial.println("Listening...\n");
  
  // Update display
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("LoRa RX RadioLib");
  display.printf("%.1f MHz SF%d\n", LORA_FREQ, LORA_SF);
  display.println("");
  display.println("Listening...");
  display.display();
}

void loop() {
  String data;
  
  int state = radio.receive(data);
  
  if (state == RADIOLIB_ERR_NONE) {
    packetCount++;
    
    float rssi = radio.getRSSI();
    float snr = radio.getSNR();
    float freqErr = radio.getFrequencyError();
    
    // Serial output
    Serial.printf("Packet #%d received!\n", packetCount);
    Serial.printf("  Data: %s\n", data.c_str());
    Serial.printf("  Length: %d bytes\n", data.length());
    Serial.printf("  RSSI: %.1f dBm\n", rssi);
    Serial.printf("  SNR: %.1f dB\n", snr);
    Serial.printf("  Freq Error: %.1f Hz\n\n", freqErr);
    
    // OLED output
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("LoRa RX RadioLib");
    display.printf("Pkt #%d\n", packetCount);
    display.printf("RSSI: %.0f dBm\n", rssi);
    display.printf("SNR: %.1f dB\n", snr);
    display.printf("Err: %.0f Hz\n", freqErr);
    display.println(data.substring(0, 21));
    display.display();
    
  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // Timeout - no packet, just keep listening
    
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    Serial.println("CRC error!");
    
  } else {
    Serial.printf("Receive failed, error: %d\n", state);
  }
}6

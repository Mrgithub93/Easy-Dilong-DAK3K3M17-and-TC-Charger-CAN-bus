
#include <mcp_can.h>
#include <SPI.h>

// Constants and variable definitions
#define SPI_CS_PIN 10 // CS Pin for MCP_CAN

word outputvoltage = 1570; // Set max voltage to 116.2V (offset = 0.1)
word outputcurrent = 200;  // Set max current to 32A (offset = 0.1)
unsigned long int sendId = 0x1806E5F4; // CAN ID to send messages

MCP_CAN CAN(SPI_CS_PIN); // Set CS pin for MCP_CAN

unsigned long previousMillis = 0; // will store last time a CAN message was sent
const long interval = 1000;       // interval at which to send CAN messages (milliseconds)

// Function to send CAN message
void sendCanMessage() {
  unsigned char data[8] = {
    highByte(outputvoltage), lowByte(outputvoltage),
    highByte(outputcurrent), lowByte(outputcurrent),
    0x00, 0x00, 0x00, 0x00
  };
  
  byte sndStat = CAN.sendMsgBuf(sendId, 1, 8, data);
  if(sndStat == CAN_OK) {
    Serial.println("CAN message sent successfully");
  } else {
    Serial.println("Error sending CAN message");
  }
}

void setup() {
  Serial.begin(115200);
  while(CAN_OK != CAN.begin(MCP_ANY, CAN_250KBPS, MCP_16MHZ)) { // Initialize CAN Bus at 250kbps
    Serial.println("CAN Initialization failed, retrying...");
    delay(100);
  }
  Serial.println("CAN initialization successful");
  delay(1000);
  int amp = 0;
  int volt = 0;
  outputvoltage = volt;
  outputcurrent = amp;
  sendCanMessage();
  delay(500);
  Serial.println("charger ON, type SET_VOLTAGE:x and then SET_CURRENT:x to configure charger.");
}


// Function to read CAN message and extract voltage and current values
void readCanMessage() {
    unsigned char len = 0;    // Length of the incoming message
    unsigned char buf[8];     // Data buffer to store the incoming message
    unsigned long rxId;       // Variable to store the ID of the received message
    byte ext = 0;             // Variable to store the "extended frame" flag (if required by your library version)

    // Check if a new message has been received
    if (CAN_MSGAVAIL == CAN.checkReceive()) {
      // Read the message into the buffer
      // If your library requires the extended frame flag, use the version of readMsgBuf() that includes it
      // Otherwise, you can omit the 'ext' variable in the function call
      CAN.readMsgBuf(&rxId, &len, buf); // Adjusted to match the library signature

      // Print the received message ID and data
      Serial.print("Received CAN ID: ");
      Serial.print(rxId, HEX);
      Serial.print(" Length: ");
      Serial.print(len);
      Serial.print(" Data: ");
      for (int i = 0; i < len; i++) {
        Serial.print(buf[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      // Extract voltage and current from the message
      float voltage = (((float)buf[0] * 256.0) + (float)buf[1]) / 10.0; // Assuming high byte first, then low byte, and 0.1V/bit
      float current = (((float)buf[2] * 256.0) + (float)buf[3]) / 10.0; // Assuming high byte first, then low byte, and 0.1A/bit

      // Print extracted voltage and current values
      Serial.print("Voltage: ");
      Serial.print(voltage);
      Serial.print("V, Current: ");
      Serial.print(current);
      Serial.println("A");
  }
}


void loop() {
  // Here we'll use millis() to manage the timing instead of SimpleTimer
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // Save the last time a message was sent
    previousMillis = currentMillis;
    delay(1000);
    readCanMessage();
  }

  // Check for incoming serial data
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n'); // Read the command from the serial port

    // Parse the command and set voltage or current
    if (command.startsWith("SET_VOLTAGE:")) {
      int voltage = command.substring(12).toInt();
      if(voltage >= 0 && voltage <= 1570) { // Check valid range
        outputvoltage = voltage;
        sendCanMessage(); // Update immediately
      }
    } 
    else if (command.startsWith("SET_CURRENT:")) {
      int current = command.substring(12).toInt();
      if(current >= 0 && current <= 250) { // Check valid range
        outputcurrent = current;
        sendCanMessage();// Update immediately
      }
    }
  }
}

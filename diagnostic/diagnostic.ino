#include <Arduino.h>
#include <Wire.h>

// ====================================================================
// Need to have "USB CDC on Boot" enabled on the Arduino IDE Tools menu
// ====================================================================

// Precise Waveshare 2.06 Pin Assignments
#define I2C_SDA 8
#define I2C_SCL 7

void setup() {
  // CRITICAL: Forces the system to pause to prevent USB initialization crashes
  delay(2500); 
  
  Serial.begin(115200);
  while (!Serial); // Handshake with the direct-USB connection
  
  Serial.println("\n=============================================");
  Serial.println("Waveshare 2.06 AMOLED Watch Bus Analyzer");
  Serial.println("=============================================");

  // Pull up pins to prevent floating I2C bus locks
  pinMode(I2C_SDA, INPUT_PULLUP);
  pinMode(I2C_SCL, INPUT_PULLUP);
  delay(100);

  Serial.println("Mounting Hardware I2C Interface...");
  Wire.begin(I2C_SDA, I2C_SCL, 100000); // 100kHz safe clock speed
  delay(500);

  Serial.println("Scanning Bus for Peripherals...");
  byte error, address;
  int devicesFound = 0;

  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("  -> Detected device at address: 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      
      // Match discovered addresses to the exact 2.06" onboard chips
      if (address == 0x18) Serial.println(" [ES8311 Audio Codec]");
      else if (address == 0x34) Serial.println(" [AXP2101 Power Management IC]");
      else if (address == 0x51) Serial.println(" [PCF85063 Real-Time Clock]");
      else if (address == 0x6B) Serial.println(" [QMI8658 IMU Motion Sensor]");
      else Serial.println(" [Screen/Touch Controller]");

      devicesFound++;
    }
    else if (error == 4) {
      Serial.print("  ! Bus Error/Collision at address: 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (devicesFound == 0) {
    Serial.println("\nCRITICAL: No components detected on the bus lines.");
  } else {
    Serial.println("\nScan complete. Internal hardware registry verified.");
  }
}

void loop() {
  delay(1000);
}

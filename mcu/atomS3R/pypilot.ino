#include <Arduino.h>
#include <pypilot_app.hpp>

void setup() {
    Serial.begin(115200);
    Serial.println("pypilot atomS3R: common app shell loaded; wire AtomS3R IMU and servo backends before enabling control.");
}

void loop() {
    delay(1000);
}

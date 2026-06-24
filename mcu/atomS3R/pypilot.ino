#include <Arduino.h>
#include <pypilot_app.hpp>

static pypilot::PypilotApp app;

void setup() {
    Serial.begin(115200);

    if (!app.begin(nullptr, nullptr)) {
        Serial.print("pypilot atomS3R: failed to start: ");
        Serial.println(app.status().fault);
        return;
    }

    Serial.print("pypilot atomS3R: runtime loop started on port ");
    Serial.println(app.status().runtime_port);
    Serial.println("Wire AtomS3R IMU and servo backends before enabling closed-loop control.");
}

void loop() {
    app.tick();
}

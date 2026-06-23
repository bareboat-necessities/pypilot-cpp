#include <Arduino.h>
#include <pypilot_app.hpp>

static pypilot::PypilotApp app;

void setup() {
    Serial.begin(115200);

    pypilot::PypilotAppConfig config;
    config.runtime_host = "0.0.0.0";
    config.runtime_port = 23322;
    config.control_period_us = 50000u;

    if (!app.begin(nullptr, nullptr, config)) {
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

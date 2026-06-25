#include <Arduino.h>
#include <M5Unified.h>
#include <pypilot_app.hpp>

static pypilot::PypilotApp app;

void setup() {
    auto cfg = M5.config();
    cfg.serial_baudrate = 115200;
    M5.begin(cfg);

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
    M5.update();
    app.tick();
}

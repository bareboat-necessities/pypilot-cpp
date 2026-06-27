#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifndef PYPILOT_SERVO_PROTOCOL_ENABLE_LINUX_SERIAL
#define PYPILOT_SERVO_PROTOCOL_ENABLE_LINUX_SERIAL 1
#endif

#include <pypilot_servo_protocol.hpp>

#include "pypilot_app.hpp"
#include "pypilot_servo_backend.hpp"

static constexpr const char* PYPILOT_SERVO_SERIAL_ENV = "PYPILOT_SERVO_SERIAL";
static constexpr int PYPILOT_SERVO_SERIAL_BAUD = 38400;

int main(int, char**) {
    const char* servo_serial = std::getenv(PYPILOT_SERVO_SERIAL_ENV);
    const bool servo_configured = servo_serial && servo_serial[0];

    pypilot_servo_protocol::LinuxSerialTransport servo_transport;
    pypilot_steering_signaling::ServoRuntimeConfig servo_config;
    servo_config.safety.enforce_rudder_limits = false;
    pypilot::PypilotServoBackend servo_backend(servo_transport, servo_config);

    pypilot::IServoBackend* servo_backend_ptr = nullptr;
    if (servo_configured) {
        if (!servo_transport.open_device(servo_serial, PYPILOT_SERVO_SERIAL_BAUD)) {
            std::fprintf(stderr, "pypilotd: failed to open servo serial port %s\n", servo_serial);
            return 1;
        }
        servo_backend.reset_protocol();
        servo_backend_ptr = &servo_backend;
    } else {
        std::fprintf(stderr, "pypilotd: servo serial disabled; set %s=/dev/tty... to enable servo communication.\n", PYPILOT_SERVO_SERIAL_ENV);
    }

    pypilot::PypilotApp app;
    if (!app.begin(nullptr, servo_backend_ptr)) {
        std::fprintf(stderr, "pypilotd: failed to start: %s\n", app.status().fault);
        return 1;
    }

    if (servo_configured) {
        app.data_model().servo.has_controller = true;
        app.data_model().servo_telemetry.controller_state.value = ship_data_model::ServoControllerState::idle;
        std::strncpy(app.data_model().servo_calibration.source,
                     servo_serial,
                     sizeof(app.data_model().servo_calibration.source) - 1);
        app.data_model().servo_calibration.source[sizeof(app.data_model().servo_calibration.source) - 1] = '\0';
    }

    std::fprintf(stderr,
                 "pypilotd: runtime server listening on port %u; servo=%s.\n",
                 static_cast<unsigned>(app.status().runtime_port),
                 servo_configured ? servo_serial : "disabled");
    app.run_forever();
    app.stop();
    return 0;
}

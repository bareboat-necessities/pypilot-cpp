#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef PYPILOT_SERVO_PROTOCOL_ENABLE_LINUX_SERIAL
#define PYPILOT_SERVO_PROTOCOL_ENABLE_LINUX_SERIAL 1
#endif

#include <pypilot_settings.hpp>
#include <pypilot_servo_protocol.hpp>

#include "pypilot_app.hpp"
#include "pypilot_servo_backend.hpp"

static constexpr const char* PYPILOT_SERVO_SERIAL_ENV = "PYPILOT_SERVO_SERIAL";
static constexpr int PYPILOT_SERVO_SERIAL_BAUD = 38400;

static bool ensure_directory(const char* path) {
    if (!path || !*path) return false;
    struct stat st;
    if (::stat(path, &st) == 0) return S_ISDIR(st.st_mode);
    return ::mkdir(path, 0755) == 0;
}

static bool apply_original_tcp_environment(pypilot_settings::SettingsManager& runtime_settings) {
    const char* port = std::getenv("PYPILOT_PORT");
    if (!port || !*port) return true;
    char error[160]{};
    return runtime_settings.save_value("runtime.tcp.port", port, error, sizeof(error));
}

static bool open_servo_serial(pypilot_servo_protocol::LinuxSerialTransport& transport, const char* path) {
    return path && path[0] && (transport.is_open() || transport.open_device(path, PYPILOT_SERVO_SERIAL_BAUD));
}

int main(int, char**) {
    char config_dir[256]{};
    if (!pypilot_settings::pypilot_default_config_dir(config_dir, sizeof(config_dir))) {
        std::fprintf(stderr, "pypilotd: failed to resolve pypilot config directory\n");
        return 1;
    }
    if (!ensure_directory(config_dir)) {
        std::fprintf(stderr, "pypilotd: failed to create/access config directory: %s\n", config_dir);
        return 1;
    }

    char config_file[512]{};
    if (!pypilot_settings::pypilot_config_file_path(config_dir, config_file, sizeof(config_file))) {
        std::fprintf(stderr, "pypilotd: failed to resolve pypilot config file\n");
        return 1;
    }

    pypilot_settings::PypilotConfigStore config_store(config_file);
    pypilot_settings::SettingsCatalog runtime_catalog = pypilot_runtime::PypilotRuntimeServiceSettings::catalog();
    pypilot_settings::SettingsManager runtime_settings(runtime_catalog, config_store);
    if (!apply_original_tcp_environment(runtime_settings)) {
        std::fprintf(stderr, "pypilotd: failed to apply PYPILOT_PORT\n");
        return 1;
    }
    if (!pypilot_runtime::PypilotRuntimeServiceSettings::apply_process_environment(runtime_settings)) {
        std::fprintf(stderr, "pypilotd: failed to apply runtime environment settings\n");
        return 1;
    }

    const char* servo_serial = std::getenv(PYPILOT_SERVO_SERIAL_ENV);
    const bool servo_configured = servo_serial && servo_serial[0];

    pypilot_servo_protocol::LinuxSerialTransport servo_transport;
    pypilot_steering_signaling::ServoRuntimeConfig servo_config;
    servo_config.safety.enforce_rudder_limits = false;
    pypilot::PypilotServoBackend servo_backend(servo_transport, servo_config);

    pypilot::PypilotApp app;
    pypilot::IServoBackend* servo_backend_ptr = servo_configured ? static_cast<pypilot::IServoBackend*>(&servo_backend) : nullptr;

    if (!app.begin(nullptr, servo_backend_ptr, nullptr, 0, nullptr, &runtime_settings)) {
        std::fprintf(stderr, "pypilotd: failed to start: %s\n", app.status().fault);
        std::fflush(stderr);
        return 1;
    }

    if (servo_configured) {
        app.loop().on_repeat_us(1000000u, [&]() {
            if (open_servo_serial(servo_transport, servo_serial)) {
                servo_backend.reset_protocol();
                app.data_model().servo.has_controller = true;
                app.data_model().servo_telemetry.controller_state.value = pypilot_data_model::ServoControllerState::idle;
                pypilot_data_model::copy_data_text(app.data_model().servo_calibration.source,
                                                   sizeof(app.data_model().servo_calibration.source),
                                                   servo_serial);
            }
        });
    } else {
        std::fprintf(stderr, "pypilotd: servo serial disabled; set %s=/dev/tty... to enable servo communication.\n", PYPILOT_SERVO_SERIAL_ENV);
    }

    std::fprintf(stderr,
                 "pypilotd: runtime server listening on port %u; config=%s; servo=%s.\n",
                 static_cast<unsigned>(app.status().runtime_port),
                 config_file,
                 servo_configured ? servo_serial : "disabled");
    std::fflush(stderr);
    app.run_forever();
    app.stop();
    return 0;
}

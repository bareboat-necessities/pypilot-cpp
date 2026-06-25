#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>

#include <pypilot_settings.hpp>

#include "pypilot_app.hpp"
#include "pypilot_linux_servo_backend.hpp"

static bool ensure_directory(const char* path) {
    if (!path || !*path) return false;
    struct stat st;
    if (::stat(path, &st) == 0) return S_ISDIR(st.st_mode);
    return ::mkdir(path, 0755) == 0;
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
    if (!pypilot_runtime::PypilotRuntimeServiceSettings::apply_process_environment(runtime_settings)) {
        std::fprintf(stderr, "pypilotd: failed to apply runtime environment settings\n");
        return 1;
    }

    pypilot::PypilotLinuxServoBackend servo_backend(config_dir);
    pypilot::PypilotApp app;

    if (!app.begin(nullptr, &servo_backend, nullptr, 0, nullptr, &runtime_settings)) {
        std::fprintf(stderr, "pypilotd: failed to start: %s\n", app.status().fault);
        std::fflush(stderr);
        return 1;
    }

    std::fprintf(stderr,
                 "pypilotd: runtime server listening on port %u; config=%s; servo serial probing uses %s/{serial_ports,blacklist_serial_ports,servodevice}.\n",
                 static_cast<unsigned>(app.status().runtime_port),
                 config_file,
                 config_dir);
    std::fflush(stderr);
    app.run_forever();
    app.stop();
    return 0;
}

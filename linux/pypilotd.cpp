#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef PYPILOT_SERVO_PROTOCOL_ENABLE_LINUX_SERIAL
#define PYPILOT_SERVO_PROTOCOL_ENABLE_LINUX_SERIAL 1
#endif

#include <pypilot_settings.hpp>
#include <pypilot_servo_protocol.hpp>

#include "pypilot_app.hpp"
#include "pypilot_servo_backend.hpp"
#include "pypilot_servo_discovery.hpp"

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

static std::string real_path(const char* path) {
    if (!path) return std::string();
    char resolved[512]{};
    if (::realpath(path, resolved)) return std::string(resolved);
    return std::string(path);
}

static bool candidate_blacklisted(const char* path, const pypilot_settings::PypilotServoSerialCandidateList<64>& blacklist) {
    const std::string real = real_path(path);
    for (size_t i = 0; i < blacklist.count(); ++i) {
        if (real_path(blacklist.at(i).path) == real) return true;
    }
    return false;
}

static void add_candidate_if_allowed(pypilot::PypilotServoCandidateList<32>& out,
                                     const char* path,
                                     int baud,
                                     bool preferred,
                                     const pypilot_settings::PypilotServoSerialCandidateList<64>& blacklist) {
    if (!path || !path[0] || candidate_blacklisted(path, blacklist)) return;
    out.add(path, baud, preferred);
}

static void add_directory_candidates(pypilot::PypilotServoCandidateList<32>& out,
                                     const char* directory,
                                     const pypilot_settings::PypilotServoSerialCandidateList<64>& blacklist) {
    DIR* dir = ::opendir(directory);
    if (!dir) return;
    while (dirent* ent = ::readdir(dir)) {
        if (ent->d_name[0] == '.') continue;
        std::string path = std::string(directory) + "/" + ent->d_name;
        add_candidate_if_allowed(out, path.c_str(), 38400, false, blacklist);
    }
    ::closedir(dir);
}

static void add_dev_prefix_candidates(pypilot::PypilotServoCandidateList<32>& out,
                                      const pypilot_settings::PypilotServoSerialCandidateList<64>& blacklist) {
    DIR* dir = ::opendir("/dev");
    if (!dir) return;
    while (dirent* ent = ::readdir(dir)) {
        const char* name = ent->d_name;
        if (std::strncmp(name, "ttyUSB", 6) == 0 ||
            std::strncmp(name, "ttyACM", 6) == 0 ||
            std::strncmp(name, "ttyAMA", 6) == 0) {
            std::string path = std::string("/dev/") + name;
            add_candidate_if_allowed(out, path.c_str(), 38400, false, blacklist);
        }
    }
    ::closedir(dir);
}

static void build_servo_candidates(const char* config_dir, pypilot::PypilotServoCandidateList<32>& out) {
    out.clear();

    pypilot_settings::PypilotServoSerialCandidateList<64> blacklist;
    pypilot_settings::pypilot_load_blacklist_serial_ports_file(config_dir, blacklist);

    pypilot_settings::PypilotServoSerialCandidateList<64> last;
    pypilot_settings::pypilot_load_servo_device_file(config_dir, last);
    for (size_t i = 0; i < last.count(); ++i) {
        add_candidate_if_allowed(out, last.at(i).path, last.at(i).baud, true, blacklist);
    }

    pypilot_settings::PypilotServoSerialCandidateList<64> allowed;
    const bool has_allowed = pypilot_settings::pypilot_load_serial_ports_file(config_dir, allowed);
    const bool any = !has_allowed || allowed.count() == 0 ||
                     (allowed.count() == 1 && std::strcmp(allowed.at(0).path, "any") == 0);
    if (any) {
        add_directory_candidates(out, "/dev/serial/by-id", blacklist);
        add_directory_candidates(out, "/dev/serial/by-path", blacklist);
        add_dev_prefix_candidates(out, blacklist);
    } else {
        for (size_t i = 0; i < allowed.count(); ++i) {
            add_candidate_if_allowed(out, allowed.at(i).path, allowed.at(i).baud, false, blacklist);
        }
    }
}

class LinuxServoPortOpener final : public pypilot::IPypilotServoPortOpener {
public:
    LinuxServoPortOpener(pypilot_servo_protocol::LinuxSerialTransport& transport, const char* config_dir)
        : transport_(transport), config_dir_(config_dir ? config_dir : ""), current_path_(), current_baud_(0) {}

    bool servo_port_is_open() const override { return transport_.is_open(); }

    bool open_servo_port(const pypilot::PypilotServoPortCandidate& candidate) override {
        if (!candidate.path || !candidate.path[0]) return false;
        if (!transport_.open_device(candidate.path, candidate.baud)) return false;
        current_path_ = candidate.path;
        current_baud_ = candidate.baud;
        pypilot_settings::pypilot_save_servo_device_file(config_dir_.c_str(), candidate.path, candidate.baud);
        return true;
    }

    const char* current_path() const { return current_path_.c_str(); }

private:
    pypilot_servo_protocol::LinuxSerialTransport& transport_;
    std::string config_dir_;
    std::string current_path_;
    int current_baud_;
};

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

    pypilot_servo_protocol::LinuxSerialTransport servo_transport;
    LinuxServoPortOpener servo_opener(servo_transport, config_dir);
    pypilot::PypilotServoDiscovery servo_discovery(servo_opener);
    pypilot::PypilotServoCandidateList<32> servo_candidates;
    pypilot_steering_signaling::ServoRuntimeConfig servo_config;
    servo_config.safety.enforce_rudder_limits = false;
    pypilot::PypilotServoBackend servo_backend(servo_transport, servo_config);

    pypilot::PypilotApp app;

    if (!app.begin(nullptr, &servo_backend, nullptr, 0, nullptr, &runtime_settings)) {
        std::fprintf(stderr, "pypilotd: failed to start: %s\n", app.status().fault);
        std::fflush(stderr);
        return 1;
    }

    app.loop().on_repeat_us(1000000u, [&]() {
        const uint64_t now_us = app.loop().clock().micros();
        build_servo_candidates(config_dir, servo_candidates);
        if (!servo_transport.is_open() && servo_discovery.ensure_open(servo_candidates.data(), servo_candidates.count(), now_us)) {
            servo_backend.reset_protocol();
            app.data_model().servo.has_controller = true;
            app.data_model().servo_telemetry.controller_state.value = pypilot_data_model::ServoControllerState::idle;
            pypilot_data_model::copy_data_text(app.data_model().servo_calibration.source,
                                               sizeof(app.data_model().servo_calibration.source),
                                               servo_opener.current_path());
        }
    });

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

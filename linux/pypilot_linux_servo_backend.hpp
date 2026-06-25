#pragma once

#ifndef PYPILOT_SERVO_PROTOCOL_ENABLE_LINUX_SERIAL
#define PYPILOT_SERVO_PROTOCOL_ENABLE_LINUX_SERIAL 1
#endif

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include <pypilot_settings.hpp>
#include <pypilot_servo_protocol.hpp>
#include <pypilot_steering_signaling.hpp>

#include "pypilot_app.hpp"

namespace pypilot {

class PypilotLinuxServoBackend final : public IServoBackend {
public:
    explicit PypilotLinuxServoBackend(const char* config_dir)
        : config_dir_(config_dir ? config_dir : ""),
          transport_(),
          host_(transport_),
          telemetry_(),
          servo_runtime_(),
          open_(false),
          last_probe_us_(0),
          packet_count_(0),
          bad_packet_count_(0) {}

    void disable(PypilotDataModel& model, uint64_t now_us) override {
        poll_telemetry(model, now_us);
        if (open_) {
            host_.neutral();
            host_.disengage();
        }
        model.servo.engaged.value = false;
        model.servo_telemetry.controller_state.value = open_
            ? pypilot_data_model::ServoControllerState::idle
            : pypilot_data_model::ServoControllerState::disconnected;
    }

    bool apply(PypilotDataModel& model, uint64_t now_us) override {
        poll_telemetry(model, now_us);
        if (!ensure_open(model, now_us)) {
            model.servo_telemetry.controller_state.value = pypilot_data_model::ServoControllerState::disconnected;
            return false;
        }

        const pypilot_steering_signaling::ServoCommand command = command_from_model(model, now_us);
        const pypilot_steering_signaling::RudderAngle rudder = rudder_from_model(model, now_us);
        const pypilot_steering_signaling::ServoRuntimeOutput output =
            servo_runtime_.update_command(command, rudder, now_us);

        if (output.emit) {
            if (!transport_.write_bytes(output.protocol.raw_packet.bytes, sizeof(output.protocol.raw_packet.bytes))) {
                close_device(model, now_us);
                return false;
            }
            model.servo_telemetry.last_command_us = now_us;
            model.servo_telemetry.command_norm.set(command.value, now_us);
            model.servo_telemetry.raw_command_norm.set(output.protocol.allowed ? command.value : 0.0f, now_us);
        }

        model.servo.engaged.value = true;
        if (model.servo_telemetry.controller_state.value == pypilot_data_model::ServoControllerState::unknown ||
            model.servo_telemetry.controller_state.value == pypilot_data_model::ServoControllerState::disconnected) {
            model.servo_telemetry.controller_state.value = pypilot_data_model::ServoControllerState::moving;
        }
        return true;
    }

private:
    static constexpr uint64_t ProbeIntervalUs = 5000000ULL;
    static constexpr uint64_t ManualOverrideUs = 1000000ULL;
    static constexpr uint64_t PilotOutputTimeoutUs = 1000000ULL;

    struct SerialCandidate {
        std::string path;
        int baud;
        bool from_last_working;
    };

    static bool read_lines(const std::string& path, std::vector<std::string>& lines) {
        std::ifstream input(path);
        if (!input.good()) return false;
        std::string line;
        while (std::getline(input, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
            size_t first = 0;
            while (first < line.size() && (line[first] == ' ' || line[first] == '\t')) ++first;
            if (first != 0) line.erase(0, first);
            if (!line.empty()) lines.push_back(line);
        }
        return true;
    }

    static bool path_exists(const std::string& path) {
        struct stat st;
        return ::stat(path.c_str(), &st) == 0;
    }

    static std::string real_path(const std::string& path) {
        char resolved[512]{};
        if (::realpath(path.c_str(), resolved)) return std::string(resolved);
        return path;
    }

    std::string config_path(const char* filename) const {
        char out[512]{};
        if (!pypilot_settings::settings_append_path(out, sizeof(out), config_dir_.c_str(), filename)) return std::string();
        return std::string(out);
    }

    bool parse_last_working(SerialCandidate& out) const {
        std::ifstream input(config_path("servodevice"));
        if (!input.good()) return false;
        std::string text;
        std::getline(input, text);
        const size_t first_quote = text.find('"');
        if (first_quote == std::string::npos) return false;
        const size_t second_quote = text.find('"', first_quote + 1);
        if (second_quote == std::string::npos) return false;
        const size_t comma = text.find(',', second_quote + 1);
        if (comma == std::string::npos) return false;
        out.path = text.substr(first_quote + 1, second_quote - first_quote - 1);
        out.baud = std::atoi(text.c_str() + comma + 1);
        if (out.baud <= 0) out.baud = 38400;
        out.from_last_working = true;
        return !out.path.empty();
    }

    void save_last_working(const SerialCandidate& candidate) const {
        std::ofstream output(config_path("servodevice"), std::ios::trunc);
        if (!output.good()) return;
        output << "[\"" << candidate.path << "\"," << candidate.baud << "]\n";
    }

    bool allowed_any(std::vector<std::string>& allowed) const {
        if (!read_lines(config_path("serial_ports"), allowed)) return true;
        if (allowed.empty()) return true;
        return allowed.size() == 1 && allowed[0] == "any";
    }

    bool blacklisted(const std::string& path, const std::vector<std::string>& blacklist) const {
        const std::string real = real_path(path);
        for (const std::string& b : blacklist) {
            if (real_path(b) == real) return true;
        }
        return false;
    }

    void add_candidate(std::vector<SerialCandidate>& candidates,
                       const std::string& path,
                       int baud,
                       bool last,
                       const std::vector<std::string>& blacklist) const {
        if (path.empty() || blacklisted(path, blacklist)) return;
        for (const SerialCandidate& c : candidates) {
            if (real_path(c.path) == real_path(path) && c.baud == baud) return;
        }
        SerialCandidate candidate;
        candidate.path = path;
        candidate.baud = baud > 0 ? baud : 38400;
        candidate.from_last_working = last;
        candidates.push_back(candidate);
    }

    void add_directory_candidates(std::vector<SerialCandidate>& candidates,
                                  const std::string& directory,
                                  const std::vector<std::string>& blacklist) const {
        DIR* dir = ::opendir(directory.c_str());
        if (!dir) return;
        while (dirent* ent = ::readdir(dir)) {
            if (ent->d_name[0] == '.') continue;
            add_candidate(candidates, directory + "/" + ent->d_name, 38400, false, blacklist);
        }
        ::closedir(dir);
    }

    void add_dev_prefix_candidates(std::vector<SerialCandidate>& candidates,
                                   const std::vector<std::string>& blacklist) const {
        DIR* dir = ::opendir("/dev");
        if (!dir) return;
        while (dirent* ent = ::readdir(dir)) {
            const char* name = ent->d_name;
            if (std::strncmp(name, "ttyUSB", 6) == 0 ||
                std::strncmp(name, "ttyACM", 6) == 0 ||
                std::strncmp(name, "ttyAMA", 6) == 0) {
                add_candidate(candidates, std::string("/dev/") + name, 38400, false, blacklist);
            }
        }
        ::closedir(dir);
    }

    std::vector<SerialCandidate> build_candidates() const {
        std::vector<std::string> blacklist;
        read_lines(config_path("blacklist_serial_ports"), blacklist);

        std::vector<SerialCandidate> candidates;
        SerialCandidate last;
        if (parse_last_working(last)) add_candidate(candidates, last.path, last.baud, true, blacklist);

        std::vector<std::string> allowed;
        if (allowed_any(allowed)) {
            add_directory_candidates(candidates, "/dev/serial/by-id", blacklist);
            add_directory_candidates(candidates, "/dev/serial/by-path", blacklist);
            add_dev_prefix_candidates(candidates, blacklist);
        } else {
            for (const std::string& path : allowed) add_candidate(candidates, path, 38400, false, blacklist);
        }
        return candidates;
    }

    bool ensure_open(PypilotDataModel& model, uint64_t now_us) {
        if (open_ && transport_.is_open()) return true;
        if (now_us >= last_probe_us_ && now_us - last_probe_us_ < ProbeIntervalUs) return false;
        last_probe_us_ = now_us;

        const std::vector<SerialCandidate> candidates = build_candidates();
        for (const SerialCandidate& candidate : candidates) {
            if (transport_.open_device(candidate.path.c_str(), candidate.baud)) {
                open_ = true;
                current_device_ = candidate.path;
                current_baud_ = candidate.baud;
                telemetry_.reset();
                host_.force_unsync();
                host_.reset_faults();
                save_last_working(candidate);
                model.servo.has_controller = true;
                model.servo_telemetry.controller_state.value = pypilot_data_model::ServoControllerState::idle;
                pypilot_data_model::copy_data_text(model.servo_calibration.source, sizeof(model.servo_calibration.source), candidate.path.c_str());
                return true;
            }
        }
        model.servo.has_controller = false;
        return false;
    }

    void close_device(PypilotDataModel& model, uint64_t now_us) {
        (void)now_us;
        transport_.close_device();
        open_ = false;
        current_device_.clear();
        current_baud_ = 0;
        model.servo.has_controller = false;
        model.servo.engaged.value = false;
        model.servo_telemetry.controller_state.value = pypilot_data_model::ServoControllerState::disconnected;
    }

    pypilot_steering_signaling::ServoCommand command_from_model(const PypilotDataModel& model, uint64_t now_us) const {
        if (model.servo.command_norm.valid &&
            now_us >= model.servo.command_norm.last_external_set_us &&
            now_us - model.servo.command_norm.last_external_set_us <= ManualOverrideUs) {
            return pypilot_steering_signaling::ServoCommand(
                pypilot_steering_signaling::ServoCommandMode::Speed,
                model.servo.command_norm.value,
                now_us,
                true);
        }
        if (model.pilot_output.command_norm.valid && !model.pilot_output.command_norm.stale(now_us, PilotOutputTimeoutUs)) {
            return pypilot_steering_signaling::ServoCommand(
                pypilot_steering_signaling::ServoCommandMode::Speed,
                model.pilot_output.command_norm.value,
                now_us,
                true);
        }
        return pypilot_steering_signaling::servo_stop_command(now_us);
    }

    pypilot_steering_signaling::RudderAngle rudder_from_model(const PypilotDataModel& model, uint64_t now_us) const {
        if (model.rudder.angle_deg.valid) {
            return pypilot_steering_signaling::RudderAngle(model.rudder.angle_deg.value, model.rudder.angle_deg.last_update_us, true);
        }
        if (model.servo.position_deg.valid) {
            return pypilot_steering_signaling::RudderAngle(model.servo.position_deg.value, model.servo.position_deg.last_update_us, true);
        }
        return pypilot_steering_signaling::RudderAngle(0.0f, now_us, false);
    }

    void poll_telemetry(PypilotDataModel& model, uint64_t now_us) {
        if (!open_ || !transport_.is_open()) return;
        bool any = false;
        for (unsigned i = 0; i < 32; ++i) {
            if (!host_.poll(telemetry_)) break;
            any = true;
            ++packet_count_;
        }
        if (!any) return;

        model.servo_telemetry.packet_count.set(packet_count_, now_us);
        model.servo_telemetry.last_telemetry_us = now_us;
        servo_runtime_.update_feedback(telemetry_, now_us);

        if (telemetry_.has_current) {
            model.servo.current_a.set(telemetry_.current_a, now_us);
            model.servo_telemetry.current_a.set(telemetry_.current_a, now_us);
        }
        if (telemetry_.has_voltage) {
            model.servo.voltage_v.set(telemetry_.voltage_v, now_us);
            model.servo_telemetry.voltage_v.set(telemetry_.voltage_v, now_us);
        }
        if (telemetry_.has_controller_temp) {
            model.servo.controller_temp_c.set(telemetry_.controller_temp_c, now_us);
            model.servo_telemetry.controller_temp_c.set(telemetry_.controller_temp_c, now_us);
        }
        if (telemetry_.has_motor_temp) {
            model.servo.motor_temp_c.set(telemetry_.motor_temp_c, now_us);
            model.servo_telemetry.motor_temp_c.set(telemetry_.motor_temp_c, now_us);
        }
        if (telemetry_.has_flags) {
            model.servo.flags.value = telemetry_.flags;
            model.servo_telemetry.flags.value = telemetry_.flags;
            model.servo.engaged.value = telemetry_.engaged();
            model.servo_telemetry.controller_state.value = telemetry_.faulted()
                ? pypilot_data_model::ServoControllerState::fault
                : telemetry_.engaged() ? pypilot_data_model::ServoControllerState::engaged
                                       : pypilot_data_model::ServoControllerState::idle;
        }
        if (telemetry_.has_rudder && telemetry_.rudder_valid) {
            const float range = model.rudder.range_deg.value != 0.0f ? model.rudder.range_deg.value : 45.0f;
            const float angle = telemetry_.rudder * 2.0f * range;
            model.rudder.angle_deg.set(angle, now_us);
            model.servo.position_deg.set(angle, now_us);
            model.servo_telemetry.rudder_feedback_deg.set(angle, now_us);
        }
        if (telemetry_.has_flags && telemetry_.faulted()) {
            ++model.servo.faults.value;
            ++model.servo_telemetry.faults.value;
        }
    }

    std::string config_dir_;
    pypilot_servo_protocol::LinuxSerialTransport transport_;
    pypilot_servo_protocol::Host host_;
    pypilot_servo_protocol::Telemetry telemetry_;
    pypilot_steering_signaling::ServoRuntime servo_runtime_;
    bool open_;
    uint64_t last_probe_us_;
    std::string current_device_;
    int current_baud_ = 0;
    uint32_t packet_count_;
    uint32_t bad_packet_count_;
};

} // namespace pypilot

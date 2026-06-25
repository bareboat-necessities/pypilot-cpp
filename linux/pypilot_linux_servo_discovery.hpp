#pragma once

#ifndef PYPILOT_SERVO_PROTOCOL_ENABLE_LINUX_SERIAL
#define PYPILOT_SERVO_PROTOCOL_ENABLE_LINUX_SERIAL 1
#endif

#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include <pypilot_settings.hpp>
#include <pypilot_servo_protocol.hpp>

namespace pypilot {

class PypilotLinuxServoDiscovery final {
public:
    explicit PypilotLinuxServoDiscovery(const char* config_dir)
        : config_dir_(config_dir ? config_dir : ""), last_probe_us_(0) {}

    const char* current_device() const { return current_device_.c_str(); }
    int current_baud() const { return current_baud_; }

    bool ensure_open(pypilot_servo_protocol::LinuxSerialTransport& transport, uint64_t now_us) {
        if (transport.is_open()) return true;
        if (now_us >= last_probe_us_ && now_us - last_probe_us_ < ProbeIntervalUs) return false;
        last_probe_us_ = now_us;

        const std::vector<SerialCandidate> candidates = build_candidates();
        for (const SerialCandidate& candidate : candidates) {
            if (transport.open_device(candidate.path.c_str(), candidate.baud)) {
                current_device_ = candidate.path;
                current_baud_ = candidate.baud;
                save_last_working(candidate);
                return true;
            }
        }
        current_device_.clear();
        current_baud_ = 0;
        return false;
    }

private:
    static constexpr uint64_t ProbeIntervalUs = 5000000ULL;

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

    std::string config_dir_;
    uint64_t last_probe_us_;
    std::string current_device_;
    int current_baud_ = 0;
};

} // namespace pypilot

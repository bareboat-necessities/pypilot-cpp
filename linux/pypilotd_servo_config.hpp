#pragma once

#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include <pypilot_settings.hpp>

#include "pypilot_servo_discovery.hpp"

namespace pypilotd_servo_config {

static inline std::string path_join(const char* config_dir, const char* filename) {
    char out[512]{};
    if (!pypilot_settings::settings_append_path(out, sizeof(out), config_dir, filename)) return std::string();
    return std::string(out);
}

static inline bool read_lines(const std::string& path, std::vector<std::string>& lines) {
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

static inline std::string real_path(const std::string& path) {
    char resolved[512]{};
    if (::realpath(path.c_str(), resolved)) return std::string(resolved);
    return path;
}

static inline bool blacklisted(const std::string& path, const std::vector<std::string>& blacklist) {
    const std::string real = real_path(path);
    for (const std::string& b : blacklist) {
        if (real_path(b) == real) return true;
    }
    return false;
}

template<size_t MaxCandidates, size_t MaxPathLength>
static inline void add_candidate(pypilot::PypilotServoCandidateList<MaxCandidates, MaxPathLength>& out,
                                 const char* path,
                                 int baud,
                                 bool preferred,
                                 const std::vector<std::string>& blacklist) {
    if (!path || !path[0] || blacklisted(path, blacklist)) return;
    out.add(path, baud, preferred);
}

template<size_t MaxCandidates, size_t MaxPathLength>
static inline void add_last_working(const char* config_dir,
                                    pypilot::PypilotServoCandidateList<MaxCandidates, MaxPathLength>& out,
                                    const std::vector<std::string>& blacklist) {
    std::ifstream input(path_join(config_dir, "servodevice"));
    if (!input.good()) return;
    std::string text;
    std::getline(input, text);
    const size_t first_quote = text.find('"');
    if (first_quote == std::string::npos) return;
    const size_t second_quote = text.find('"', first_quote + 1);
    if (second_quote == std::string::npos) return;
    const size_t comma = text.find(',', second_quote + 1);
    if (comma == std::string::npos) return;
    const std::string path = text.substr(first_quote + 1, second_quote - first_quote - 1);
    int baud = std::atoi(text.c_str() + comma + 1);
    if (baud <= 0) baud = 38400;
    add_candidate(out, path.c_str(), baud, true, blacklist);
}

template<size_t MaxCandidates, size_t MaxPathLength>
static inline void add_directory(const char* directory,
                                 pypilot::PypilotServoCandidateList<MaxCandidates, MaxPathLength>& out,
                                 const std::vector<std::string>& blacklist) {
    DIR* dir = ::opendir(directory);
    if (!dir) return;
    while (dirent* ent = ::readdir(dir)) {
        if (ent->d_name[0] == '.') continue;
        const std::string path = std::string(directory) + "/" + ent->d_name;
        add_candidate(out, path.c_str(), 38400, false, blacklist);
    }
    ::closedir(dir);
}

template<size_t MaxCandidates, size_t MaxPathLength>
static inline void add_dev_prefixes(pypilot::PypilotServoCandidateList<MaxCandidates, MaxPathLength>& out,
                                    const std::vector<std::string>& blacklist) {
    DIR* dir = ::opendir("/dev");
    if (!dir) return;
    while (dirent* ent = ::readdir(dir)) {
        const char* name = ent->d_name;
        if (std::strncmp(name, "ttyUSB", 6) == 0 ||
            std::strncmp(name, "ttyACM", 6) == 0 ||
            std::strncmp(name, "ttyAMA", 6) == 0) {
            const std::string path = std::string("/dev/") + name;
            add_candidate(out, path.c_str(), 38400, false, blacklist);
        }
    }
    ::closedir(dir);
}

template<size_t MaxCandidates, size_t MaxPathLength>
static inline void build_candidates(const char* config_dir,
                                    pypilot::PypilotServoCandidateList<MaxCandidates, MaxPathLength>& out) {
    out.clear();
    std::vector<std::string> blacklist;
    read_lines(path_join(config_dir, "blacklist_serial_ports"), blacklist);
    add_last_working(config_dir, out, blacklist);

    std::vector<std::string> allowed;
    if (!read_lines(path_join(config_dir, "serial_ports"), allowed) || allowed.empty() ||
        (allowed.size() == 1 && allowed[0] == "any")) {
        add_directory("/dev/serial/by-id", out, blacklist);
        add_directory("/dev/serial/by-path", out, blacklist);
        add_dev_prefixes(out, blacklist);
    } else {
        for (const std::string& path : allowed) add_candidate(out, path.c_str(), 38400, false, blacklist);
    }
}

static inline void save_last_working(const char* config_dir, const pypilot::PypilotServoPortCandidate& candidate) {
    std::ofstream output(path_join(config_dir, "servodevice"), std::ios::trunc);
    if (!output.good()) return;
    output << "[\"" << (candidate.path ? candidate.path : "") << "\"," << candidate.baud << "]\n";
}

} // namespace pypilotd_servo_config

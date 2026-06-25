#pragma once

#include <stddef.h>
#include <stdint.h>

namespace pypilot {

struct PypilotServoPortCandidate {
    const char* path;
    int baud;
    bool preferred;

    PypilotServoPortCandidate()
        : path(nullptr), baud(38400), preferred(false) {}

    PypilotServoPortCandidate(const char* candidate_path, int candidate_baud, bool is_preferred = false)
        : path(candidate_path), baud(candidate_baud > 0 ? candidate_baud : 38400), preferred(is_preferred) {}
};

class IPypilotServoPortOpener {
public:
    virtual ~IPypilotServoPortOpener() {}
    virtual bool servo_port_is_open() const = 0;
    virtual bool open_servo_port(const PypilotServoPortCandidate& candidate) = 0;
};

template<size_t MaxCandidates, size_t MaxPathLength = 160>
class PypilotServoCandidateList final {
public:
    PypilotServoCandidateList() : count_(0) {}

    void clear() { count_ = 0; }
    size_t count() const { return count_; }
    const PypilotServoPortCandidate* data() const { return candidates_; }
    const PypilotServoPortCandidate& at(size_t index) const { return candidates_[index]; }

    bool add(const char* path, int baud = 38400, bool preferred = false) {
        if (!path || !path[0] || count_ >= MaxCandidates) return false;
        for (size_t i = 0; i < count_; ++i) {
            if (same_text(paths_[i], path) && candidates_[i].baud == (baud > 0 ? baud : 38400)) return true;
        }
        if (!copy_text(paths_[count_], MaxPathLength, path)) return false;
        candidates_[count_] = PypilotServoPortCandidate(paths_[count_], baud, preferred);
        ++count_;
        return true;
    }

private:
    static bool same_text(const char* a, const char* b) {
        if (!a || !b) return a == b;
        while (*a && *b) {
            if (*a++ != *b++) return false;
        }
        return *a == *b;
    }

    static bool copy_text(char* dst, size_t dst_size, const char* src) {
        if (!dst || dst_size == 0) return false;
        if (!src) src = "";
        size_t i = 0;
        for (; src[i] && i + 1 < dst_size; ++i) dst[i] = src[i];
        dst[i] = '\0';
        return src[i] == '\0';
    }

    size_t count_;
    char paths_[MaxCandidates][MaxPathLength]{};
    PypilotServoPortCandidate candidates_[MaxCandidates]{};
};

class PypilotServoDiscovery final {
public:
    explicit PypilotServoDiscovery(IPypilotServoPortOpener& opener)
        : opener_(opener), retry_interval_us_(5000000ULL), last_probe_us_(0), current_() {}

    void set_retry_interval_us(uint64_t retry_interval_us) { retry_interval_us_ = retry_interval_us; }
    uint64_t retry_interval_us() const { return retry_interval_us_; }

    const PypilotServoPortCandidate& current_candidate() const { return current_; }
    const char* current_path() const { return current_.path ? current_.path : ""; }
    int current_baud() const { return current_.baud; }

    bool ensure_open(const PypilotServoPortCandidate* candidates, size_t count, uint64_t now_us) {
        if (opener_.servo_port_is_open()) return true;
        if (now_us >= last_probe_us_ && now_us - last_probe_us_ < retry_interval_us_) return false;
        last_probe_us_ = now_us;
        current_ = PypilotServoPortCandidate();
        if (!candidates || count == 0) return false;

        for (size_t pass = 0; pass < 2; ++pass) {
            for (size_t i = 0; i < count; ++i) {
                const PypilotServoPortCandidate& candidate = candidates[i];
                if (!candidate.path || !candidate.path[0]) continue;
                if ((pass == 0) != candidate.preferred) continue;
                if (opener_.open_servo_port(candidate)) {
                    current_ = candidate;
                    return true;
                }
            }
        }
        return false;
    }

private:
    IPypilotServoPortOpener& opener_;
    uint64_t retry_interval_us_;
    uint64_t last_probe_us_;
    PypilotServoPortCandidate current_;
};

} // namespace pypilot

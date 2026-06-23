#pragma once

#include <stdint.h>

namespace pypilot {

struct BoatImuSample {
    uint64_t timestamp_us;
    float heading_deg;
    float heading_rate_deg_s;
    bool valid;

    BoatImuSample()
        : timestamp_us(0), heading_deg(0.0f), heading_rate_deg_s(0.0f), valid(false) {}
};

class IBoatImuBackend {
public:
    virtual ~IBoatImuBackend() {}
    virtual bool read_sample(BoatImuSample& out) = 0;
};

class IServoBackend {
public:
    virtual ~IServoBackend() {}
    virtual void disable(uint64_t now_us) = 0;
    virtual bool write_normalized(float command, uint64_t now_us) = 0;
};

struct PypilotAppConfig {
    uint32_t loop_period_us;
    float max_abs_command;

    PypilotAppConfig()
        : loop_period_us(50000u), max_abs_command(1.0f) {}
};

enum class PypilotAppState : uint8_t {
    stopped,
    disabled,
    enabled,
    fault
};

struct PypilotAppStatus {
    PypilotAppState state;
    uint64_t last_tick_us;
    uint32_t ticks;
    bool imu_seen;
    bool servo_seen;
    const char* fault;

    PypilotAppStatus()
        : state(PypilotAppState::stopped),
          last_tick_us(0),
          ticks(0),
          imu_seen(false),
          servo_seen(false),
          fault("") {}
};

class PypilotApp {
public:
    PypilotApp();

    bool begin(IBoatImuBackend& imu, IServoBackend& servo, const PypilotAppConfig& config = PypilotAppConfig());
    void stop(uint64_t now_us);
    void tick(uint64_t now_us);

    bool set_enabled(bool enabled, uint64_t now_us);
    void set_servo_command(float normalized_command);

    const PypilotAppStatus& status() const { return status_; }
    float servo_command() const { return servo_command_; }

private:
    float clamp_command(float command) const;
    void set_fault(const char* message, uint64_t now_us);

    IBoatImuBackend* imu_;
    IServoBackend* servo_;
    PypilotAppConfig config_;
    PypilotAppStatus status_;
    uint64_t next_tick_us_;
    float servo_command_;
};

} // namespace pypilot

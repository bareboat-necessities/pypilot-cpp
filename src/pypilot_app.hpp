#pragma once

#include <stddef.h>
#include <stdint.h>

#if defined(ARDUINO) && defined(ESP32)
#ifndef PYPILOT_EVENT_LOOP_ENABLE_ARDUINO_WIFI_TCP
#define PYPILOT_EVENT_LOOP_ENABLE_ARDUINO_WIFI_TCP 1
#endif
#ifndef PYPILOT_EVENT_LOOP_ENABLE_ARDUINO_WIFI_UDP
#define PYPILOT_EVENT_LOOP_ENABLE_ARDUINO_WIFI_UDP 1
#endif
#endif

#include <pypilot_event_loop.hpp>
#include <pypilot_runtime.hpp>

namespace pypilot {

using PypilotEventLoop = pypilot_event_loop::EventLoop<128, 96>;
using PypilotRuntimeService = pypilot_runtime::PypilotRuntimeService<PypilotEventLoop, 8, 16>;

class IBoatImuBackend {
public:
    virtual ~IBoatImuBackend() {}
    virtual bool poll(pypilot_runtime::PypilotRuntimeState& runtime, uint64_t now_us) = 0;
};

class IServoBackend {
public:
    virtual ~IServoBackend() {}
    virtual void disable(pypilot_runtime::PypilotRuntimeState& runtime, uint64_t now_us) = 0;
    virtual bool apply(pypilot_runtime::PypilotRuntimeState& runtime, uint64_t now_us) = 0;
};

struct PypilotAppConfig {
    const char* runtime_host;
    uint16_t runtime_port;
    uint16_t runtime_udp_watch_port;
    uint32_t runtime_publish_period_us;
    uint32_t control_period_us;
    size_t max_runtime_output_bytes;
    bool enable_runtime_tcp;

    PypilotAppConfig()
        : runtime_host("0.0.0.0"),
          runtime_port(23322),
          runtime_udp_watch_port(0),
          runtime_publish_period_us(50000u),
          control_period_us(50000u),
          max_runtime_output_bytes(32768u),
          enable_runtime_tcp(true) {}
};

enum class PypilotAppState : uint8_t {
    stopped,
    running,
    fault
};

struct PypilotAppStatus {
    PypilotAppState state;
    bool runtime_listening;
    uint16_t runtime_port;
    uint64_t last_control_tick_us;
    uint32_t control_ticks;
    const char* fault;

    PypilotAppStatus()
        : state(PypilotAppState::stopped),
          runtime_listening(false),
          runtime_port(0),
          last_control_tick_us(0),
          control_ticks(0),
          fault("") {}
};

class PypilotApp {
public:
    PypilotApp();

    bool begin(IBoatImuBackend* imu_backend, IServoBackend* servo_backend, const PypilotAppConfig& config = PypilotAppConfig());
    void tick();
    void run_forever();
    void request_exit();
    void stop();

    PypilotEventLoop& loop() { return loop_; }
    PypilotRuntimeService& runtime() { return runtime_; }
    pypilot_runtime::PypilotRuntimeState& runtime_state() { return runtime_.state(); }
    const PypilotAppStatus& status() const { return status_; }

private:
    void control_tick();
    void set_fault(const char* message);
    void publish_runtime();

    PypilotEventLoop loop_;
    PypilotRuntimeService runtime_;

    IBoatImuBackend* imu_backend_;
    IServoBackend* servo_backend_;
    PypilotAppConfig config_;
    PypilotAppStatus status_;
    pypilot_event_loop::EventHandle control_tick_handle_;
};

} // namespace pypilot

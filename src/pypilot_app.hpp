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

#include <pypilot_data_model.hpp>
#include <pypilot_event_loop.hpp>
#include <pypilot_runtime.hpp>

namespace pypilot {

using PypilotEventLoop = pypilot_event_loop::EventLoop<128, 96>;
using PypilotRuntimeService = pypilot_runtime::PypilotRuntimeService<PypilotEventLoop, 8, 16>;
using PypilotDataModel = pypilot_data_model::DataModel<float>;

class IPypilotInputService {
public:
    virtual ~IPypilotInputService() {}
    virtual bool begin(PypilotEventLoop& loop, PypilotDataModel& model) = 0;
    virtual void stop(PypilotEventLoop& loop) = 0;
};

class IPypilotControlService {
public:
    virtual ~IPypilotControlService() {}
    virtual bool step(PypilotDataModel& model, uint64_t now_us) = 0;
};

class IBoatImuBackend {
public:
    virtual ~IBoatImuBackend() {}
    virtual bool poll(PypilotDataModel& model, uint64_t now_us) = 0;
};

class IServoBackend {
public:
    virtual ~IServoBackend() {}
    virtual void disable(PypilotDataModel& model, uint64_t now_us) = 0;
    virtual bool apply(PypilotDataModel& model, uint64_t now_us) = 0;
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

    bool begin(IBoatImuBackend* imu_backend,
               IServoBackend* servo_backend,
               IPypilotInputService* const* input_services = nullptr,
               size_t input_service_count = 0,
               IPypilotControlService* control_service = nullptr);
    void tick();
    void run_forever();
    void request_exit();
    void stop();

    PypilotEventLoop& loop() { return loop_; }
    PypilotRuntimeService& runtime() { return runtime_; }
    PypilotDataModel& data_model() { return model_; }
    const PypilotDataModel& data_model() const { return model_; }
    pypilot_runtime::PypilotRuntimeState& runtime_state() { return runtime_.state(); }
    const PypilotAppStatus& status() const { return status_; }

private:
    static constexpr uint32_t kControlPeriodUs = 50000u;

    void control_tick();
    void set_fault(const char* message);
    void publish_runtime();
    void stop_input_services();

    PypilotEventLoop loop_;
    PypilotDataModel model_;
    PypilotRuntimeService runtime_;

    IBoatImuBackend* imu_backend_;
    IServoBackend* servo_backend_;
    IPypilotInputService* const* input_services_;
    size_t input_service_count_;
    IPypilotControlService* control_service_;
    PypilotAppStatus status_;
    pypilot_event_loop::EventHandle control_tick_handle_;
};

} // namespace pypilot

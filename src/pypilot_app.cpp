#include "pypilot_app.hpp"

namespace pypilot {

PypilotApp::PypilotApp()
    : loop_(),
      runtime_(loop_),
      imu_backend_(0),
      servo_backend_(0),
      config_(),
      status_(),
      control_tick_handle_() {}

bool PypilotApp::begin(IBoatImuBackend* imu_backend, IServoBackend* servo_backend, const PypilotAppConfig& config) {
    imu_backend_ = imu_backend;
    servo_backend_ = servo_backend;
    config_ = config;
    status_ = PypilotAppStatus();

    if (!loop_.valid()) {
        set_fault("event loop backend is not valid");
        return false;
    }
    if (config_.control_period_us == 0) {
        set_fault("control loop period is zero");
        return false;
    }

    pypilot_runtime::PypilotRuntimeServiceOptions runtime_options;
    runtime_options.host = config_.runtime_host;
    runtime_options.tcp_port = config_.runtime_port;
    runtime_options.udp_watch_port = config_.runtime_udp_watch_port;
    runtime_options.publish_period_us = config_.runtime_publish_period_us;
    runtime_options.max_output_bytes = config_.max_runtime_output_bytes;
    runtime_options.enable_tcp = config_.enable_runtime_tcp;
    runtime_options.enable_periodic_publish = true;
    runtime_options.server_version = "pypilot-cpp";

    if (!runtime_.begin(runtime_options)) {
        set_fault(runtime_.fault());
        return false;
    }

    runtime_.state().servo.state.set(servo_backend_ ? "idle" : "no_servo_backend");
    runtime_.state().servo.engaged.set(false);

    control_tick_handle_ = loop_.on_repeat_us(config_.control_period_us, [this]() { control_tick(); });
    if (!control_tick_handle_.assigned()) {
        set_fault("failed to register control loop task");
        return false;
    }

    status_.runtime_listening = runtime_.listening();
    status_.runtime_port = runtime_.port();
    status_.state = PypilotAppState::running;
    publish_runtime();
    return true;
}

void PypilotApp::tick() {
    loop_.tick();
}

void PypilotApp::run_forever() {
    loop_.run_forever();
}

void PypilotApp::request_exit() {
    loop_.request_exit();
}

void PypilotApp::stop() {
    pypilot_runtime::PypilotRuntimeState& state = runtime_.state();
    if (servo_backend_) {
        servo_backend_->disable(state, loop_.clock().micros());
    }
    state.autopilot.enabled.set(false);
    state.servo.engaged.set(false);
    state.servo.state.set("stopped");
    if (control_tick_handle_.assigned()) {
        loop_.remove(control_tick_handle_);
        control_tick_handle_ = pypilot_event_loop::EventHandle{};
    }
    runtime_.stop();
    status_.runtime_listening = false;
    status_.runtime_port = 0;
    status_.state = PypilotAppState::stopped;
}

void PypilotApp::control_tick() {
    pypilot_runtime::PypilotRuntimeState& state = runtime_.state();
    const uint64_t now_us = loop_.clock().micros();
    status_.last_control_tick_us = now_us;
    ++status_.control_ticks;

    state.sensors.server_uptime.set(static_cast<double>(now_us) / 1000000.0);

    if (imu_backend_) {
        if (!imu_backend_->poll(state, now_us)) {
            state.sensors.status_warnings.set(state.sensors.status_warnings.get() + 1.0);
        }
    }

    const bool enabled = state.autopilot.enabled.get();
    if (!enabled) {
        state.servo.engaged.set(false);
        if (servo_backend_) {
            servo_backend_->disable(state, now_us);
            state.servo.state.set("disabled");
        } else {
            state.servo.state.set("no_servo_backend");
        }
        publish_runtime();
        return;
    }

    if (!imu_backend_) {
        state.servo.engaged.set(false);
        state.servo.state.set("no_imu_backend");
        state.sensors.status_faults.set(state.sensors.status_faults.get() + 1.0);
        publish_runtime();
        return;
    }

    if (!servo_backend_) {
        state.servo.engaged.set(false);
        state.servo.state.set("no_servo_backend");
        state.sensors.status_faults.set(state.sensors.status_faults.get() + 1.0);
        publish_runtime();
        return;
    }

    state.servo.engaged.set(true);
    if (!servo_backend_->apply(state, now_us)) {
        state.servo.engaged.set(false);
        state.servo.state.set("servo_rejected_command");
        state.sensors.status_faults.set(state.sensors.status_faults.get() + 1.0);
    } else {
        state.servo.state.set("active");
    }
    publish_runtime();
}

void PypilotApp::publish_runtime() {
    runtime_.publish_changed_values();
}

void PypilotApp::set_fault(const char* message) {
    status_.state = PypilotAppState::fault;
    status_.fault = message ? message : "fault";
    runtime_.state().sensors.status_faults.set(runtime_.state().sensors.status_faults.get() + 1.0);
    runtime_.state().servo.engaged.set(false);
    runtime_.state().servo.state.set(status_.fault);
}

} // namespace pypilot

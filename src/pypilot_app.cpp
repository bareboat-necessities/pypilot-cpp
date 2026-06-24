#include <cstring>

#include "pypilot_app.hpp"

namespace pypilot {

namespace {

void copy_text(char* out, size_t out_size, const char* text) {
    if (!out || out_size == 0) return;
    if (!text) {
        out[0] = '\0';
        return;
    }
    std::strncpy(out, text, out_size - 1);
    out[out_size - 1] = '\0';
}

} // namespace

PypilotApp::PypilotApp()
    : loop_(),
      runtime_(loop_),
      model_(),
      imu_backend_(nullptr),
      servo_backend_(nullptr),
      input_services_(nullptr),
      input_service_count_(0),
      control_service_(nullptr),
      status_(),
      control_tick_handle_() {}

bool PypilotApp::begin(IBoatImuBackend* imu_backend,
                       IServoBackend* servo_backend,
                       IPypilotInputService* const* input_services,
                       size_t input_service_count,
                       IPypilotControlService* control_service) {
    imu_backend_ = imu_backend;
    servo_backend_ = servo_backend;
    input_services_ = input_services;
    input_service_count_ = input_service_count;
    control_service_ = control_service;
    status_ = PypilotAppStatus();

    if (!loop_.valid()) {
        set_fault("event loop backend is not valid");
        return false;
    }

    if (!runtime_.begin()) {
        set_fault(runtime_.fault());
        return false;
    }

    copy_text(model_.server.version, sizeof(model_.server.version), runtime_.state().sensors.server_version.get());
    copy_text(model_.server.profile_name, sizeof(model_.server.profile_name), runtime_.state().sensors.profile_name.get());

    for (size_t i = 0; i < input_service_count_; ++i) {
        if (!input_services_ || !input_services_[i]) {
            set_fault("invalid input service");
            stop_input_services();
            runtime_.stop();
            return false;
        }
        if (!input_services_[i]->begin(loop_, model_)) {
            set_fault("failed to start input service");
            stop_input_services();
            runtime_.stop();
            return false;
        }
    }

    model_.servo.engaged.value = false;
    runtime_.state().servo.state.set(servo_backend_ ? "idle" : "no_servo_backend");
    runtime_.state().servo.engaged.set(false);

    control_tick_handle_ = loop_.on_repeat_us(kControlPeriodUs, [this]() { control_tick(); });
    if (!control_tick_handle_.assigned()) {
        set_fault("failed to register control loop task");
        stop_input_services();
        runtime_.stop();
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
    const uint64_t now_us = loop_.clock().micros();
    if (servo_backend_) {
        servo_backend_->disable(model_, now_us);
    }
    model_.ap.enabled.value = false;
    model_.servo.engaged.value = false;
    if (control_tick_handle_.assigned()) {
        loop_.remove(control_tick_handle_);
        control_tick_handle_ = pypilot_event_loop::EventHandle{};
    }
    stop_input_services();
    pypilot_runtime::publish_data_model_to_runtime(runtime_.state(), model_);
    runtime_.state().servo.engaged.set(false);
    runtime_.state().servo.state.set("stopped");
    runtime_.stop();
    status_.runtime_listening = false;
    status_.runtime_port = 0;
    status_.state = PypilotAppState::stopped;
}

void PypilotApp::control_tick() {
    pypilot_runtime::PypilotRuntimeState& runtime_state = runtime_.state();
    const uint64_t now_us = loop_.clock().micros();
    status_.last_control_tick_us = now_us;
    ++status_.control_ticks;

    pypilot_runtime::apply_runtime_commands_to_data_model(runtime_state, model_, now_us);
    model_.server.uptime_s.set(static_cast<float>(now_us) / 1000000.0f, now_us);

    if (imu_backend_) {
        if (!imu_backend_->poll(model_, now_us)) {
            ++model_.status.warnings.value;
        }
    }

    if (control_service_) {
        if (!control_service_->step(model_, now_us)) {
            ++model_.status.faults.value;
            model_.servo.engaged.value = false;
        }
    }

    if (!model_.ap.enabled.value) {
        model_.servo.engaged.value = false;
        if (servo_backend_) {
            servo_backend_->disable(model_, now_us);
        }
        pypilot_runtime::publish_data_model_to_runtime(runtime_state, model_);
        runtime_state.servo.state.set(servo_backend_ ? "disabled" : "no_servo_backend");
        publish_runtime();
        return;
    }

    if (!imu_backend_) {
        model_.servo.engaged.value = false;
        ++model_.status.faults.value;
        pypilot_runtime::publish_data_model_to_runtime(runtime_state, model_);
        runtime_state.servo.state.set("no_imu_backend");
        publish_runtime();
        return;
    }

    if (!servo_backend_) {
        model_.servo.engaged.value = false;
        ++model_.status.faults.value;
        pypilot_runtime::publish_data_model_to_runtime(runtime_state, model_);
        runtime_state.servo.state.set("no_servo_backend");
        publish_runtime();
        return;
    }

    model_.servo.engaged.value = true;
    if (!servo_backend_->apply(model_, now_us)) {
        model_.servo.engaged.value = false;
        ++model_.status.faults.value;
        pypilot_runtime::publish_data_model_to_runtime(runtime_state, model_);
        runtime_state.servo.state.set("servo_rejected_command");
    } else {
        pypilot_runtime::publish_data_model_to_runtime(runtime_state, model_);
        runtime_state.servo.state.set("active");
    }
    publish_runtime();
}

void PypilotApp::publish_runtime() {
    runtime_.publish_changed_values();
}

void PypilotApp::stop_input_services() {
    if (!input_services_) return;
    for (size_t i = input_service_count_; i > 0; --i) {
        if (input_services_[i - 1]) {
            input_services_[i - 1]->stop(loop_);
        }
    }
}

void PypilotApp::set_fault(const char* message) {
    status_.state = PypilotAppState::fault;
    status_.fault = message ? message : "fault";
    ++model_.status.faults.value;
    model_.servo.engaged.value = false;
    runtime_.state().sensors.status_faults.set(runtime_.state().sensors.status_faults.get() + 1.0);
    runtime_.state().servo.engaged.set(false);
    runtime_.state().servo.state.set(status_.fault);
}

} // namespace pypilot

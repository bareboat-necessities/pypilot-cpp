#include <chrono>
#include <cstdlib>
#include <thread>

#include <pypilot_app.hpp>

namespace {

bool set_env(const char* name, const char* value) {
#if defined(_WIN32)
    return _putenv_s(name, value) == 0;
#else
    return setenv(name, value, 1) == 0;
#endif
}

class FakeNmeaLikeInput final : public pypilot::IPypilotInputService {
public:
    bool begin(pypilot::PypilotEventLoop& loop, pypilot::PypilotDataModel& model) override {
        handle_ = loop.on_delay_us(1000, [&loop, &model, this]() {
            const uint64_t now_us = loop.clock().micros();
            model.navigation.gps.speed_kn.set(6.5f, now_us);
            model.navigation.gps.track_deg.set(123.0f, now_us);
            model.navigation.gps.source.value = ship_data_model::SensorSource::serial;
            model.navigation.gps.last_update_us = now_us;
            fired_ = true;
        });
        return handle_.assigned();
    }

    void stop(pypilot::PypilotEventLoop& loop) override {
        if (loop.valid(handle_)) {
            loop.remove(handle_);
        }
    }

    bool fired() const { return fired_; }

private:
    async_event_loop::EventHandle handle_{};
    bool fired_ = false;
};

class ObservingControl final : public pypilot::IPypilotControlService {
public:
    bool step(pypilot::PypilotDataModel& model, uint64_t) override {
        ++calls_;
        if (model.navigation.gps.speed_kn.valid && model.navigation.gps.speed_kn.value == 6.5f) {
            saw_input_speed_ = true;
        }
        return true;
    }

    int calls() const { return calls_; }
    bool saw_input_speed() const { return saw_input_speed_; }

private:
    int calls_ = 0;
    bool saw_input_speed_ = false;
};

} // namespace

int main() {
    if (!set_env("PYPILOT_RUNTIME_TCP_ENABLED", "false")) return 1;
    if (!set_env("PYPILOT_RUNTIME_PERIODIC_PUBLISH_ENABLED", "false")) return 2;

    FakeNmeaLikeInput input;
    ObservingControl control;
    pypilot::IPypilotInputService* inputs[] = {&input};

    pypilot::PypilotApp app;
    if (!app.begin(nullptr, nullptr, inputs, 1, &control)) return 3;

    for (int i = 0; i < 40; ++i) {
        app.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if (!input.fired()) return 4;
    if (!app.data_model().navigation.gps.speed_kn.valid) return 5;
    if (app.data_model().navigation.gps.speed_kn.value != 6.5f) return 6;
    if (control.calls() <= 0) return 7;
    if (!control.saw_input_speed()) return 8;

    app.stop();
    return 0;
}

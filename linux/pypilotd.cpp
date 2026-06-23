#include <cstdio>
#include <cstdlib>

#include "pypilot_app.hpp"

namespace {

uint16_t read_env_port(const char* name, uint16_t fallback) {
    const char* text = std::getenv(name);
    if (!text || !*text) return fallback;

    char* end = nullptr;
    const unsigned long value = std::strtoul(text, &end, 10);
    if (!end || *end != '\0' || value > 65535ul) return fallback;
    return static_cast<uint16_t>(value);
}

const char* read_env_text(const char* name, const char* fallback) {
    const char* text = std::getenv(name);
    return (text && *text) ? text : fallback;
}

} // namespace

int main(int, char**) {
    pypilot::PypilotApp app;
    pypilot::PypilotAppConfig config;
    config.runtime_host = read_env_text("PYPILOTD_RUNTIME_HOST", config.runtime_host);
    config.runtime_port = read_env_port("PYPILOTD_RUNTIME_PORT", config.runtime_port);

    if (!app.begin(nullptr, nullptr, config)) {
        std::fprintf(stderr, "pypilotd: failed to start: %s\n", app.status().fault);
        std::fflush(stderr);
        return 1;
    }

    std::fprintf(stderr,
                 "pypilotd: runtime server listening on port %u; "
                 "wire Linux BoatIMU and servo backends before enabling closed-loop control.\n",
                 static_cast<unsigned>(app.status().runtime_port));
    std::fflush(stderr);
    app.run_forever();
    app.stop();
    return 0;
}

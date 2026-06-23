#include <cstdio>

#include "pypilot_app.hpp"

int main(int, char**) {
    pypilot::PypilotApp app;
    pypilot::PypilotAppConfig config;

    if (!app.begin(nullptr, nullptr, config)) {
        std::fprintf(stderr, "pypilotd: failed to start: %s\n", app.status().fault);
        return 1;
    }

    std::fprintf(stderr,
                 "pypilotd: runtime server listening on port %u; "
                 "wire Linux BoatIMU and servo backends before enabling closed-loop control.\n",
                 static_cast<unsigned>(app.status().runtime_port));
    app.run_forever();
    app.stop();
    return 0;
}

#include <cstdio>

#include "pypilot_app.hpp"

int main(int, char**) {
    pypilot::PypilotApp app;

    if (!app.begin(nullptr, nullptr)) {
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

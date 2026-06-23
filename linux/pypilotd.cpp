#include <cstdio>
#include "pypilot_app.hpp"

int main(int, char**) {
    std::fprintf(stderr,
                 "pypilotd: pypilot-cpp application shell is present; "
                 "Linux BoatIMU and servo backends must be wired before enabling closed-loop autopilot.\n");
    return 2;
}

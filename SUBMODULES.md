# Submodule pins

All module entries track `main` in `.gitmodules`. Current CI bootstraps fresh module checkouts from `main` with `scripts/bootstrap-modules.sh`.

This document records the complete umbrella checkout set in one table. The commit values are documentation snapshots for reproducibility; when true gitlink submodules are used, this super-repo can pin exact commits in the tree.

| Module | Umbrella role | Recorded commit |
| --- | --- | --- |
| `pypilot-event-loop` | build/test | `d3eba4a8dccd10721de3d9682b1d4a022592e2c4` |
| `pypilot-settings` | build/test | `0049f028cc65590a797992c0a4faad9986e20469` |
| `pypilot-mdns` | build/test | `fbf84161cd456fbbb58392e93c7e1baf09df2fef` |
| `pypilot-syslib` | build/test | `fbd2d2d4c358009232d0ac586df2eb9c60c31630` |
| `pypilot-data-model` | build/test | `7cde4cea09a9471eccdefa5bcc733fe0831f448c` |
| `pypilot-servo-protocol` | build/test | `ba5d5ddc938a06ef1d226e575cea6e057ef1a297` |
| `pypilot-client-protocol` | build/test | `af3c23f9cbe4144e21b84ace83a98ba522ad91a0` |
| `pypilot-runtime` | build/test | `2d58dd99a0f37e6a8c4a58cb4cde676bbb5b5fde` |
| `pypilot-algorithms` | build/test | `0941a2fb6cb121b018a0081e9c864edfe78675d6` |
| `pypilot-boatimu` | build/test | `4db929c1726aa2af0318e0bdd92b2c6a3fcb3435` |
| `ocean-imu` | checkout-only | `a5814ce94521f6037d1eda5780f2096e432b50a5` |
| `pypilot-nmea0183-connector` | build/test | `699b58031a5496678114417e28dc02cd19d48c66` |
| `pypilot-signalk-connector` | build/test | `185d50b892e5a71a746c1ac4b1c2ff5cd5b552c3` |
| `pypilot-sensors` | build/test | `c32892c8aa9ce85d02d4bd54ec59f9cfb46c93d4` |
| `pypilot-gps-adapter` | build/test | `072caaddd967e579d6b0fdee7bc0611905d5caee` |
| `pypilot-pilots-logic` | build/test | `983a07de94f484e31784fe164b1c27842f4330c6` |
| `pypilot-steering-signaling` | build/test | `33341e4ed8e2ab7d6cc044d2cc214c0a7dd1c322` |

`ocean-imu` is checkout-only for future BoatIMU/AHRS integration. It is cloned and verified as a git checkout, but it is not built by `scripts/build-linux-all.sh`, not compiled by `scripts/build-arduino-all.sh`, and its tests are not run by this umbrella repository.

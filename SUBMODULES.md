# Submodule pins

All module entries track `main` in `.gitmodules`. Current CI bootstraps fresh module checkouts from `main` with `scripts/bootstrap-modules.sh`.

This document records the complete umbrella checkout set in one table. The commit values are the latest observed `main` heads for the respective submodule repositories at the time this file was updated. When true gitlink submodules are used, the hash shown by GitHub under `modules/` should match these recorded commits.

| Module | Umbrella role | Recorded commit |
| --- | --- | --- |
| `pypilot-event-loop` | build/test | `9e391ab5ded6f2c1721587b9b035938c15264cc6` |
| `pypilot-settings` | build/test | `a453f8693e60246097853bcf885eaa62a7a9caae` |
| `pypilot-mdns` | build/test | `02854328f0ab3bd8a3f459c521ac1df1d469832f` |
| `pypilot-syslib` | build/test | `fbd2d2d4c358009232d0ac586df2eb9c60c31630` |
| `pypilot-data-model` | build/test | `3f1a14b83263df14ecafcc8f62fbd1ba8ab82ef8` |
| `pypilot-servo-protocol` | build/test | `ba5d5ddc938a06ef1d226e575cea6e057ef1a297` |
| `pypilot-client-protocol` | build/test | `af3c23f9cbe4144e21b84ace83a98ba522ad91a0` |
| `pypilot-runtime` | build/test | `694146a4bc4867c2bcf8d277a04cefa2b87f5ff3` |
| `pypilot-algorithms` | build/test | `f6ddd889d19a00dd94da9ad960d303606afda732` |
| `pypilot-boatimu` | build/test | `4db929c1726aa2af0318e0bdd92b2c6a3fcb3435` |
| `ocean-imu` | checkout-only | `a872d34e8aefcd65030eb2ec0c5c2ba3d87f810c` |
| `pypilot-nmea0183-connector` | build/test | `699b58031a5496678114417e28dc02cd19d48c66` |
| `pypilot-signalk-connector` | build/test | `c0e2444c6e7e36fc332ef026b69283823005031d` |
| `pypilot-sensors` | build/test | `a1c875d012639dcff3bd89abe263892cf4093670` |
| `pypilot-gps-adapter` | build/test | `072caaddd967e579d6b0fdee7bc0611905d5caee` |
| `pypilot-pilots-logic` | build/test | `18b2ac77fe21caee2ac664cbd9c8d64b6c669607` |
| `pypilot-steering-signaling` | build/test | `33341e4ed8e2ab7d6cc044d2cc214c0a7dd1c322` |

`ocean-imu` is checkout-only for future BoatIMU/AHRS integration. It is cloned and verified as a git checkout, but it is not built by `scripts/build-linux-all.sh`, not compiled by `scripts/build-arduino-all.sh`, and its tests are not run by this umbrella repository.

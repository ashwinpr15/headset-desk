# Diagnostic prerelease build and validation

Date: 2026-10-07. Target: Windows 11 x64. Build and automated checks were offline; the user separately ran the reviewed physical sessions recorded in `hardware-validation.md`.

## Local build

Checksum-verified portable build tools were used within the workspace. No system compiler installation or global PATH change was required.

| Tool | Version | Source |
|---|---|---|
| LLVM-MinGW / Clang | 20261006 / 23.1.3 | Official mstorsjo/llvm-mingw release |
| CMake | 4.4.4 | Official Kitware release |
| Ninja | 1.13.2 | Official ninja-build release |
| Catch2 | 3.8.1 | Pinned upstream source archive and SHA-256 |

Debug build: all targets compiled and linked, including native Windows RFCOMM. Release build: internal diagnostics compiled and statically linked to the C++ runtime. No Qt, GUI or hardware was required for these builds. MSVC presets are provided, but MSVC has not been tested here.

## Automated results

**6/6 CTest groups passed.** The two C++ suites contain **100 test cases / 881 assertions**, all passing:

- Imported protocol/transport regression suite: 78 cases, 536 assertions.
- Headset Desk core/safety/capture suite: 22 cases, 345 assertions.
- XM5 simulation executable smoke test.
- CH720N simulation executable smoke test.
- CLI refuses an implicit hardware query without `--read-only`.
- CLI refuses mixed hardware/simulation modes.

Final Debug test duration: approximately 12.25 seconds. Sanitized machine-readable results are at `validation/publication-tests.xml`. Capture replay decodes the original wire bytes, checks the exact outgoing GET order, and verifies both models' actual telemetry and XM5 notifications without opening a native socket.

The updated Release executable was checked with only Windows System32 on PATH: help, both simulated models and both CLI refusal cases passed. Synthetic TX/RX packet capture worked, and an existing capture was preserved when a second run tried to reuse its name. Its import table contains only Windows system components. Checks and executable hash are recorded in `validation/portable-smoke.json`; distributed ZIP hashes accompany the release.

Tests cover framing, escaping, checksum validation, malformed packets, incomplete stream chunks, duplicate handling, ACK-vs-response distinction, missing ACK/response timeouts, request/subtype matching, rejection of old buffered telemetry, V1/V2 battery separation, supported device selection, paired/active launch policy, capability evidence, Known/Unknown values, simulated disconnect/reconnect and the final write-blocking transport. No test calls native device enumeration or opens a real Bluetooth socket.

## Limits

Offline simulation demonstrates program behavior, not model compatibility. Successful native read sessions have now been confirmed on XM5 2.5.1 and CH720N 1.1.4. This does not cover failure deadlines, reconnect behavior or unobserved control states. Charging=false is observed; charging=true is untested. Active codec is a returned Sony value and needs independent interpretation under multipoint. Local C++ test binaries need the portable compiler's runtime on PATH; the distributed diagnostic executable statically links that runtime.

The core is synchronous for the upcoming GUI worker. Automatic reconnect scheduling, GUI notifications, tray lifecycle, startup settings and writable controls are later phases. Imported ACK sequence conventions need packet-capture review before any write controls are enabled. The CLI is an internal target and is not a finished or signed end-user release.

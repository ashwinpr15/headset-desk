# Headset Desk

Windows 11 utility in development for **WH-1000XM5** and **WH-CH720N**. The current experimental prerelease contains the protocol/core and internal read-only diagnostic tool. The desktop application is a later phase.

[![Windows build](https://github.com/ashwinpr15/headset-desk/actions/workflows/windows.yml/badge.svg)](https://github.com/ashwinpr15/headset-desk/actions/workflows/windows.yml)

[Diagnostic downloads](https://github.com/ashwinpr15/headset-desk/releases) · [Hardware evidence](docs/hardware-validation.md) · [Architecture](docs/architecture.md)

The intended MVP is a window plus tray with model, battery, charging and firmware telemetry; NC/Ambient 1–20/Off; five-band EQ and Clear Bass. Closing the window will keep the connection alive in the tray; Quit will exit. Sign-in launch is off by default. The GUI, tray, editable controls and installer come in later phases.

## What exists

- C++20/CMake targets for the reused Sony protocol, native Windows RFCOMM transport, Headset Desk core and internal diagnostic executable.
- Paired-device enumeration includes disconnected devices and preserves Windows paired/active flags. It uses cached Windows records, with no inquiry or custom pairing.
- Strict supported-model selection. An unknown or renamed device cannot select V2 by substring or MAC prefix.
- A V2 service check before queries and a final transport guard that blocks all headset setting writes, including writes from imported protocol setters.
- Known/Unknown telemetry, with unknown charging distinct from `false` and unknown battery distinct from `0`.
- Read/write evidence tracked separately. Physical captures confirm the observed reads on XM5 firmware 2.5.1 and CH720N firmware 1.1.4. Successful reads do not enable writes.
- Offline simulation, protocol regressions and core tests. Test execution never enumerates or connects to real headphones.

## Building

Requires Windows, a C++20 toolchain, CMake 3.25+ and Ninja or Visual Studio 2022. Qt is not needed for Phase 2. The test build uses Catch2 3.8.1, fetched with a pinned SHA-256; an installed Catch2 3 can also be used.

From this folder, with a compiler available:

```powershell
cmake --preset windows-ninja-debug
cmake --build --preset windows-ninja-debug
ctest --preset windows-ninja-debug
```

Visual Studio has equivalent `windows-msvc-debug` presets. [Windows CI](https://github.com/ashwinpr15/headset-desk/actions/workflows/windows.yml) builds with MSVC and runs only offline checks. The locally verified compiler is recorded in [build validation](docs/build-validation.md). To build only diagnostics, use `windows-ninja-diagnostics`. MSVC uses its static runtime; the portable prerelease uses LLVM-MinGW with its C++ runtime linked statically.

## Internal diagnostics

No arguments shows help and opens no Bluetooth connection.

```powershell
.\headset-desk-diagnostics.exe --simulate xm5
.\headset-desk-diagnostics.exe --simulate ch720n
```

These print **synthetic** data and cannot verify physical device compatibility.

Extract the portable ZIP from Releases and open PowerShell in that folder:

```powershell
.\headset-desk-diagnostics.exe --devices
.\headset-desk-diagnostics.exe --device-index N --read-only --dump capture.txt
```

Replace `N` with the number printed beside the desired headset. Pair normally in Windows first. Use a new capture filename. Follow [the hardware instructions and evidence limits](docs/hardware-validation.md). The CLI remains an internal diagnostic build target; this download is for development and validation.

Example from the reviewed CH720N hardware run:

```text
Model (paired Windows name): WH-CH720N
Battery %: 99
Charging: false
Firmware: 1.1.4
Reported active codec: AAC
Noise control: Off
EQ: 0 5 7 7 9 Clear Bass -1
Writes: disabled
```

## Connection policy for the future GUI

The core selects an auto-connect target only when Windows reports a supported device both paired and active. Otherwise it remains idle and offers manual Connect. Paired disconnected headsets remain in the manual list. If both are active, a supplied preferred address wins; absent a preference, XM5 is selected first. The current diagnostic tool never auto-connects on startup. Automatic reconnect and tray lifecycle integration are deferred to the GUI worker.

## Scope and evidence

Supported profile names are exactly `WH-1000XM5` and `WH-CH720N` (case insensitive). This is a conservative protocol-generation choice; the paired Windows name is not a live Sony identity response. A renamed headset stays idle. Unknown model support can be added through a reviewed profile and independent evidence later.

The user ran read-only diagnostics on both physical headsets and confirmed the returned values. The reviewed packet fixtures and regression tests cover firmware 2.5.1 on XM5 and 1.1.4 on CH720N, with noise control Off and charging false. Charging true, other noise states, all EQ presets/range endpoints and all setting writes remain untested by Headset Desk. AAC is the headset-reported value; independent confirmation of Windows' negotiated audio codec remains separate. Supported codec lists are static specs.

See [architecture](docs/architecture.md), [upstream changes](docs/upstream-changes.md), [CH720N validation](docs/wh-ch720n-validation.md) and [attribution](docs/attribution.md).

## License

New Headset Desk code is MIT licensed. Imported code retains the exact upstream [MIT notice](vendor/sony-device-center/LICENSE). No Sony artwork or marketing assets were imported. Headset Desk is an independent project and is not affiliated with Sony. Retain the included license and runtime notices when distributing the portable ZIP.

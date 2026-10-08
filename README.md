# Headset Desk

A Windows 11 desktop app for **Sony WH-1000XM5** and **WH-CH720N**: battery and firmware, noise control, five-band EQ and Clear Bass, with a system tray.

[![Windows build](https://github.com/ashwinpr15/headset-desk/actions/workflows/windows.yml/badge.svg)](https://github.com/ashwinpr15/headset-desk/actions/workflows/windows.yml)

## Download for Windows

**[Download Windows installer](https://github.com/ashwinpr15/headset-desk/releases/download/v0.3.0-beta.1/headset-desk-v0.3.0-beta.1-windows-x64-setup.exe)**

[Portable ZIP](https://github.com/ashwinpr15/headset-desk/releases/download/v0.3.0-beta.1/headset-desk-v0.3.0-beta.1-windows-x64.zip) · [All versions and release notes](https://github.com/ashwinpr15/headset-desk/releases) · [Report an issue](https://github.com/ashwinpr15/headset-desk/issues)

Current version: **0.3.0-beta.1**. Windows 11, x64. The installer bundles the app and runtime, creates a Start menu shortcut, and supports uninstalling through Windows Settings. It installs for your Windows account without administrator rights. No Qt installation, developer tools, sign-in or internet connection is needed to run the installed app.

This is an **experimental beta**. Reads have been checked on the devices listed below. Setting writes have passed offline checks but **have not been tested on physical headphones by Headset Desk**. Controls start off and must be enabled for each connection. Successful reads do not verify writes.

The app and installer are unsigned; Windows SmartScreen may warn about an unrecognized app. Download from this repository's Releases; `CHECKSUMS.txt` accompanies the assets. No firmware updates or power commands are offered.

## Install and use

1. Download the installer, run it, then open **Headset Desk** from Start. Alternatively, extract the **entire** portable ZIP and open `headset-desk.exe` inside its folder.
2. Pair your headset in Windows Bluetooth settings and turn it on. Close other headset-control tools. The app connects automatically when Windows reports a supported paired headset as active; otherwise choose **Connect**.
3. Check the readings. To try changing settings, turn on **Controls** in the Noise control card. Changes are experimental on both models. Choose NC, Ambient or Off; Ambient has levels 1–20. Edit the five EQ bands and Clear Bass, then choose **Apply**. **Reset** discards unsent EQ edits.

Every change is sent once and checked with a fresh readback. An ACK alone is insufficient. A timeout, disconnection or mismatch disables controls and reports the problem; no change is replayed. Check Sony's app if a change is uncertain. Disconnecting, switching or reconnecting clears the experimental toggle.

## The app

These screenshots are rendered by the actual Windows app with **simulated data**, not live readings.

<img src="docs/screenshots/light-100.png" alt="Headset Desk light theme, simulated data" width="360"> <img src="docs/screenshots/dark-100.png" alt="Headset Desk dark theme, simulated data" width="360">

- Model, battery percentage, charging state, firmware and headset-reported codec.
- NC / Ambient / Off; Ambient level 1–20.
- Five EQ bands: 400 Hz, 1 kHz, 2.5 kHz, 6.3 kHz and 16 kHz, plus Clear Bass. Values −10 to +10; Apply sends custom EQ.
- Resizable window, Windows light/dark appearance and native tray.
- Close hides to tray and keeps the link alive. Tray **Quit** exits. Launching a second copy brings forward the first.
- Refresh on connection, reopening and demand, plus a slow battery timer. One headset session at a time; select the other device, then Connect.
- No cloud account, analytics, internet updater, Windows service or launch at sign-in.

## Device evidence

| Model | Physical read evidence | Noise / EQ writes |
|---|---|---|
| WH-1000XM5 | Firmware 2.5.1; battery 44%, charging false, reported AAC, Off, one EQ state | Experimental opt-in; hardware confidence **UNKNOWN** |
| WH-CH720N | Firmware 1.1.4; battery 99%, charging false, reported AAC, Off, one EQ state | Experimental opt-in; hardware confidence **UNKNOWN** |

Read evidence covers those captured states only. Charging true, NC/Ambient reads, other firmware, EQ presets/endpoints, GUI operation on hardware and long-session/reconnect behavior still need physical testing. The Off replies differ; successful reads do not establish interchangeable write behavior. Model identity comes from Windows' cached pairing name. The codec is reported by the headset, not measured from Windows audio. Renamed and other models are excluded.

[Hardware captures and limits](docs/hardware-validation.md) · [CH720N evidence](docs/wh-ch720n-validation.md) · [Beta validation](docs/desktop-validation.md) · [Architecture](docs/architecture.md)

## Build from source

Use C++20, CMake 3.25+, Ninja, Qt **6.8.3** `mingw_64`, and its matching MinGW **GCC 13.1.0** kit. Put compiler and Qt `bin` directories on PATH.

```powershell
cmake -S . -B build-desktop -G Ninja -DCMAKE_BUILD_TYPE=Release -DHEADSET_DESK_BUILD_DESKTOP=ON -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/mingw_64
cmake --build build-desktop --parallel 4
ctest --test-dir build-desktop --output-on-failure
```

[Build, deploy and package](docs/desktop-build.md). Default CMake builds the core and internal read-only diagnostics without Qt; the CLI is a development tool, not the consumer app. [Diagnostic instructions](docs/hardware-validation.md). Windows CI checks the core, protocol and diagnostics offline; local validation also checks Qt and the installer.

## Credits and license

Based on protocol and Windows transport from [marconvcm/sony-device-center](https://github.com/marconvcm/sony-device-center). Imported files retain upstream MIT notices; [attribution and changes](docs/attribution.md). New Headset Desk code is [MIT licensed](LICENSE).

Qt 6.8.3 is dynamically linked under LGPLv3. Downloads include Qt and other runtime notices. Exact corresponding Qt sources are supplied as a companion release asset; application source is also available. Replacing compatible Qt DLLs/plugins and reverse engineering to debug those replacements is permitted. Preserve notices and source availability when redistributing.

Headset Desk is independent and not affiliated with Sony. No Sony artwork is included.

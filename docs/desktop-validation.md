# Headset Desk — local desktop review

Version **0.2.0-preview.1**, Windows 11 x64. This build has not been published on GitHub.
It is read-only: changing noise control or EQ is unavailable, and the transport blocks
all setting writes on both models. New GUI testing was entirely offline; no commands
were sent to the physical headphones by the agent.

## What changed

Qt 6.8.3 with FluentWinUI3 was verified in the actual kit and rendered app. QApplication
and Qt Widgets provide the tray and menu; a small shared palette follows the Windows theme.
One worker owns the connection and queries. Close hides to tray; Open restores and refreshes;
Quit disconnects and joins. Only one session can exist in a process, and a second app launch
forwards Open to the first. Manual disconnect and switching cancel reconnect attempts.

The UI feedback is incorporated: visually inactive noise controls with the explanatory line
inside the card; taller EQ tracks with values above them; grey preview text; a selector only
for multiple paired headsets; refresh in the header; full-refresh age under the battery;
and Clear Bass's value beside its label. Default size is 420 × 700 logical pixels, resizable
with scrolling. The EQ track length is approximately twice the first preview's length.

Noise/EQ write confidence on **both** XM5 and CH720N is Unknown. Physical read evidence remains
limited to the previously reviewed firmware and states. Simulation does not promote confidence.
The public repository and earlier diagnostic release were not modified.

## Screenshots

Companion screenshots are in the separate local review deliverable. These are unedited client-area captures from the actual deployed app, clearly labelled simulated.
Windows title-bar chrome is excluded. Process scale factors 1.0 and 1.5 exercise Qt DPI paths;
the user's global Windows display setting was not changed. The 150% test caught the available
height limit on this display; spacing and default height were adjusted so the connected main
view fits without clipping. Smaller windows scroll intentionally.

| Theme | 100% | 150% |
|---|---|---|
| Light | `light-100.png` | `light-150.png` |
| Dark | `dark-100.png` | `dark-150.png` |

Also supplied: `ch720n-dark-100.png` and `idle-light-100.png`.
The synthetic 80%, NC and flat EQ values are not the real captured headset values.

## Measured size and performance

- Complete deployed folder, including licenses: **71,125,330 bytes (67.83 MiB)**.
- Median process-launch-to-first-frame time: **666.7 ms**.
- Observed startup range across six local runs: **649.5–714.1 ms**.
- Idle visible-window working set: **181.6–192.5 MiB**.
- Idle private bytes: **136.3–146.6 MiB**.

| Preview | PNG pixels | Launch → first frame | Idle working set | Idle private bytes |
|---|---|---:|---:|---:|
| light-100 | 420 × 700 | 714.1 ms | 183.1 MiB | 137.5 MiB |
| dark-100 | 420 × 700 | 680.1 ms | 182.6 MiB | 136.8 MiB |
| light-150 | 630 × 1050 | 665.8 ms | 192.5 MiB | 146.6 MiB |
| dark-150 | 630 × 1050 | 667.6 ms | 192.1 MiB | 146.0 MiB |
| ch720n-dark-100 | 420 × 700 | 657.3 ms | 183.0 MiB | 136.9 MiB |
| idle-light-100 | 420 × 700 | 649.5 ms | 181.6 MiB | 136.3 MiB |

Measured on this Windows 11 machine with the visible window and offline simulation, warm OS
caches, a clean runtime PATH and no development Qt import overrides. Startup includes process
launch and DLL loading; the external harness samples the first-frame marker every five milliseconds.
Internal main-to-frame timings are retained in each JSON. Memory was read from Windows after
about six seconds, before screenshot readback. Working set includes shared DLL pages, so it
differs from private bytes. No cold-boot, real Bluetooth connection or minimized-tray memory
claim is made. This Qt build has a modest download size but is not a tiny-memory application.

## Checks and remaining validation

**7/7 CTest groups passed**, including 101 C++ protocol/core cases and four desktop functional
scenarios. Desktop checks cover two-model selection, clearing old telemetry, idle manual connect,
refresh, disabled EQ controls, resize/scroll, close/reopen connection preservation, Quit/join,
and forwarding from a second process. Existing guards still reject setters and unknown commands.

The actual portable EXE was also launched twice outside the sandbox on this Windows machine.
The second launch exited successfully without rendering another window or opening another
session; the primary survived and exited cleanly. Loaded Qt libraries came from the portable
folder. The earlier single-instance failure was a sandbox denial of Windows named-pipe access.
The artifact launches with a clean runtime PATH; Qt does not need to be installed separately.

This is not new physical GUI evidence. Real-headset GUI connection/reconnect/long-session
checks and additional NC/Ambient/EQ read states are still needed. Writes remain blocked until
model-specific validation is reviewed. Firmware updates, a public CLI, installer, sign-in launch
toggle, service and cloud features are outside this build.

Extract the entire portable ZIP and read START-HERE.txt. The app is unsigned; SmartScreen
may show a warning. Runtime license notices and complete corresponding Qt sources are supplied.
See the source deliverable's docs/desktop-build.md for rebuilding and runtime replacement.

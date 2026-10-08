# Windows desktop build and packaging

Version: 0.3.0-beta.1. Windows 11 x64 only. Installer and portable ZIP in GitHub Releases.

## Build

Use Qt 6.8.3's `mingw_64` kit and matching Qt MinGW GCC 13.1.0 toolchain,
CMake 3.25 or newer and Ninja. Put the compiler and Qt `bin` directories on PATH.
Set `CMAKE_PREFIX_PATH` to the Qt kit. Qt is dynamically linked; do not statically
link Qt for this packaging workflow. With MSVC, use a matching Qt MSVC kit and dynamic CRT.

```powershell
cmake -S . -B build-desktop -G Ninja -DCMAKE_BUILD_TYPE=Release -DHEADSET_DESK_BUILD_DESKTOP=ON -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/mingw_64
cmake --build build-desktop --parallel 4
ctest --test-dir build-desktop --output-on-failure
```

Qt modules: Core, Gui, Qml, Quick, QuickControls2, Widgets and Network.
Network is used for local instance IPC, not an internet service. Widgets supplies
QSystemTrayIcon and its context menu, hence QApplication. Test builds also need Qt Test.
Catch2 3.8.1 is fetched with a pinned hash unless supplied locally.

The default `HEADSET_DESK_BUILD_DESKTOP=OFF` preserves the core and internal CLI build.
The existing public CI builds those targets only; it also covers the experimental core setting paths.

## Local deployment

Copy `build-desktop/bin/headset-desk.exe` to an empty staging directory. Run:

```powershell
windeployqt --release --qmldir apps/desktop/qml --no-translations --no-opengl-sw --no-system-d3d-compiler --skip-plugin-types designer,qmltooling STAGING/headset-desk.exe
```

Windows 11 provides the system D3D compiler. Retain deployed Qt/QML plugins and
matching MinGW runtime DLLs. This application uses Qt's deployment output with all
discovered control styles; the selected style is FluentWinUI3, fallback Fusion.
Do not delete runtime files based on filename guesses. Include application and
upstream MIT licenses, Qt LGPLv3/GPLv3 texts, Qt third-party notices and GCC/MinGW
runtime notices. Provide exact Qt source archives alongside redistributed binaries.
Runtime source URLs and verified hashes are in the portable folder's license manifest.

Extract the entire ZIP before running. This is a portable folder, not a single-file EXE.
The app is unsigned, and Windows SmartScreen may display a warning.

## Desktop behavior

- Default 420 × 700 logical-pixel window; resizable to 360 × 420 minimum, with scrolling.
  Height was increased from the first preview to accommodate taller EQ tracks.
- Follow Windows light/dark mode using QStyleHints. FluentWinUI3 is available since Qt 6.8
  and was verified in the actual 6.8.3 kit. A shared palette colors the page and text.
- Auto-connect to supported paired/active headphones; otherwise idle with Connect.
- One selected session; changing target disconnects, then manual Connect opens the new target.
- Close hides to tray and leaves the session intact. Left click or Open restores and refreshes.
  If no tray is available, Close performs Quit so the app cannot become inaccessible.
- Quit stops timers, disconnects on the worker and joins the thread before exit.
- Read on connect/open/demand. Battery and charging query every three minutes.
  The header age refers to the last full refresh; individual fields may be Unknown.
- Bounded reconnect delays 1, 2, 4, 8 and 16 seconds, at most five attempts.
  Initial manual connect failures do not retry automatically. A successful manual session
  may reconnect after loss. Manual Disconnect, switching and Quit stop retrying.
- Local single-instance pipe accepts only `OPEN`; no raw protocol or remote command interface.
- Remember the last successful real device in per-user Windows settings. No sign-in launch,
  service, cloud account, analytics or updater is installed.
- Controls start disabled. Explicit per-connection experimental opt-in permits only validated
  noise/EQ settings. Each setting requires ACK plus fresh readback. Failure disables controls;
  no writes are retried or replayed. Hardware write confidence remains Unknown on both models.

## Offline review

Internal preview switches never enumerate Windows headphones or open hardware:

```powershell
.\headset-desk.exe --simulate xm5
.\headset-desk.exe --simulate ch720n
.\headset-desk.exe --simulate idle
```

`--theme light|dark`, `--capture PNG`, `--metrics JSON`, `--ready-file JSON` and
`--exit-after-capture` are internal review flags and require simulation. They are
not product settings or marketed CLI features. Capture records the real rendered
Qt client area, excluding the Windows title bar. QT_SCALE_FACTOR=1 and 1.5 exercise
Qt's 100%/150% DPI rendering without changing the user's global display setting.

Memory is sampled after six seconds with the window visible and before screenshot
readback. Working set includes shared DLL pages; private bytes is separate. Startup
is measured both from main to first rendered frame and externally from process launch
to the first-frame marker. These local runs use offline simulation and warm OS caches;
they do not measure Bluetooth connection latency or cold boot performance.

Offline coverage includes supported-model switching, known-to-unknown clearing, idle
manual connect, refresh, disabled controls, resize, close/reopen session preservation,
Quit/join and second-process forwarding. Existing protocol guard and hardware-fixture
replays remain in place. New real-device GUI/reconnect/long-session validation is still
needed; simulation does not promote hardware confidence.

Official style reference: https://doc.qt.io/qt-6.8/qtquickcontrols-fluentwinui3.html

## Installer

Deploy the complete runtime folder and licenses first. Compile with Inno Setup 7.1.0:

```powershell
ISCC /DStageDir=C:/release/headset-desk-v0.3.0-beta.1-windows-x64 /DOutputDir=C:/release packaging/windows/headset-desk.iss
```

The installer is per-user, requires Windows build 22000 or newer and x64 compatibility,
includes all staged runtime files and an uninstaller, and creates a Start menu shortcut.
An optional desktop shortcut is unchecked. Launch after install is unchecked and skipped
for silent installation. No network or startup registration is required. Quit the app
from its tray before installing an update; the installer checks the running-app mutex.
Publish the setup EXE, complete portable ZIP, application source, exact Qt runtime sources
and SHA-256 checksums under the same version tag. The binary is unsigned.

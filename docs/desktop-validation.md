# Windows beta validation

Version **0.3.0-beta.1**. Built on Windows 11 using Qt 6.8.3, FluentWinUI3,
MinGW GCC 13.1.0, CMake 4.4.4, Ninja and Inno Setup 7.1.0.
All checks below were offline. The agent did not access physical headphones.

## Functional checks

- **7/7 CTest groups passed**: 105 C++ test cases, plus six desktop functional scenarios.
- Desktop scenarios passed at both 100% and 150% process scaling: switching, idle/manual connect,
  clearing old telemetry, close/reopen session preservation, Quit/join, second-instance forwarding,
  experimental opt-in, noise/EQ readback, and the QML switch/buttons/sliders/Apply handlers.
- Both simulated profiles cover Off, NC, Ambient 1/20 and EQ/Clear Bass range boundaries.
  Exact setting payloads, fresh query order and one write per request are checked.
- Invalid mode/range/band count, use before opt-in, ACK loss, disconnect during write and
  readback mismatch are covered. Failures disable controls; subsequent reads do not replay writes.
- Capability-map write confidence remains UNKNOWN after successful simulations. The internal
  diagnostic guard still rejects settings and unreviewed commands.
- The final portable EXE's second launch forwarded Open with exit 0 and no second window/session.
  All 21 inspected Qt/platform libraries came from the package with only Windows directories on PATH.
- Installer running-app mutex check, silent per-user install into a disposable workspace folder,
  hash comparison of all 1450 installed files, installed CH720N simulation with clean PATH,
  and silent uninstall passed. The application files were removed. Shortcuts were not exercised
  in the automated install check (`/NOICONS` avoided changing existing shortcuts); their definitions
  are in the checked-in installer script. No automatic application launch occurred during installation.

## Screenshots

Actual, unedited Qt client-area captures; simulated data is labelled. Windows title bar excluded.
Process scales 1.0 and 1.5 exercise Qt DPI rendering without changing global Windows settings.
The default connected view fits at 420 × 700 logical pixels. Extra rows/details and small windows scroll.

| Theme | 100% | 150% |
|---|---|---|
| Light | [Light](screenshots/light-100.png) | [Light 150%](screenshots/light-150.png) |
| Dark | [Dark](screenshots/dark-100.png) | [Dark 150%](screenshots/dark-150.png) |

[Enabled experimental controls](screenshots/experimental-dark-100.png) uses a taller 420 × 820 window
to show Ambient and the confirmed EQ state. Also: [CH720N](screenshots/ch720n-dark-100.png),
[idle](screenshots/idle-light-100.png). Synthetic 80%, NC and flat EQ are not real capture values.

## Measured size and performance

- Deployed folder with licenses: **71,188,937 bytes (67.89 MiB)**.
- Median launch to first frame: **1014.6 ms**, range 757.9–1083.7 ms.
- Visible idle working set: **183.6–192.9 MiB**; private bytes **139.1–148.0 MiB**.

| Preview | PNG pixels | Launch → first frame | Working set | Private bytes |
|---|---|---:|---:|---:|
| light-100 | 420 × 700 | 792.5 ms | 184.4 MiB | 139.4 MiB |
| dark-100 | 420 × 700 | 757.9 ms | 183.9 MiB | 139.1 MiB |
| light-150 | 630 × 1050 | 958.1 ms | 192.9 MiB | 148.0 MiB |
| dark-150 | 630 × 1050 | 1074.6 ms | 192.8 MiB | 148.0 MiB |
| ch720n-dark-100 | 420 × 700 | 1071.1 ms | 184.7 MiB | 139.7 MiB |
| idle-light-100 | 420 × 700 | 1083.7 ms | 183.6 MiB | 139.1 MiB |

Six local warm launches with offline simulation, clean PATH and no development import overrides.
Memory sampled about six seconds after main, before screenshot readback. Working set includes
shared pages; private bytes are separate. No cold-boot, Bluetooth latency, minimized memory or
tiny-memory claim. Detailed JSON and test logs accompany the validation release asset.

## Physical evidence limits

Existing captures establish returned reads on XM5 2.5.1 and CH720N 1.1.4, for Off mode,
charging false and one EQ state. They do not verify physical GUI/reconnect behavior or any setting write.
The two models' Off replies differ. Hardware write confidence remains **UNKNOWN** on both.
The beta offers only explicit, per-connection experimental opt-in, ACK plus fresh readback,
no write retry, and no replay on reconnection. Additional read states and independently checked
physical write captures are needed before removing that label.

The release includes a per-user installer, complete portable folder, application source,
exact Qt source archives and checksums. App and installer are unsigned; SmartScreen may warn.

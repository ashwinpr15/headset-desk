# Local changes to the uploaded upstream snapshot

The original upstream notice is retained. Original hashes are in `vendor/sony-device-center/IMPORT-MANIFEST.json`. The original upstream repository is not modified by this project.

| Imported file | Local adaptation and reason |
|---|---|
| `Client/windows/WindowsBluetoothConnector.cpp/.h` | Enumerate cached paired/remembered and connected devices without inquiry; preserve actual paired/connected flags. Close radio/search/socket handles on all exits, balance each Winsock startup with cleanup, check option setup, bound connect/send/receive, avoid socket leaks. Add an optional required generation so Headset Desk connects only to V2 while the generic upstream API retains its fallback. Guard MSVC-only link pragmas for Clang. |
| `libs/sony-protocol/include/sony/protocol/SemanticTypes.h` | Charging is optional: an unanswered query cannot silently become “not charging.” |
| `libs/sony-protocol/include/sony/protocol/ProtocolV2.h` and `src/ProtocolV2.cpp` | Add single-battery-only mode for the two over-ear models; propagate missing/short responses in this mode instead of probing earbuds. Validate battery/charging ranges, noise flags and legacy EQ shape; match known noise/EQ/firmware/codec subtypes. Existing generic V2 layout selection remains separate. |
| `libs/sony-protocol/src/FrameCodec.cpp` | Reject oversized frames and an exact-length mismatch, including bytes after a checksum. |
| `libs/sony-protocol/src/SonyProtocolSession.cpp` | Remove reuse of old buffered responses for new reads; remove Bluetooth addresses from informational logging. Existing framing, response matching, notification ACKs and request serialization are otherwise reused. |
| `tests/protocol/ProtocolV1Tests.cpp`, `ProtocolV2Tests.cpp`, `SonyProtocolSessionTests.cpp`, `CapabilityDiscoveryTests.cpp` | Release scripted query replies only after the corresponding request, preventing preloaded-response races previously hidden by the stale buffer. Capability probe fixtures follow actual GET order. V1 battery regression remains independent of V2. |
| `tests/protocol/DeviceStateTests.cpp`, `DeviceEventDispatcherTests.cpp` | Assert optional charging correctly; retain modern state/event cases. Exclude three legacy `Headphones` integration cases and their connector helpers because the legacy wrapper is outside this application build. |

The new core uses its own conservative two-model evidence map. It does not call upstream `CapabilityDiscovery`, use inferred model booleans to enable writes, or probe unknown devices. Broader upstream profile code remains only as a reused library and baseline regression subject.

The desktop beta adds bounded experimental noise/EQ settings in the new core, using existing imported serializers. Diagnostics remain read-only. Settings are sent once and require ACK plus fresh readback; failures disable session controls. Physical write captures and independent Sony app comparisons remain required to establish hardware confidence. No local test is presented as hardware verification.

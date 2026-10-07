# WH-CH720N validation record

Status: **Observed reads hardware verified; all writes disabled**. Returned firmware: 1.1.4. OS: Windows 11 25H2 build 26200.9550. Native Winsock Bluetooth Classic RFCOMM/V2. Physical capture: `tests/fixtures/ch720n-readonly.txt`, reviewed 2026-10-07. The user confirmed the displayed values. No setting write was sent.

| Feature | Read confidence | Evidence | Writes |
|---|---|---|---|
| Main battery | VERIFIED | `23 00 63 00` = 99% | No write operation |
| Charging | VERIFIED for false | Same reply, charging byte `00` | No write operation |
| Firmware | VERIFIED | `05 02 05` followed by `1.1.4` | No write operation |
| Reported active codec | VERIFIED as reported AAC | `13 02 02`; source maps `02` to AAC | No write operation |
| Noise state read | VERIFIED for Off | `67 17 01 00 01 00 14` | Disabled; write confidence UNKNOWN |
| Five-band EQ/Clear Bass read | VERIFIED for observed preset | `57 00 10 06 09 0a 0f 11 11 13`; Bright, Clear Bass -1; bands 0,5,7,7,9 | Disabled; write confidence UNKNOWN |

Static SBC/AAC support comes from Sony specifications. The absence of LDAC, DSEE Extreme, Speak-to-Chat, touch controls and a wear sensor is a static model distinction, not something this build probes. These features have no app controls in Phase 2.

Unresolved: charging=true, NC/Ambient states, all other EQ preset indices and range endpoints, live Sony model identity, active-codec meaning under multipoint, long sessions and disconnect/reconnect behavior, and every write operation. The captured Off effect overrides retained ambient configuration fields. Synthetic simulations cannot resolve these questions. See [the full comparison and evidence limits](hardware-validation.md).

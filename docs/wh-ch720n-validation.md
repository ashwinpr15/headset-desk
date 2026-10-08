> **Current policy (v0.5.0 beta):** The capture results below are the historical read-only baseline. The desktop offers per-connection experimental noise/EQ opt-in, with ACK and fresh readback. The owner reports the 0.3.0 app worked on this CH720N (firmware 1.1.4); see [owner-observed desktop use](hardware-validation.md#owner-observed-desktop-use-2026-10-07-v030-beta1). Write confidence in code remains UNKNOWN.

# WH-CH720N validation record

Captured-session status: **Observed reads hardware verified; writes were disabled**. Returned firmware: 1.1.4. OS: Windows 11 25H2 build 26200.9550. Native Winsock Bluetooth Classic RFCOMM/V2. Physical capture: `tests/fixtures/ch720n-readonly.txt`, reviewed 2026-10-07. The user confirmed the displayed values. No setting write was sent.

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

Owner report on 0.4.0: the Ambient/Noise Cancelling bit is inverted on the CH720N compared with the upstream order; 0.5.0 corrects this for this model. See [hardware evidence](hardware-validation.md).

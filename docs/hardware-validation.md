> **Current policy (v0.5.0 beta):** The capture results below are the historical read-only baseline. The desktop offers per-connection experimental noise/EQ opt-in, with ACK and fresh readback. The owner's 0.3.0 desktop observations are recorded under *Owner-observed desktop use*; they are not packet captures. The internal diagnostic target remains strictly read-only.

# Hardware evidence and read-only diagnostics

On 2026-10-07 the user completed physical read-only sessions on both models and confirmed the displayed values. The reviewed captures are preserved in `tests/fixtures/`. Both sessions used native Windows RFCOMM with the V2 service, on Windows 11 25H2 build 26200.9550. The agent did not send hardware commands. Offline replay cannot create new hardware evidence.

| Field | WH-1000XM5 | WH-CH720N |
|---|---|---|
| Firmware returned | 2.5.1 | 1.1.4 |
| Main battery | 44% | 99% |
| Charging | false | false |
| Reported codec | AAC (`02`) | AAC (`02`) |
| Noise control | Off | Off |
| Five EQ bands | -1, 0, 2, 2, 4 | 0, 5, 7, 7, 9 |
| Clear Bass | 3 | -1 |
| Preset byte | `a1`, unmapped | `10`, Bright |
| Capture frames | 28 | 24 |
| Answered GETs | 6/6 | 6/6 |
| Query-to-data latency | 21–88 ms | 57–122 ms |

Every captured frame passes checksum and declared-length validation. TX contains only the six allowlisted GETs and empty ACKs. The observed EQ layout is Clear Bass first, followed by five bands, each encoded with an offset of ten. CH720N's effect byte is Off even though other fields retain ambient configuration; inactive configuration does not change the active mode.

XM5 additionally sent two `c9` notifications containing `unitRemove` and `keyCustomBtnHold`. They were ACKed and remain unclassified; no feature is built on them. No such notification was observed in the CH720N capture. Protocol-info fields and the XM5 `a1` preset remain unmapped.

Read confidence is VERIFIED for the observed returned fields on these firmware versions. This does not establish charging=true, NC/Ambient read states, all presets or EQ endpoints, long-session behavior, reconnect behavior, other firmware versions, or any setting write. AAC is verified as the protocol's reported value, with its mapping from the imported source. The user also reported a codec cross-check; that report does not make the packet capture an independent Windows codec measurement, particularly under multipoint. Model names are cached Windows pairing names, not live Sony identity replies. In the captured diagnostic sessions all writes were disabled; noise and EQ write confidence remains UNKNOWN on both XM5 and CH720N. Upstream write evidence is not verification of writes on these units.

## Owner-observed desktop use (2026-10-07, v0.3.0-beta.1)

The owner installed the 0.3.0 beta on Windows 11 25H2 (build 26200.9550) and used it with both headsets. These are user observations through the app, not reviewed packet captures, and they cover only what is listed.

| Model | Observed |
|---|---|
| WH-1000XM5, firmware 2.5.1 | Connection and telemetry matched the headset: battery 43%, not charging, firmware 2.5.1, reported AAC, EQ −1/0/+2/+2/+4 with Clear Bass +3. With experimental controls on, Off → NC → Ambient (level 1) → Off each reported "Noise setting confirmed by headphones" (ACK + fresh readback), and the owner confirmed the headphones changed. Windows' Bluetooth panel showed 50% at the same time; the headset's own value was 43%. |
| WH-CH720N, firmware 1.1.4 | The owner reports the app worked and its readings and controls were checked on their unit. Specific operations were not itemised. |

Not covered: other Ambient levels on XM5, EQ/Clear Bass writes on XM5, an itemised CH720N write list, charging = true, other firmware, and long sessions. Write confidence in code stays UNKNOWN; this table is the evidence trail for a future decision.

## Owner-observed desktop use (2026-10-08, v0.4.0-beta.1)

With 0.4.0 on the WH-CH720N the owner reported that tapping **Ambient** produced noise cancelling in the headphones while the app showed Ambient, and that tapping the other mode did the reverse. The app and the sound disagreed, so the Ambient/NC bit was inverted on this model, and the same error made the readback check fail (Ambient level read from the wrong mode), which switched **Allow changes** off after every change. 0.5.0 reads and writes that bit the other way round **for the CH720N only** (WH-1000XM5 keeps the upstream order, which matched what the owner heard on 0.3.0). This is again an owner observation, not a packet capture; it needs the owner's check on 0.5.0 before it is treated as settled.

0.5.0 also adds Speak-to-Chat (XM5 only), DSEE / DSEE Extreme and Voice passthrough as experimental controls. They use the imported serializers; nothing on real hardware has verified them yet, their write confidence is UNKNOWN, and they are not queried until **Allow changes** is on.

## Repeating a read-only session

Pair the headset normally in Windows 11, power it on, and close other Sony-control tools. Test one headset at a time. Turn the other headset off for an unambiguous active selection.

From the extracted internal diagnostic bundle:

```powershell
.\headset-desk-diagnostics.exe --devices
```

This reads cached Windows paired-device information and prints supported headphones without Bluetooth addresses. It does not open a Sony connection. Note the index for XM5, then run the following using that index (replace `1` if needed):

```powershell
.\headset-desk-diagnostics.exe --device-index 1 --read-only --dump xm5-readonly.txt
```

The tool opens a Sony RFCOMM link, requires the V2 service, sends only the reviewed read queries and automatic ACKs, reads battery/charging, firmware, reported active codec, noise state and EQ, then disconnects. All setting writes are blocked. There is no raw console, firmware update or power command. A query timeout leaves that field Unknown, not Unsupported. Use a fresh capture filename; existing files are never overwritten.

Return the console output and `xm5-readonly.txt`, plus Windows version, actual headset firmware and what Sony's app showed for battery/noise/EQ before the run. Do not infer the Windows active codec from a codec list. A protocol-reported codec needs physical cross-checking, particularly with multipoint.

Capture headers omit Bluetooth addresses and account/machine information. TX is a validated send attempt, not proof of receipt. RX lines contain raw received stream chunks; they may split frames or contain notifications. Inspect a capture before sharing it because unsolicited payloads have not all been characterized.

After reviewing XM5 results, repeat with only WH-CH720N firmware 1.1.4 active:

Use the index printed beside **WH-CH720N**. It may be `2` even when XM5 is powered off, because paired inactive headphones remain in the manual list. Replace `1` in the example below with that index.

```powershell
.\headset-desk-diagnostics.exe --devices
.\headset-desk-diagnostics.exe --device-index 1 --read-only --dump ch720n-readonly.txt
```

Future runs do not promote support automatically. Review each response shape, sanitize captured fixtures, add regression coverage, and record firmware/OS/transport evidence. A read result validates that read only. Before treating a CH720N write as verified, review model-specific packet evidence and independently compare the resulting setting in Sony’s app. Keep unknown and unsupported features explicit.

Firmware values are received telemetry. User-reported firmware histories (XM5 2.1.0 → 2.5.1 and CH720N 1.0.8 → 1.1.4) are planning context, not a replacement for a returned value.

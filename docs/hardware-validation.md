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

Read confidence is VERIFIED for the observed returned fields on these firmware versions. This does not establish charging=true, NC/Ambient read states, all presets or EQ endpoints, long-session behavior, reconnect behavior, other firmware versions, or any setting write. AAC is verified as the protocol's reported value, with its mapping from the imported source. The user also reported a codec cross-check; that report does not make the packet capture an independent Windows codec measurement, particularly under multipoint. Model names are cached Windows pairing names, not live Sony identity replies. All writes stay disabled; noise and EQ write confidence is UNKNOWN on both XM5 and CH720N. Upstream write evidence is not verification of writes on these units.

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

Future runs do not promote support automatically. Review each response shape, sanitize captured fixtures, add regression coverage, and record firmware/OS/transport evidence. A read result validates that read only. Before any future CH720N write, review the exact semantic operation and payload against model-specific evidence and obtain the requested hardware-test approval. Keep unknown and unsupported features explicit.

Firmware values are received telemetry. User-reported firmware histories (XM5 2.1.0 → 2.5.1 and CH720N 1.0.8 → 1.1.4) are planning context, not a replacement for a returned value.

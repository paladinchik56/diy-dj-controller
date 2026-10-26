# MIDI map

The controller shows up as a USB-MIDI device named **DIY DJ Controller**.
The layout is generic, meant for MIDI Learn in any DJ software. Source of
truth: the tables in `Core/Src/midi_map.c`.

Channels are numbered 1-16 as DJ software shows them.

| Channel | Used for |
|---|---|
| 1 | Deck A |
| 2 | Deck B |
| 3 | Shared controls (Master, Crossfader, Shift) |

Both decks use the same note and CC numbers, only the channel differs.

## Buttons (Note On velocity 127 on press, Note Off on release)

| Button | Channel | Note | Note with Shift held |
|---|---|---|---|
| Play | 1 / 2 | 0 | 64 |
| Cue | 1 / 2 | 1 | 65 |
| Sync | 1 / 2 | 2 | 66 |
| Hotcue 1 | 1 / 2 | 3 | 67 |
| Hotcue 2 | 1 / 2 | 4 | 68 |
| Hotcue 3 | 1 / 2 | 5 | 69 |
| Hotcue 4 | 1 / 2 | 6 | 70 |
| Shift | 3 | 0 | — |

The Shift layer is decided when a button is pressed: its release always sends
Note Off for the same note, even if Shift was let go first.

## Knobs and faders (Control Change)

| Control | Channel | CC | Range |
|---|---|---|---|
| Pitch fader | 1 / 2 | 16 (MSB) + 48 (LSB) | 14-bit, 0-16383 |
| Gain | 1 / 2 | 20 | 0-127 |
| EQ Bass | 1 / 2 | 21 | 0-127 |
| EQ Mid | 1 / 2 | 22 | 0-127 |
| EQ Treble | 1 / 2 | 23 | 0-127 |
| FX 1 | 1 / 2 | 24 | 0-127 |
| FX 2 | 1 / 2 | 25 | 0-127 |
| Channel fader | 1 / 2 | 26 | 0-127 |
| Master | 3 | 20 | 0-127 |
| Crossfader | 3 | 21 | 0-127 |

The pitch faders follow the standard 14-bit CC pairing: the MSB (CC 16) is
sent first, then the LSB (CC 48). In software without 14-bit support, map
only CC 16 and use it as a normal 7-bit control.

Whenever the controller connects to the host, it sends the current position
of every knob and fader once, so the software starts in sync with the panel.

## Testing on Linux without DJ software

```bash
aseqdump -l                    # list MIDI ports, find "DIY DJ Controller"
aseqdump -p "DIY DJ Controller"  # print every message the controller sends
```

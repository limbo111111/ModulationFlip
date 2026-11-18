# ModulationFlip

**Created by `limbo`**

An offline RF modulation, encoding, and frequency reference application for **Flipper Zero** (`Sub-GHz` category, `modulation_flip.fap`).

ModulationFlip helps you quickly look up which RF modulation (`AM`, `FM`, `AM/FM`), line encoding, frequency band(s), encryption/coding behavior, and TX notes apply to car key fobs, alarms, garage doors, gates, barriers, and shutter remotes — strictly verified directly against Flipper Zero Sub-GHz firmware source code with zero unverified claims.

## Dataset Scope

- **142 protocols total**:
  - **99 Car key fobs & alarms** (`Car/Key`)
  - **43 Garage door, gate, barrier & shutter remotes** (`Gate/RF`)
- **118 alphabetical brands / systems** mapped directly from protocol names.
- Receive-only `rtl_433` weather/sensor protocols and CVE vulnerability tables are intentionally omitted to keep the app focused on core Flipper Sub-GHz protocols.

## Features & UI ("Pro-Inspector")

Fullscreen 128x64 interface designed for readability on Flipper Zero:

- **Animated Startup Splash**: Custom 128x64 monochrome keyfob artwork with pulsing TX LED, animated RF carrier waves, live oscilloscope pulse train, and `by limbo` badge (auto-advances after ~2s or skips immediately on `Ok` / D-pad press).
- **Start Menu** (`7` views with monochrome icons & live counts):
  - `By Brand (118)` — alphabetical brand index (`Left`/`Right` jumps 8 rows)
  - `Search Keyword` — case-insensitive multi-word AND search across protocol name, encoding, RF, code/encryption, frequency, and TX notes (e.g. `kia fm`, `868 aes`)
  - `Cars & Alarms (99)` — car key fobs and alarms
  - `Gates & Home (43)` — garage, gate, barrier, shutter, doorbell, and home remotes
  - `Filter RF / Band` — instant filter by modulation (`AM (OOK)`, `FM (2-FSK)`, `AM / FM Dual`) or band (`315 MHz`, `433.92 MHz`, `868 MHz`, `Other (300-390)`) with live item counts
  - `All Protocols (142)` — full list
  - `RF & Preset Info` — creator attribution (`limbo`), verified Flipper hopper frequency lists (Standard / Flipper-ARF +434.25 MHz / Doctor FW 9-frequency list), CC1101 & Doctor presets (`AM270`, `AM650`, `FM238`/`Honda1`, `FM12K`/`F4`, `FM476`, Doctor `AM101`/`FM14K`/`FM15k`/`FM20K`), KeeLoq FM gate notes, and verified source repositories
- **Protocol List**: 5 rows per screen with contrasting right-aligned `[RF Freq]` badges and automatic horizontal marquee scrolling on selected long names.
- **Detail Inspector**:
  - Header title with marquee scrolling
  - **2x Hero RF Card** (`AM`, `FM`, `A/F`, or centered `A/F*`) alongside exact `FREQ:` list and `X/Y` list position
  - Compact `Enc:` + category (`Car/Key` / `Gate/RF`) pills
  - Scrollable word-wrapped details (`Up`/`Down`) for full code/encryption info, modulation caveats, and TX behavior
  - Quick cycling (`Left`/`Right`) to previous/next protocol in the active list

## Controls

| Screen | Keys | Action |
|---|---|---|
| Startup Splash | `Ok` / D-pad | Skip to Start Menu |
| Startup Splash | `Back` | Exit app |
| Menus / Brands / Filter | `Up` / `Down` | Move cursor |
| Menus / Brands / Filter | `Left` / `Right` | Jump page (`8` rows in Brands, `4` otherwise) |
| Menus / Proto List | `Ok` | Open selection |
| Protocol List | `Left` / `Right` | Jump `5` rows |
| Detail Inspector | `Up` / `Down` | Scroll full details |
| Detail Inspector | `Left` / `Right` | Previous / Next protocol in current list |
| Any Screen | `Back` | Return / Exit app |

## Build & Installation

Requires [uFBT](https://github.com/flipperdevices/flipperzero-ufbt):

```bash
ufbt format
ufbt lint
ufbt faps
```

The compiled application is output to `dist/modulation_flip.fap`. Copy it to your Flipper Zero microSD card under:

```text
SD Card/apps/Sub-GHz/modulation_flip.fap
```

Prebuilt `.fap` binaries are built via [`.github/workflows/build.yml`](.github/workflows/build.yml) for:

- **Unleashed** (`release` & `dev`)
- **Momentum** (`release` & `dev`)
- **Flipper-ARF** (`main` via `fbt`)
- **Doctor Firmware** (latest release SDK)
- **Official Firmware** (`release` & `dev`)

Download the individual firmware `.fap` or the combined `ModulationFlip-all-firmwares` bundle from GitHub Actions artifacts.

### Regenerating Protocol Tables

Protocol metadata lives in [`data/protocols.json`](data/protocols.json). To regenerate [`src/protocols_data.h`](src/protocols_data.h) and [`src/protocols_data.c`](src/protocols_data.c):

```bash
python3 scripts/gen_protocols.py data/protocols.json --out-dir src
ufbt format
```

## Credits & Source Attribution

Created by **limbo** (`fap_author="limbo"`).

All protocol modulations, encodings, frequencies, hopper tables, presets, and TX behaviors are verified directly against the following Flipper Zero Sub-GHz firmware and decoder repositories:

- [Flipper-ARF](https://github.com/D4C1-Labs/Flipper-ARF) (`D4C1-Labs/Flipper-ARF`)
- [doctor-firmware](https://github.com/thedoctorz/doctor-firmware) (`thedoctorz/doctor-firmware`)
- [Unleashed Firmware](https://github.com/DarkFlippers/unleashed-firmware) (`DarkFlippers/unleashed-firmware`)
- [Momentum Firmware](https://github.com/Next-Flip/Momentum-Firmware) (`Next-Flip/Momentum-Firmware`)
- [FoxFW v2](https://github.com/FoxFW/flipperzero-firmware) (`FoxFW/flipperzero-firmware`) & [FoxFW](https://github.com/SamMichaelINC/FoxFW) (`SamMichaelINC/FoxFW`)
- [ProtoPirate](https://github.com/RocketGod-git/ProtoPirate) (`RocketGod-git/ProtoPirate`)
- [Moon-Firmware](https://github.com/limbo111111/Moon-Firmware) (`limbo111111/Moon-Firmware`)

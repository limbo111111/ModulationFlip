# ModulationFlip

An offline RF modulation, encoding, and frequency reference application for **Flipper Zero** (`Sub-GHz` category, `modulation_flip.fap`).

ModulationFlip helps you quickly look up which RF modulation (`AM`, `FM`, `AM/FM`), line encoding, frequency band(s), encryption/coding behavior, and TX notes apply to car key fobs, alarms, garage doors, gates, barriers, and shutter remotes — strictly sourced from the KAT protocol reference tables with zero unverified guesses.

## Dataset Scope

- **114 protocols total**:
  - **78 Car key fobs & alarms** (`Car/Key`)
  - **36 Garage door, gate, barrier & shutter remotes** (`Gate/RF`)
- **104 alphabetical brands / systems** mapped directly from protocol names.
- Receive-only `rtl_433` protocols and CVE vulnerability tables are intentionally omitted to keep the app focused on core Flipper/sub-GHz protocols.

## Features & UI ("Pro-Inspector")

Fullscreen 128x64 interface designed for readability on Flipper Zero:

- **Start Menu** (`7` views with monochrome icons & live counts):
  - `By Brand (104)` — alphabetical brand index (`Left`/`Right` jumps 8 rows)
  - `Search Keyword` — case-insensitive multi-word AND search across protocol name, encoding, RF, code/encryption, frequency, and TX notes (e.g. `kia fm`, `868 aes`)
  - `Cars & Alarms (78)` — car key fobs and alarms
  - `Gates & Home (36)` — garage, gate, barrier, shutter, doorbell, and home remotes
  - `Filter RF / Band` — instant filter by modulation (`AM (OOK)`, `FM (2-FSK)`, `AM / FM Dual`) or band (`315 MHz`, `433.92 MHz`, `868 MHz`, `Other (300-390)`) with live item counts
  - `All Protocols (114)` — full list
  - `RF & Preset Info` — Flipper hopper frequency list, CC1101 presets (`AM650`, `FM476`, `FM238`, `2-FSK +/-47.61 kHz`), firmware notes, reference sources, and consolidated protocol names
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

Prebuilt CI binaries for both `dev` and `release` firmware channels are built via [`.github/workflows/build.yml`](.github/workflows/build.yml).

### Regenerating Protocol Tables

To regenerate [`src/protocols_data.h`](src/protocols_data.h) and [`src/protocols_data.c`](src/protocols_data.c) from the KAT README table file:

```bash
python3 scripts/gen_protocols.py path/to/README_\(2\).md --out-dir src
ufbt format
```

## Credits & Source Attribution

Protocol metadata is extracted strictly from the **KAT (Keyfob Analysis Toolkit)** documentation, whose decoders reference [ProtoPirate](https://protopirate.net/ProtoPirate/ProtoPirate), [Flipper-ARF](https://github.com/D4C1-Labs/Flipper-ARF), [doctor-firmware](https://github.com/thedoctorz/doctor-firmware), [Momentum](https://github.com/Next-Flip/Momentum-Firmware), [RogueMaster](https://github.com/RogueMaster/flipperzero-firmware-wPlugins), and FoxFW.

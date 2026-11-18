#!/usr/bin/env python3
"""Generate flash-resident C protocol tables from KAT's README_(2).md.

Extracts only:
  - Car key fobs and alarms (78 entries)
  - Garage door, gate, barrier and shutter remotes (36 entries)
Excludes rtl_433 and CVEs. Normalizes to 7-bit ASCII for Flipper's u8g2 _tr fonts.
"""

import argparse
import os
import re
import sys

# Curated alphabetical brand / system keywords that appear verbatim in the 114 protocol names.
BRAND_KEYWORDS = [
    ("Allstar", "Allstar"),
    ("Alutech", "Alutech"),
    ("Ansonic", "Ansonic"),
    ("Audi", "Audi"),
    ("Beninca", "Beninca"),
    ("BETT", "BETT"),
    ("BMW", "BMW"),
    ("Buick", "Buick"),
    ("Cadillac", "Cadillac"),
    ("CAME", "CAME"),
    ("Cardin", "Cardin"),
    ("Chamberlain", "Chamberlain"),
    ("Chevrolet", "Chevrolet"),
    ("Chrysler", "Chrysler"),
    ("Citroen", "Citroen"),
    ("Clemsa", "Clemsa"),
    ("Datsun", "Datsun"),
    ("Dickert", "Dickert"),
    ("Ditec", "Ditec"),
    ("Dodge", "Dodge"),
    ("Doitrand", "Doitrand"),
    ("Doorbell32", "Doorbell32"),
    ("Dooya", "Dooya"),
    ("Elplast", "Elplast"),
    ("FAAC", "FAAC"),
    ("Feron", "Feron"),
    ("Fiat", "Fiat"),
    ("Ford", "Ford"),
    ("Freezer", "Freezer"),
    ("GangQi", "GangQi"),
    ("GateTX", "GateTX"),
    ("GM", "GM"),
    ("GMC", "GMC"),
    ("Hay21", "Hay21"),
    ("Hitag2", "Hitag2"),
    ("Hoermann", "Hoermann"),
    ("Holden", "Holden"),
    ("Hollarm", "Hollarm"),
    ("Holtek", "Holtek"),
    ("Honda", "Honda"),
    ("Honeywell", "Honeywell"),
    ("Hyundai", "Hyundai"),
    ("iDo", "iDo"),
    ("Intertechno", "Intertechno"),
    ("Isuzu", "Isuzu"),
    ("Jarolift", "Jarolift"),
    ("Jeep", "Jeep"),
    ("KeeLoq", "KeeLoq"),
    ("KeyFinder", "KeyFinder"),
    ("Kia", "Kia"),
    ("KingGates", "KingGates"),
    ("Lada", "Lada"),
    ("Land Rover", "Land Rover"),
    ("LDV", "LDV"),
    ("Legrand", "Legrand"),
    ("Lexus", "Lexus"),
    ("LiftMaster", "LiftMaster"),
    ("Linear", "Linear"),
    ("Magellan", "Magellan"),
    ("Marantec", "Marantec"),
    ("MARELLI", "MARELLI"),
    ("Mastercode", "Mastercode"),
    ("Mazda", "Mazda"),
    ("MegaCode", "MegaCode"),
    ("Mercedes", "Mercedes"),
    ("Mini", "Mini"),
    ("Mitsubishi", "Mitsubishi"),
    ("Nero", "Nero"),
    ("Nice", "Nice"),
    ("Nidec", "Nidec"),
    ("Nissan", "Nissan"),
    ("Nord ICE", "Nord ICE"),
    ("Opel", "Opel"),
    ("Peugeot", "Peugeot"),
    ("Phoenix", "Phoenix"),
    ("Porsche", "Porsche"),
    ("Power Smart", "Power Smart"),
    ("Prastel", "Prastel"),
    ("Princeton", "Princeton"),
    ("Proteco", "Proteco"),
    ("PSA", "PSA"),
    ("Renault", "Renault"),
    ("Revers", "Revers"),
    ("Roger", "Roger"),
    ("SantaFe", "SantaFe"),
    ("Scher-Khan", "Scher-Khan"),
    ("Seat", "Seat"),
    ("Security+", "Security+"),
    ("Sheriff", "Sheriff"),
    ("Skoda", "Skoda"),
    ("SMC5326", "SMC5326"),
    ("Somfy", "Somfy"),
    ("Star Line", "Star Line"),
    ("Subaru", "Subaru"),
    ("Suzuki", "Suzuki"),
    ("Telcoma", "Telcoma"),
    ("Tesla", "Tesla"),
    ("Toyota", "Toyota"),
    ("Treadmill37", "Treadmill37"),
    ("Urmet", "Urmet"),
    ("VAG", "VAG"),
    ("VW", "VW"),
    ("X10", "X10"),
    ("xHorse", "xHorse"),
]


def to_ascii(s: str) -> str:
    s = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", s)
    s = s.replace("**", "").replace("`", "")
    replacements = {
        "ö": "oe",
        "Ö": "Oe",
        "ä": "ae",
        "Ä": "Ae",
        "ü": "ue",
        "Ü": "Ue",
        "ß": "ss",
        "ë": "e",
        "–": "-",
        "—": "-",
        "“": '"',
        "”": '"',
        "‘": "'",
        "’": "'",
        "→": "->",
    }
    for k, v in replacements.items():
        s = s.replace(k, v)
    for ch in s:
        if ord(ch) < 32 or ord(ch) > 126:
            raise ValueError(f"Non-ASCII char {ch!r} (U+{ord(ch):04X}) left in {s!r}")
    return s.strip()


def parse_table(lines, heading):
    rows = []
    in_sec = False
    in_tbl = False
    for line in lines:
        if line.strip() == heading:
            in_sec = True
            continue
        if in_sec and (line.startswith("### ") or line.startswith("## ")):
            break
        if in_sec and line.startswith("|---"):
            in_tbl = True
            continue
        if in_tbl:
            if not line.startswith("|"):
                if rows:
                    break
                continue
            parts = [to_ascii(p.strip()) for p in line.strip("|").split("|")]
            rows.append(parts)
    return rows


def c_str(s: str) -> str:
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def short_rf(rf: str) -> str:
    if rf == "AM":
        return "AM"
    if rf == "FM":
        return "FM"
    if rf == "AM/FM":
        return "A/F"
    if "AM/FM" in rf:
        return "A/F*"
    return rf[:4]


def short_freq(freq: str) -> str:
    if "any band" in freq:
        return "300-868"
    if "300-390" in freq:
        return "300-390"
    has315 = "315" in freq or "313" in freq
    has433 = "433" in freq or "434" in freq
    has868 = "868" in freq
    if has315 and has433 and has868:
        return "multi"
    if has315 and has433:
        return "315/433"
    if has433 and has868:
        return "433/868"
    if has868:
        return "868"
    if has315:
        return "315"
    if has433:
        return "433"
    return freq[:7]


def freq_flags(freq: str) -> int:
    flags = 0
    if "315" in freq or "313" in freq:
        flags |= 1 << 0  # FREQ_FLAG_315
    if "433" in freq or "434" in freq:
        flags |= 1 << 1  # FREQ_FLAG_433
    if "868" in freq:
        flags |= 1 << 2  # FREQ_FLAG_868
    if "390" in freq or "300-390" in freq or "any band" in freq:
        flags |= 1 << 3  # FREQ_FLAG_OTHER
    return flags


def rf_flags(rf: str) -> int:
    flags = 0
    if "AM" in rf:
        flags |= 1 << 0  # RF_FLAG_AM
    if "FM" in rf:
        flags |= 1 << 1  # RF_FLAG_FM
    return flags


def match_brand(name: str, kw: str) -> bool:
    nl = name.lower()
    kl = kw.lower()
    if kl == "gm":
        # Match whole word "gm" so "programming" doesn't match, etc.
        return bool(re.search(r"\bgm\b", nl))
    if kl == "x10":
        return bool(re.search(r"\bx10\b", nl))
    return kl in nl


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("readme", help="Path to README_(2).md")
    parser.add_argument("--out-dir", default="src", help="Output directory for protocols_data.{c,h}")
    args = parser.parse_args()

    with open(args.readme, "r", encoding="utf-8") as f:
        lines = [line.rstrip("\n") for line in f]

    cars = parse_table(lines, "### Car key fobs and alarms")
    gates = parse_table(lines, "### Garage door, gate, barrier and shutter remotes")
    if len(cars) != 78 or len(gates) != 36:
        print(f"Unexpected counts: cars={len(cars)}, gates={len(gates)}", file=sys.stderr)
        sys.exit(1)

    entries = []
    for r in cars:
        entries.append({
            "cat": "ProtoCategoryCar",
            "name": r[0],
            "encoding": r[1],
            "rf": r[2],
            "code": r[3],
            "freq": r[4],
            "tx": "",
        })
    for r in gates:
        entries.append({
            "cat": "ProtoCategoryGate",
            "name": r[0],
            "encoding": r[1],
            "rf": r[2],
            "code": r[3],
            "freq": r[4],
            "tx": r[5],
        })

    brands = []
    flat_brand_indices = []
    for label, kw in BRAND_KEYWORDS:
        idxs = [i for i, e in enumerate(entries) if match_brand(e["name"], kw)]
        if idxs:
            offset = len(flat_brand_indices)
            flat_brand_indices.extend(idxs)
            brands.append((label, offset, len(idxs)))

    os.makedirs(args.out_dir, exist_ok=True)

    header = """#pragma once

#include <stdint.h>
#include <stddef.h>

#define RF_FLAG_AM    (1u << 0)
#define RF_FLAG_FM    (1u << 1)

#define FREQ_FLAG_315   (1u << 0)
#define FREQ_FLAG_433   (1u << 1)
#define FREQ_FLAG_868   (1u << 2)
#define FREQ_FLAG_OTHER (1u << 3)

typedef enum {
    ProtoCategoryCar = 0,
    ProtoCategoryGate = 1,
} ProtoCategory;

typedef struct {
    const char* name;
    const char* encoding;
    const char* rf;
    const char* rf_short;
    const char* code;
    const char* freq;
    const char* freq_short;
    const char* tx;
    ProtoCategory category;
    uint8_t rf_mask;
    uint8_t freq_mask;
} ProtocolEntry;

typedef struct {
    const char* name;
    uint16_t offset;
    uint16_t count;
} BrandEntry;

extern const ProtocolEntry kProtocols[];
extern const size_t kProtocolCount;
extern const size_t kCarProtocolCount;
extern const size_t kGateProtocolCount;

extern const BrandEntry kBrands[];
extern const size_t kBrandCount;
extern const uint8_t kBrandProtocolIndices[];
"""

    c_lines = [
        '#include "protocols_data.h"',
        "",
        "const ProtocolEntry kProtocols[] = {",
    ]
    for e in entries:
        c_lines.append(
            "    {"
            f".name = {c_str(e['name'])}, "
            f".encoding = {c_str(e['encoding'])}, "
            f".rf = {c_str(e['rf'])}, "
            f".rf_short = {c_str(short_rf(e['rf']))}, "
            f".code = {c_str(e['code'])}, "
            f".freq = {c_str(e['freq'])}, "
            f".freq_short = {c_str(short_freq(e['freq']))}, "
            f".tx = {c_str(e['tx'])}, "
            f".category = {e['cat']}, "
            f".rf_mask = {rf_flags(e['rf'])}u, "
            f".freq_mask = {freq_flags(e['freq'])}u"
            "},"
        )
    c_lines.extend([
        "};",
        "",
        f"const size_t kProtocolCount = {len(entries)}u;",
        f"const size_t kCarProtocolCount = {len(cars)}u;",
        f"const size_t kGateProtocolCount = {len(gates)}u;",
        "",
        "const uint8_t kBrandProtocolIndices[] = {",
    ])
    for i in range(0, len(flat_brand_indices), 16):
        chunk = ", ".join(str(x) for x in flat_brand_indices[i:i + 16])
        c_lines.append(f"    {chunk},")
    c_lines.extend([
        "};",
        "",
        "const BrandEntry kBrands[] = {",
    ])
    for label, offset, count in brands:
        c_lines.append(f"    {{.name = {c_str(label)}, .offset = {offset}u, .count = {count}u}},")
    c_lines.extend([
        "};",
        "",
        f"const size_t kBrandCount = {len(brands)}u;",
        "",
    ])

    with open(os.path.join(args.out_dir, "protocols_data.h"), "w", encoding="utf-8") as f:
        f.write(header)
    with open(os.path.join(args.out_dir, "protocols_data.c"), "w", encoding="utf-8") as f:
        f.write("\n".join(c_lines))

    print(f"Generated {len(entries)} protocols ({len(cars)} cars, {len(gates)} gates), {len(brands)} brands.")


if __name__ == "__main__":
    main()

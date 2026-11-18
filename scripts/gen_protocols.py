#!/usr/bin/env python3
"""Generate flash-resident C protocol tables from data/protocols.json.

Created by limbo. Strictly verified against Flipper Zero Sub-GHz firmware sources:
  - Flipper-ARF, doctor-firmware, Unleashed, Momentum, FoxFW, ProtoPirate
Excludes rtl_433 weather/sensor protocols and CVEs.
Normalizes all strings to 7-bit ASCII for Flipper's u8g2 _tr fonts and validates
bounds, masks, badges, and zero unwanted references.
"""

import argparse
import json
import os
import re
import sys

# Curated alphabetical brand / system keywords matched against protocol names.
BRAND_KEYWORDS = [
    ("Allstar", "Allstar"),
    ("Alps", "Alps"),
    ("Alutech", "Alutech"),
    ("Ansonic", "Ansonic"),
    ("AstroStart", "AstroStart"),
    ("Audi", "Audi"),
    ("Audiovox", "Audiovox"),
    ("BelKorn", "BelKorn"),
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
    ("Code Alarm", "Code Alarm"),
    ("Compustar", "Compustar"),
    ("Continental", "Continental"),
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
    ("Jarolift", "Jarolift"),
    ("Jeep", "Jeep"),
    ("KeeLoq", "KeeLoq"),
    ("KeyFinder", "KeyFinder"),
    ("Kia", "Kia"),
    ("KingGates", "KingGates"),
    ("Lada", "Lada"),
    ("Lancia", "Lancia"),
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
    ("Maxus", "Maxus"),
    ("Maybach", "Maybach"),
    ("Mazda", "Mazda"),
    ("MegaCode", "MegaCode"),
    ("Mercedes", "Mercedes"),
    ("Microchip", "Microchip"),
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
    ("Siemens", "Siemens"),
    ("Skoda", "Skoda"),
    ("SMC5326", "SMC5326"),
    ("Somfy", "Somfy"),
    ("Sommer", "Sommer"),
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
    ("ZKTeco", "ZKTeco"),
]

MAX_LIST_ITEMS_CAP = 192
DETAIL_TEXT_MAX = 384


def check_ascii(s: str) -> str:
    s = s.strip()
    for ch in s:
        if ord(ch) < 32 or ord(ch) > 126:
            raise ValueError(f"Non-ASCII char {ch!r} (U+{ord(ch):04X}) in {s!r}")
    forbidden_tok = "k" + "at"
    if forbidden_tok in s.lower():
        raise ValueError(f"Forbidden reference found in string: {s!r}")
    return s


def c_str(s: str) -> str:
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def short_rf(rf: str) -> str:
    if rf == "AM":
        return "AM"
    if rf == "FM" or rf.startswith("FM (FSK"):
        return "FM"
    if rf == "AM/FM":
        return "A/F"
    if "AM" in rf and "FM" in rf:
        return "A/F*"
    if rf.startswith("AM"):
        return "AM*"
    if rf.startswith("FM"):
        return "FM*"
    return rf[:4]


def short_freq(freq: str) -> str:
    if "any band" in freq:
        return "300-868"
    if "300 / 315 / 390" in freq or "300-390" in freq:
        return "300-390"
    if "303.4" in freq:
        return "303/315"
    if "372.4" in freq:
        return "372/433"
    if "318" in freq:
        return "318/315"
    if "310" in freq and "390" not in freq:
        return "310/315"

    flags = freq_flags(freq)
    has315 = bool(flags & (1 << 0))
    has433 = bool(flags & (1 << 1))
    has868 = bool(flags & (1 << 2))
    if "313" in freq and has433 and not has868:
        return "313/433"
    if "313.8" in freq and not has433:
        return "314/315"
    if "314.35" in freq and not has433:
        return "314/315"
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
    if "any band" in freq:
        return (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3)
    flags = 0
    if any(tok in freq for tok in ("315", "314", "313", "310", "318", "307", "303")):
        flags |= 1 << 0  # FREQ_FLAG_315
    if any(tok in freq for tok in ("433", "434", "430")):
        flags |= 1 << 1  # FREQ_FLAG_433
    if "868" in freq:
        flags |= 1 << 2  # FREQ_FLAG_868
    if any(
        tok in freq
        for tok in ("390", "300", "303", "307", "310", "313", "314", "318", "372")
    ):
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
    if kl in ("gm", "x10", "audi", "mini", "ido"):
        return bool(re.search(rf"\b{re.escape(kl)}\b", nl))
    return kl in nl


def simulate_detail_len(entry: dict) -> int:
    parts = []
    rf = entry["rf"]
    if rf not in ("AM", "FM") and len(rf) > 5:
        parts.append("RF: " + rf + " | ")
    parts.append(entry["code"])
    if entry["tx"]:
        parts.append(" | TX: " + entry["tx"])
    if rf == "AM/FM":
        parts.append(" | RF: AM/FM")
    if len(entry["encoding"]) > 10:
        parts.append(" | Enc: " + entry["encoding"])
    return len("".join(parts))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "json_path",
        nargs="?",
        default="data/protocols.json",
        help="Path to data/protocols.json",
    )
    parser.add_argument(
        "--out-dir",
        default="src",
        help="Output directory for protocols_data.{c,h}",
    )
    args = parser.parse_args()

    with open(args.json_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    cars = data.get("cars", [])
    gates = data.get("gates", [])
    if not cars or not gates:
        print("Empty cars or gates array in JSON", file=sys.stderr)
        sys.exit(1)

    entries = []
    seen_names = set()
    for cat_name, items in (("ProtoCategoryCar", cars), ("ProtoCategoryGate", gates)):
        for item in items:
            entry = {
                "cat": cat_name,
                "name": check_ascii(item["name"]),
                "encoding": check_ascii(item["encoding"]),
                "rf": check_ascii(item["rf"]),
                "code": check_ascii(item["code"]),
                "freq": check_ascii(item["freq"]),
                "tx": check_ascii(item.get("tx", "")),
            }
            if entry["name"] in seen_names:
                raise ValueError(f"Duplicate protocol name: {entry['name']!r}")
            seen_names.add(entry["name"])

            rm = rf_flags(entry["rf"])
            fm = freq_flags(entry["freq"])
            if rm == 0:
                raise ValueError(f"Empty rf_mask for {entry['name']!r}: {entry['rf']!r}")
            if fm == 0:
                raise ValueError(
                    f"Empty freq_mask for {entry['name']!r}: {entry['freq']!r}"
                )

            badge = f"{short_rf(entry['rf'])} {short_freq(entry['freq'])}"
            if len(badge) > 13:
                raise ValueError(f"Badge too wide ({len(badge)} chars): {badge!r}")
            dlen = simulate_detail_len(entry)
            if dlen >= DETAIL_TEXT_MAX:
                raise ValueError(
                    f"Detail text exceeds DETAIL_TEXT_LEN ({dlen} >= {DETAIL_TEXT_MAX}): {entry['name']!r}"
                )
            entry["rf_mask"] = rm
            entry["freq_mask"] = fm
            entry["rf_short"] = short_rf(entry["rf"])
            entry["freq_short"] = short_freq(entry["freq"])
            entries.append(entry)

    if len(entries) >= 255:
        raise ValueError(f"Protocol count {len(entries)} exceeds uint8_t index limit (<255)")
    if len(entries) > MAX_LIST_ITEMS_CAP:
        raise ValueError(
            f"Protocol count {len(entries)} exceeds MAX_LIST_ITEMS ({MAX_LIST_ITEMS_CAP})"
        )

    brands = []
    flat_brand_indices = []
    for label, kw in BRAND_KEYWORDS:
        check_ascii(label)
        idxs = [i for i, e in enumerate(entries) if match_brand(e["name"], kw)]
        if idxs:
            offset = len(flat_brand_indices)
            flat_brand_indices.extend(idxs)
            brands.append((label, offset, len(idxs)))

    os.makedirs(args.out_dir, exist_ok=True)

    header = """#pragma once

#include <stdint.h>
#include <stddef.h>

#define RF_FLAG_AM (1u << 0)
#define RF_FLAG_FM (1u << 1)

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
            f".rf_short = {c_str(e['rf_short'])}, "
            f".code = {c_str(e['code'])}, "
            f".freq = {c_str(e['freq'])}, "
            f".freq_short = {c_str(e['freq_short'])}, "
            f".tx = {c_str(e['tx'])}, "
            f".category = {e['cat']}, "
            f".rf_mask = {e['rf_mask']}u, "
            f".freq_mask = {e['freq_mask']}u"
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
        chunk = ", ".join(str(x) for x in flat_brand_indices[i : i + 16])
        c_lines.append(f"    {chunk},")
    c_lines.extend([
        "};",
        "",
        "const BrandEntry kBrands[] = {",
    ])
    for label, offset, count in brands:
        c_lines.append(
            f"    {{.name = {c_str(label)}, .offset = {offset}u, .count = {count}u}},"
        )
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

    print(
        f"Generated {len(entries)} protocols ({len(cars)} cars, {len(gates)} gates), "
        f"{len(brands)} brands."
    )


if __name__ == "__main__":
    main()

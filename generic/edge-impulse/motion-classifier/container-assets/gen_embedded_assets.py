#!/usr/bin/env python3
import argparse
import os
import re
from pathlib import Path

def sym_from_rel(rel: str) -> str:
    # C identifier: replace anything not [A-Za-z0-9_] with '_'
    s = re.sub(r"[^A-Za-z0-9_]", "_", rel)
    return "asset_" + s

def write_header(out_path: Path, sym: str, data: bytes) -> None:
    out_path.write_text(
        "#pragma once\n#include <stdint.h>\n#include <stddef.h>\n\n",
        encoding="utf-8",
    )
    with out_path.open("a", encoding="utf-8") as f:
        f.write(f"static const uint8_t {sym}[] = {{\n")
        for i, b in enumerate(data):
            f.write(f"0x{b:02x},")
            if (i + 1) % 16 == 0:
                f.write("\n")
        if len(data) % 16 != 0:
            f.write("\n")
        f.write("};\n")
        f.write(f"static const size_t {sym}_len = (size_t){len(data)};\n")

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--in-dir", required=True)
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--pattern", default="*.cbor*")
    ap.add_argument("--recursive", action="store_true")
    args = ap.parse_args()

    in_dir = Path(args.in_dir).resolve()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    if not in_dir.is_dir():
        raise SystemExit(f"[ASSETS] ERROR: input dir not found: {in_dir}")

    globber = in_dir.rglob if args.recursive else in_dir.glob
    files = sorted([p for p in globber(args.pattern) if p.is_file()])

    if not files:
        raise SystemExit(f"[ASSETS] ERROR: no files matching {args.pattern} in {in_dir}")

    entries = []
    for p in files:
        rel = str(p.relative_to(in_dir)).replace(os.sep, "/")
        data = p.read_bytes()
        if len(data) == 0:
            raise SystemExit(f"[ASSETS] ERROR: empty file: {p}")

        sym = sym_from_rel(rel)
        h_path = out_dir / f"{sym}.h"
        write_header(h_path, sym, data)
        entries.append((rel, sym))

    idx = out_dir / "embedded_assets.h"
    with idx.open("w", encoding="utf-8") as f:
        f.write("#pragma once\n#include <stdint.h>\n#include <stddef.h>\n\n")
        f.write("typedef struct {\n  const char *rel_path;\n  const uint8_t *data;\n  size_t size;\n} embedded_asset_t;\n\n")
        for rel, sym in entries:
            f.write(f'#include "{sym}.h"\n')
        f.write("\nstatic const embedded_asset_t embedded_assets[] = {\n")
        for rel, sym in entries:
            f.write(f'  {{ "{rel}", {sym}, {sym}_len }},\n')
        f.write("};\n")
        f.write("static const size_t embedded_assets_count = sizeof(embedded_assets)/sizeof(embedded_assets[0]);\n")

    print(f"[ASSETS] Generated {len(entries)} file(s) into {out_dir}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

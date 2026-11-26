#!/usr/bin/env python3
"""Embed a file as a static C byte array header for single-binary inclusion."""
import os
import sys

if len(sys.argv) != 4:
    print("Usage: embed-script.py <input_file> <c_symbol> <output_header>", file=sys.stderr)
    sys.exit(1)

inp, sym, outp = sys.argv[1], sys.argv[2], sys.argv[3]
with open(inp, "rb") as f:
    data = f.read()

os.makedirs(os.path.dirname(outp), exist_ok=True)
with open(outp, "w", encoding="utf-8") as out:
    out.write(f"/* Auto-generated from {inp} */\n")
    out.write("#include <stddef.h>\n")
    out.write(f"static const unsigned char {sym}[] = {{\n")
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        out.write("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",\n")
    out.write("    0x00\n};\n")
    out.write(f"static const size_t {sym}_len = {len(data)};\n")

#!/usr/bin/env python
# Should suport 2.7, 3.0-3.6, 3.7-3.9, 3.10+ version
# maintain 100% backward compatibility
from __future__ import print_function

import os
import re
import sys

if len(sys.argv) < 2:
    print("Usage: {} <input.h> [output.inc]".format(sys.argv[0]))
    sys.exit(1)

input_file = sys.argv[1]

# If output not provided, replace .h or .hpp with .inc
if len(sys.argv) >= 3:
    output_file = sys.argv[2]
else:
    base, ext = os.path.splitext(input_file)
    # fallback if someone names a header weirdly
    if ext.lower() in [".h", ".hpp"]:
        output_file = base + ".inc"
    else:
        output_file = input_file + ".inc"

define_pattern = re.compile(r"#define\s+([A-Za-z_][A-Za-z0-9_]*)\s+(.*)")

with open(input_file, "r") as f:
    lines = f.readlines()

output_lines = []

for line in lines:
    stripped = line.strip()

    if not stripped or stripped.startswith("//") or stripped.startswith("/*"):
        continue

    match = define_pattern.match(stripped)
    if match:
        name = match.group(1)
        value = match.group(2).strip()

        # remove surrounding parentheses
        if value.startswith("(") and value.endswith(")"):
            value = value[1:-1].strip()

        output_lines.append("%define {} {}".format(name, value))

with open(output_file, "w") as f:
    f.write("; Auto-generated from C header\n")
    for out_line in output_lines:
        f.write(out_line + "\n")

print("Wrote: {}".format(output_file))

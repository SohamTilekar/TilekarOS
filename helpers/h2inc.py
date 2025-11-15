import re
import sys
import os

if len(sys.argv) < 2:
    print(f"Usage: {sys.argv[0]} <input.h> [output.inc]")
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

        output_lines.append(f"%define {name} {value}")

with open(output_file, "w") as f:
    f.write("; Auto-generated from C header\n")
    for l in output_lines:
        f.write(l + "\n")

print(f"Wrote: {output_file}")

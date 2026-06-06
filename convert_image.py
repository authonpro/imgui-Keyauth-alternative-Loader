# Usage: python convert_image.py image.jpg > image_data.h
# Converts any image file to a C byte array for embedding in ImGui

import sys
import os

if len(sys.argv) < 2:
    print("Usage: python convert_image.py <image_file>")
    sys.exit(1)

filepath = sys.argv[1]
varname = os.path.splitext(os.path.basename(filepath))[0].replace(' ', '_').replace('-', '_')

with open(filepath, 'rb') as f:
    data = f.read()

print(f"// Auto-generated from {os.path.basename(filepath)}")
print(f"// Size: {len(data)} bytes")
print(f"#pragma once")
print(f"")
print(f"static const unsigned char {varname}_data[] = {{")

for i in range(0, len(data), 16):
    chunk = data[i:i+16]
    line = ", ".join(f"0x{b:02x}" for b in chunk)
    print(f"    {line},")

print(f"}};")
print(f"static const unsigned int {varname}_size = {len(data)};")

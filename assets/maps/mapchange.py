#!/usr/bin/env python3
"""
Overwrite map.txt by decrementing every Y coordinate by 1.
Usage: python fix_map.py map.txt
"""

import sys

def process_line(line):
    line = line.strip()
    if not line:
        return None
    parts = line.split(',')
    if len(parts) != 3:
        parts = [p.strip() for p in parts]
        if len(parts) != 3:
            print(f"Warning: skipping line: {line}", file=sys.stderr)
            return None
    try:
        x = int(parts[0])
        y = int(parts[1])
        tile_id = int(parts[2])
    except ValueError:
        print(f"Warning: skipping line (non‑integer): {line}", file=sys.stderr)
        return None
    new_y = y - 1
    return f"{x},{new_y},{tile_id}"

def main():
    if len(sys.argv) < 2:
        print("Usage: python fix_map.py map.txt", file=sys.stderr)
        sys.exit(1)

    filename = sys.argv[1]
    try:
        with open(filename, 'r') as f:
            lines = f.readlines()
    except FileNotFoundError:
        print(f"Error: file '{filename}' not found.", file=sys.stderr)
        sys.exit(1)

    new_lines = []
    for line in lines:
        new_line = process_line(line)
        if new_line is not None:
            new_lines.append(new_line)

    # Overwrite the original file
    with open(filename, 'w') as f:
        f.write('\n'.join(new_lines))
        if new_lines:
            f.write('\n')

    print(f"Done. Updated {filename} in place.")

if __name__ == "__main__":
    main()
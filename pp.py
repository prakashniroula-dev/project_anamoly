#!/usr/bin/env python3
"""
Recursively rename all files (and optionally folders) inside the given root
to lowercase. Works on Windows, Linux, and macOS.
"""

import os
import sys
import tempfile

def rename_to_lower(root_dir='assets', rename_dirs=False):
    # Walk bottom‑up so child items are renamed before parents
    for root, dirs, files in os.walk(root_dir, topdown=False):
        # ---------- rename files ----------
        for name in files:
            lower = name.lower()
            if name == lower:
                continue
            old = os.path.join(root, name)
            new = os.path.join(root, lower)

            try:
                # If target exists and is a different file, skip
                if os.path.exists(new) and not os.path.samefile(old, new):
                    print(f'⚠️ Skipping {old} (target {new} already exists)')
                    continue

                # If target is the same file (case difference only),
                # use a temporary name to force case change (needed on Windows)
                if os.path.exists(new) and os.path.samefile(old, new):
                    dirname = os.path.dirname(old)
                    # generate a unique temporary name
                    temp_name = next(tempfile._get_candidate_names())
                    temp_path = os.path.join(dirname, temp_name)
                    while os.path.exists(temp_path):
                        temp_name = next(tempfile._get_candidate_names())
                        temp_path = os.path.join(dirname, temp_name)

                    os.rename(old, temp_path)          # File.txt → tmp123
                    os.rename(temp_path, new)          # tmp123 → file.txt
                    print(f'✅ Renamed {old} -> {new} (via temporary)')
                else:
                    # Normal rename (target does not exist)
                    os.rename(old, new)
                    print(f'✅ Renamed {old} -> {new}')

            except OSError as e:
                print(f'❌ Error renaming {old} -> {new}: {e}')

        # ---------- rename directories (if requested) ----------
        if rename_dirs:
            for name in dirs:
                lower = name.lower()
                if name == lower:
                    continue
                old = os.path.join(root, name)
                new = os.path.join(root, lower)

                try:
                    if os.path.exists(new) and not os.path.samefile(old, new):
                        print(f'⚠️ Skipping dir {old} (target {new} already exists)')
                        continue
                    if os.path.exists(new) and os.path.samefile(old, new):
                        # Same logic as for files
                        dirname = os.path.dirname(old)
                        temp_name = next(tempfile._get_candidate_names())
                        temp_path = os.path.join(dirname, temp_name)
                        while os.path.exists(temp_path):
                            temp_name = next(tempfile._get_candidate_names())
                            temp_path = os.path.join(dirname, temp_name)

                        os.rename(old, temp_path)
                        os.rename(temp_path, new)
                        print(f'✅ Renamed dir {old} -> {new} (via temporary)')
                    else:
                        os.rename(old, new)
                        print(f'✅ Renamed dir {old} -> {new}')
                except OSError as e:
                    print(f'❌ Error renaming dir {old} -> {new}: {e}')


if __name__ == '__main__':
    # Usage: python3 rename_lower.py [root_dir] [--dirs]
    root = sys.argv[1] if len(sys.argv) > 1 and not sys.argv[1].startswith('--') else 'assets'
    rename_dirs = '--dirs' in sys.argv or '-d' in sys.argv
    rename_to_lower(root, rename_dirs)
    print('🎉 Done!')

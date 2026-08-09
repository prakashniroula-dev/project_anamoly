#!/usr/bin/env python3
"""
Recursively collect text files from a directory and concatenate their contents
into a single file (dump.txt). Each file's content is preceded by a C-style
comment with its relative path.
"""

import os
import sys


def is_text_file(filepath, sample_size=1024):
    """
    Heuristic: read the first sample_size bytes; if no null byte is found,
    assume it's a text file. Also catches decoding errors.
    """
    try:
        with open(filepath, 'rb') as f:
            chunk = f.read(sample_size)
            # If there is a null byte, it's likely binary
            if b'\0' in chunk:
                return False
        # Try to decode as UTF-8 (strict) to be sure
        with open(filepath, 'r', encoding='utf-8') as f:
            f.read(sample_size)
        return True
    except (UnicodeDecodeError, IOError, OSError):
        return False


def dump_text_files(root_dir, output_file='dump.txt'):
    """
    Walk root_dir recursively; for each text file, append its content to
    output_file with a C-style /* relative/path */ comment above.
    """
    root_dir = os.path.abspath(root_dir)
    output_abs = os.path.abspath(output_file)

    # Overwrite any existing dump file
    with open(output_file, 'w', encoding='utf-8') as out:
        out.write(f"/* Root directory : {root_dir} */")   # truncate

    count = 0
    for dirpath, _, filenames in os.walk(root_dir):
        for filename in filenames:
            filepath = os.path.join(dirpath, filename)
            # Skip the output file itself if it lies inside the scanned tree
            if os.path.abspath(filepath) == output_abs:
                continue

            # Compute relative path from root_dir
            rel_path = os.path.relpath(filepath, root_dir)

            # Check if it's a text file
            if not is_text_file(filepath):
                print(f"Skipping (not text): {rel_path}")
                continue

            try:
                with open(filepath, 'r', encoding='utf-8') as f:
                    content = f.read()
            except Exception as e:
                print(f"Error reading {rel_path}: {e}")
                continue

            # Append to dump.txt
            with open(output_file, 'a', encoding='utf-8') as out:
                out.write(f"/* {rel_path} */\n")
                out.write(content)
                if not content.endswith('\n'):
                    out.write('\n')
                out.write('\n')   # extra blank line between files

            count += 1
            print(f"Dumped: {rel_path}")

    print(f"\nDone. {count} text files dumped to {output_file}")


if __name__ == '__main__':
    if len(sys.argv) > 1:
        folder = sys.argv[1]
    else:
        folder = '.'   # current directory

    if not os.path.isdir(folder):
        print(f"Error: '{folder}' is not a valid directory.", file=sys.stderr)
        sys.exit(1)

    dump_text_files(folder)

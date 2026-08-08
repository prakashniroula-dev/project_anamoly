#!/usr/bin/env python3
"""
recursive_lowercase.py

Recursively rename all files (and optionally directories) inside a given root
folder to lowercase. On Linux, this handles case‑sensitive renaming safely.

Usage:
    python3 recursive_lowercase.py [path] [--dirs] [--dry-run]

    path      : root folder (default: "assets")
    --dirs    : also rename directories (default: only files)
    --dry-run : show what would be renamed without making changes
"""

import os
import sys
import argparse


def safe_rename(src, dst, dry_run=False):
    """
    Rename src to dst, but if dst already exists, find a unique name by adding
    a numeric suffix, e.g. "file.txt" -> "file_1.txt".
    """
    if src == dst:
        return False

    if dry_run:
        print(f"Would rename: {src} -> {dst}")
        return True

    # If destination already exists, find a free name
    if os.path.exists(dst):
        base, ext = os.path.splitext(dst)
        counter = 1
        while True:
            new_dst = f"{base}_{counter}{ext}"
            if not os.path.exists(new_dst):
                dst = new_dst
                break
            counter += 1
        print(f"Collision: renamed to {dst}")

    try:
        os.rename(src, dst)
        print(f"Renamed: {src} -> {dst}")
        return True
    except OSError as e:
        print(f"Error renaming {src}: {e}", file=sys.stderr)
        return False


def rename_items(root, rename_dirs=False, dry_run=False):
    """
    Walk the directory tree and rename all files (or directories) to lowercase.
    """
    renamed_count = 0

    # Walk bottom‑up to avoid issues when renaming directories that might affect
    # paths of inner items.
    for dirpath, dirnames, filenames in os.walk(root, topdown=False):
        # Rename files first
        for name in filenames:
            src = os.path.join(dirpath, name)
            dst = os.path.join(dirpath, name.lower())
            if safe_rename(src, dst, dry_run):
                renamed_count += 1

        # Optionally rename directories (also bottom‑up)
        if rename_dirs:
            for name in dirnames:
                src = os.path.join(dirpath, name)
                dst = os.path.join(dirpath, name.lower())
                if safe_rename(src, dst, dry_run):
                    renamed_count += 1

    return renamed_count


def main():
    parser = argparse.ArgumentParser(
        description="Recursively rename files (and optionally directories) to lowercase."
    )
    parser.add_argument(
        "path", nargs="?", default="assets",
        help="Root directory to process (default: assets)"
    )
    parser.add_argument(
        "--dirs", action="store_true",
        help="Also rename directories (default: only files)"
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Print the planned renames without actually doing them"
    )
    args = parser.parse_args()

    if not os.path.isdir(args.path):
        print(f"Error: '{args.path}' is not a valid directory.", file=sys.stderr)
        sys.exit(1)

    print(f"Processing directory: {args.path}")
    print(f"Rename directories: {args.dirs}")
    print(f"Dry run: {args.dry_run}")
    print("---")

    count = rename_items(args.path, args.dirs, args.dry_run)

    print("---")
    if args.dry_run:
        print(f"Dry run complete. {count} items would be renamed.")
    else:
        print(f"Done. {count} items renamed.")


if __name__ == "__main__":
    main()
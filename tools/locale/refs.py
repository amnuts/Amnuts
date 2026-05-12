#!/usr/bin/env python3
"""
Cross-check lang_*("key", ...) calls in source against the default
catalog. Reports missing keys (referenced in C but absent from
en_GB/strings.yml) and orphans (in en_GB/strings.yml but no caller).
"""
import argparse
import os
import re
import sys

try:
    import yaml
except ImportError:
    sys.exit("PyYAML required: pip3 install pyyaml")

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CATALOG   = os.path.join(REPO_ROOT, "files", "langs", "en_GB", "strings.yml")
SRC_GLOB  = os.path.join(REPO_ROOT, "src")

LANG_CALL = re.compile(r'\blang(?:_user|_room|_level|_format|)\s*\([^,)]*,\s*"([^"]+)"')


def find_source_keys():
    keys = {}
    for dirpath, _, files in os.walk(SRC_GLOB):
        if "vendors" in dirpath:
            continue
        for f in files:
            if not f.endswith(".c"):
                continue
            path = os.path.join(dirpath, f)
            with open(path, encoding="utf-8", errors="replace") as fh:
                for line_no, line in enumerate(fh, start=1):
                    for k in LANG_CALL.findall(line):
                        keys.setdefault(k, []).append((path, line_no))
    return keys


def find_catalog_keys():
    with open(CATALOG, encoding="utf-8") as f:
        data = yaml.safe_load(f) or {}
    return set(data.keys())


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--missing-only", action="store_true")
    p.add_argument("--orphans-only", action="store_true")
    args = p.parse_args()

    source_keys = find_source_keys()
    cat_keys    = find_catalog_keys()

    missing = sorted(set(source_keys) - cat_keys)
    orphans = sorted(cat_keys - set(source_keys))

    if not args.orphans_only and missing:
        print(f"# Missing keys ({len(missing)}): referenced in source but not in {os.path.relpath(CATALOG, REPO_ROOT)}.")
        for k in missing:
            sites = source_keys[k]
            print(f"{k}")
            for path, line in sites[:5]:
                print(f"  {os.path.relpath(path, REPO_ROOT)}:{line}")
            if len(sites) > 5:
                print(f"  ... and {len(sites) - 5} more.")
        print()

    if not args.missing_only and orphans:
        real_orphans = [k for k in orphans
                        if not k.startswith("ui.")
                        and not k.startswith("meta.")]
        if real_orphans:
            print(f"# Orphan keys ({len(real_orphans)}): present in catalog but no lang_* caller.")
            for k in real_orphans:
                print(f"  {k}")

    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())

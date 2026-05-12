#!/usr/bin/env python3
"""
Validate a non-default locale's strings.yml against the default's
format signatures. Mirrors the C-side load-time validation. Run
before committing a translation:

    python tools/locale/check.py files/langs/fr/strings.yml
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
DEFAULT_YML = os.path.join(REPO_ROOT, "files", "langs", "en_GB", "strings.yml")


def extract_signature(fmt):
    """Return types_per_position (list of 8) for a printf format. Raise on disallowed input."""
    types = [None] * 8
    implicit = 0
    pos_mode = imp_mode = False
    i = 0
    while i < len(fmt):
        if fmt[i] != "%":
            i += 1; continue
        i += 1
        if i >= len(fmt):
            raise ValueError("trailing %")
        if fmt[i] == "%":
            i += 1; continue
        pos = 0
        j = i
        while j < len(fmt) and fmt[j].isdigit():
            j += 1
        if j > i and j < len(fmt) and fmt[j] == "$":
            pos = int(fmt[i:j])
            i = j + 1
            pos_mode = True
        if pos == 0:
            implicit += 1
            pos = implicit
            imp_mode = True
        if pos_mode and imp_mode:
            raise ValueError("mixed positional and implicit specifiers")
        if pos < 1 or pos > 8:
            raise ValueError(f"argument position out of range: {pos}")
        while i < len(fmt) and fmt[i] in "-+ #0": i += 1
        while i < len(fmt) and fmt[i].isdigit(): i += 1
        if i < len(fmt) and fmt[i] == ".":
            i += 1
            while i < len(fmt) and fmt[i].isdigit(): i += 1
        while i < len(fmt) and fmt[i] in "hlLzjt": i += 1
        if i >= len(fmt):
            raise ValueError("trailing %")
        conv = fmt[i]; i += 1
        if conv in "diuxXo":
            canon = "d"
        elif conv == "s":
            canon = "s"
        elif conv == "c":
            canon = "c"
        elif conv == "n":
            raise ValueError("%n disallowed")
        else:
            raise ValueError(f"unsupported conversion: {conv!r}")
        if types[pos - 1] and types[pos - 1] != canon:
            raise ValueError("argument type conflict between positions")
        types[pos - 1] = canon
    return types


def load_yaml(path):
    with open(path, encoding="utf-8") as f:
        return yaml.safe_load(f) or {}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("locale_yml", help="Path to the locale's strings.yml")
    args = p.parse_args()

    default = load_yaml(DEFAULT_YML)
    target  = load_yaml(args.locale_yml)

    errors = 0
    for key, val in target.items():
        if not isinstance(val, str):
            print(f"DROP  {key!r}: value is not a scalar string")
            errors += 1
            continue
        try:
            tgt_sig = extract_signature(val)
        except ValueError as e:
            print(f"DROP  {key!r}: {e}")
            errors += 1
            continue
        if key in default:
            try:
                def_sig = extract_signature(default[key])
            except ValueError:
                continue
            for i in range(8):
                if tgt_sig[i] and not def_sig[i]:
                    print(f"DROP  {key!r}: uses position {i+1} that default omits")
                    errors += 1
                    break
                if tgt_sig[i] and def_sig[i] and tgt_sig[i] != def_sig[i]:
                    print(f"DROP  {key!r}: position {i+1} type mismatch ({tgt_sig[i]} vs default {def_sig[i]})")
                    errors += 1
                    break

    if errors == 0:
        print(f"OK: {args.locale_yml} validates clean.")
    else:
        print(f"\n{errors} key(s) would be dropped at load time.")
    return 0 if errors == 0 else 1


if __name__ == "__main__":
    sys.exit(main())

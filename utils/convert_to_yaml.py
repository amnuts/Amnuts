#!/usr/bin/env python3
"""
Convert legacy Amnuts data files (config, *.R, helpfiles/*) to the
three YAML files used by the new loader. Destructive: removes legacy
files after a successful run, with auto-backup beforehand.

Run from the repo root:
    python utils/convert_to_yaml.py
"""
import argparse
import datetime
import glob
import os
import re
import shutil
import sys
import textwrap

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATAFILES = os.path.join(REPO_ROOT, "files", "datafiles")
HELPFILES = os.path.join(REPO_ROOT, "files", "helpfiles")

CONFIG_YAML = os.path.join(DATAFILES, "config.yaml")
ROOMS_YAML = os.path.join(DATAFILES, "rooms.yaml")
HELP_YAML = os.path.join(DATAFILES, "help.yaml")

LEGACY_CONFIG = os.path.join(DATAFILES, "config")


INIT_GROUPS = {
    "server": [
        ("verification",  "verification"),
        ("mainport",      "ports.main"),
        ("wizport",       "ports.wiz"),
        ("linkport",      "ports.link"),
        ("max_users",     "max_users"),
        ("max_clones",    "max_clones"),
        ("heartbeat",     "heartbeat"),
    ],
    "timeouts": [
        ("login_idle_time",   "login_idle"),
        ("user_idle_time",    "user_idle"),
        ("time_out_afks",     "timeout_afks"),
        ("time_out_maxlevel", "timeout_maxlevel"),
    ],
    "defaults": [
        ("colour_def",       "colour"),
        ("prompt_def",       "prompt"),
        ("charecho_def",     "charecho"),
        ("passwordecho_def", "passwordecho"),
        ("default_warp",     "warp_room"),
        ("default_jail",     "jail_room"),
        ("default_bank",     "bank_room"),
        ("default_shoot",    "shoot_room"),
    ],
    "moderation": [
        ("ban_swearing",    "ban_swearing"),
        ("minlogin_level",  "minlogin_level"),
        ("min_private",     "min_private"),
        ("ignore_mp_level", "ignore_mp_level"),
        ("gatecrash_level", "gatecrash_level"),
        ("boot_off_min",    "boot_off_min"),
        ("flood_protect",   "flood_protect"),
    ],
    "system": [
        ("system_logging", "logging"),
        ("ignore_sigterm", "ignore_sigterm"),
        ("auto_connect",   "auto_connect"),
        ("crash_action",   "crash_action"),
        ("resolve_ip",     "resolve_ip"),
        ("random_motds",   "random_motds"),
    ],
    "users": [
        ("auto_purge",         "auto_purge"),
        ("allow_recaps",       "allow_recaps"),
        ("auto_promote",       "auto_promote"),
        ("personal_rooms",     "personal_rooms"),
        ("startup_room_parse", "startup_room_parse"),
        ("rem_user_maxlevel",  "rem_user_maxlevel"),
        ("rem_user_deflevel",  "rem_user_deflevel"),
        ("wizport_level",      "wizport_level"),
    ],
    "messages": [
        ("mesg_life",       "lifetime_days"),
        ("mesg_check_time", "check_time"),
    ],
}

# YAML leaf keys whose values should be ints (after coercion).
INT_KEYS = {
    "main", "wiz", "link", "max_users", "max_clones", "heartbeat",
    "login_idle", "user_idle", "min_private", "lifetime_days",
}

# Legacy keys whose values are ON/OFF or YES/NO and should become YAML bool.
BOOL_LEGACY_KEYS = {
    "system_logging", "prompt_def", "passwordecho_def", "ignore_sigterm",
    "auto_connect", "colour_def", "time_out_afks", "charecho_def",
    "auto_purge", "allow_recaps", "auto_promote", "personal_rooms",
    "random_motds", "startup_room_parse", "flood_protect", "boot_off_min",
}

# Legacy keys whose values are level names.
LEVEL_LEGACY_KEYS = {
    "minlogin_level", "wizport_level", "gatecrash_level", "ignore_mp_level",
    "rem_user_maxlevel", "rem_user_deflevel", "time_out_maxlevel",
}

# Patterns for parsing help file headers.
USAGE_LINE = re.compile(r"^~OLUsage\s*:~RS\s*(.*)$")
USAGE_CONT = re.compile(r"^~OL\s*:~RS\s*(.*)$")
ALIASES_LINE = re.compile(r"^~OLAliases\s*:~RS\s*(.*)$")
COMMAND_LINE = re.compile(r"^~OLCommand\s*:~RS\s*(.*)$")


def yaml_already_present():
    return any(os.path.exists(p) for p in (CONFIG_YAML, ROOMS_YAML, HELP_YAML))


def legacy_present():
    if os.path.exists(LEGACY_CONFIG):
        return True
    if glob.glob(os.path.join(DATAFILES, "*.R")):
        return True
    if os.path.isdir(HELPFILES) and os.listdir(HELPFILES):
        return True
    return False


def confirm():
    print(textwrap.dedent("""
        WARNING: this script is DESTRUCTIVE.

        It will delete the following once it has written the new YAML files:
          - files/datafiles/config
          - files/datafiles/*.R
          - files/helpfiles/ (entire directory)

        It WILL automatically copy these files into a backup folder before
        touching anything, but you should ALSO take your own backup right now,
        just in case.
    """).strip())
    answer = input("\nProceed with conversion? [y/N] ").strip().lower()
    return answer == "y"


def make_backup():
    stamp = datetime.datetime.now().strftime("%Y%m%d%H%M%S")
    backup_dir = os.path.join(REPO_ROOT, f"data-backup-{stamp}")
    os.makedirs(backup_dir, exist_ok=False)
    if os.path.exists(LEGACY_CONFIG):
        shutil.copy2(LEGACY_CONFIG, backup_dir)
    for r in glob.glob(os.path.join(DATAFILES, "*.R")):
        shutil.copy2(r, backup_dir)
    if os.path.isdir(HELPFILES):
        shutil.copytree(HELPFILES, os.path.join(backup_dir, "helpfiles"))
    print(f"Backup written to {backup_dir}")
    return backup_dir


def cleanup_legacy():
    if os.path.exists(LEGACY_CONFIG):
        os.remove(LEGACY_CONFIG)
    for r in glob.glob(os.path.join(DATAFILES, "*.R")):
        os.remove(r)
    if os.path.isdir(HELPFILES):
        shutil.rmtree(HELPFILES)


def parse_legacy_config():
    """Return (init_dict, sites_dict, rooms_lines, topics_lines).

    Reads files/datafiles/config and splits it into its four sections.
    Lines starting with '#' or blank lines are ignored. INIT/ROOMS/TOPICS/SITES
    section headers end with ':' and switch the active section.
    """
    section = None
    init = {}
    sites = {}
    rooms_lines = []
    topics_lines = []
    with open(LEGACY_CONFIG) as f:
        for raw in f:
            line = raw.split("#", 1)[0].rstrip("\n").rstrip()
            if not line.strip():
                continue
            if line.endswith(":") and not " " in line.rstrip(":"):
                section = line.rstrip(":").strip()
                continue
            if section == "INIT":
                parts = line.split(None, 1)
                if len(parts) == 2:
                    init[parts[0]] = parts[1].strip()
            elif section == "SITES":
                parts = line.split()
                if len(parts) >= 4:
                    sites[parts[0]] = {
                        "address": parts[1],
                        "port": int(parts[2]),
                        "verification": parts[3],
                        "allow": parts[4] if len(parts) > 4 else "ALL",
                    }
            elif section == "ROOMS":
                rooms_lines.append(line)
            elif section == "TOPICS":
                topics_lines.append(line)
    return init, sites, rooms_lines, topics_lines


def _set_path(d, dotted_path, value):
    parts = dotted_path.split(".")
    for p in parts[:-1]:
        d = d.setdefault(p, {})
    d[parts[-1]] = value


def _coerce(legacy_key, yaml_path, raw):
    leaf = yaml_path.rsplit(".", 1)[-1]
    if leaf in INT_KEYS:
        return int(raw)
    if legacy_key in BOOL_LEGACY_KEYS:
        return raw.upper() in ("YES", "ON", "1", "TRUE")
    # ban_swearing, crash_action, resolve_ip stay as strings (their values
    # are uppercase enum-like names, not booleans).
    return raw


def convert_config():
    import yaml as pyyaml  # imported lazily so the skeleton ran without yaml installed
    init, sites, _rooms_lines, _topics_lines = parse_legacy_config()
    out = {}
    for group, mappings in INIT_GROUPS.items():
        for legacy_key, yaml_path in mappings:
            full_path = f"{group}.{yaml_path}"
            if legacy_key in init:
                _set_path(out, full_path, _coerce(legacy_key, yaml_path, init[legacy_key]))
    if sites:
        out["sites"] = sites
    with open(CONFIG_YAML, "w") as f:
        pyyaml.safe_dump(out, f, sort_keys=False, default_flow_style=False)
    print(f"Wrote {CONFIG_YAML}")


def convert_rooms():
    import yaml as pyyaml

    # Force multi-line strings to be emitted as literal block scalars
    # (description: |) rather than the default folded/quoted form.
    def _str_representer(dumper, data):
        if "\n" in data:
            return dumper.represent_scalar(
                "tag:yaml.org,2002:str", data, style="|"
            )
        return dumper.represent_scalar("tag:yaml.org,2002:str", data)

    pyyaml.SafeDumper.add_representer(str, _str_representer)

    _init, _sites, room_lines, topic_lines = parse_legacy_config()
    rooms = {}
    label_to_name = {}

    # Pass 1: parse structure, build label-to-name map.
    parsed = []
    for line in room_lines:
        parts = line.split()
        if len(parts) < 4:
            raise ValueError(f"Malformed ROOMS line: {line!r}")
        map_, label, name, links_csv = parts[:4]
        access = parts[4] if len(parts) > 4 else None
        netlink_kw = parts[5] if len(parts) > 5 else None
        netlink_name = parts[6] if len(parts) > 6 else None
        label_to_name[label] = name
        parsed.append((name, map_, label, links_csv, access, netlink_kw, netlink_name))

    # Pass 2: build entries, resolving links from labels.
    for name, map_, label, links_csv, access, netlink_kw, netlink_name in parsed:
        link_names = []
        for ll in links_csv.split(","):
            ll = ll.strip()
            if not ll:
                continue
            if ll not in label_to_name:
                raise ValueError(
                    f"Room '{name}' links to unknown label '{ll}'"
                )
            link_names.append(label_to_name[ll])

        entry = {
            "map": map_,
            "label": label,
            "links": link_names,
        }
        if access and access != "BOTH":
            entry["access"] = access
        if netlink_kw == "ACCEPT":
            entry["netlink"] = "ACCEPT"
        elif netlink_kw == "CONNECT":
            if not netlink_name:
                raise ValueError(
                    f"Room '{name}' has CONNECT without a netlink name"
                )
            entry["netlink"] = {"type": "CONNECT", "name": netlink_name}
        elif netlink_kw is not None:
            raise ValueError(
                f"Room '{name}' has unknown netlink keyword '{netlink_kw}'"
            )

        # Description from <name>.R, if present.
        desc_path = os.path.join(DATAFILES, f"{name}.R")
        if os.path.exists(desc_path):
            with open(desc_path) as f:
                entry["description"] = f.read()

        rooms[name] = entry

    # Pass 3: attach topics by room name.
    for line in topic_lines:
        # Topic format: <room_name> <topic_text...>
        parts = line.split(None, 1)
        if len(parts) < 2:
            continue
        room_name, topic = parts
        if room_name in rooms:
            rooms[room_name]["topic"] = topic.strip()
        # If a topic references a non-existent room, skip silently -- the
        # legacy parser would error, but for one-shot migration we'd rather
        # produce output and let the operator notice on review.

    with open(ROOMS_YAML, "w") as f:
        pyyaml.safe_dump(
            {"rooms": rooms},
            f,
            sort_keys=False,
            default_flow_style=False,
            allow_unicode=True,
        )
    print(f"Wrote {ROOMS_YAML}")


def parse_help_file(path):
    """Parse a single legacy helpfile, returning a dict suitable for YAML."""
    entry = {}
    usage = []
    aliases = []
    desc_lines = []
    in_desc = False

    with open(path) as f:
        for raw in f:
            line = raw.rstrip("\n")

            if in_desc:
                desc_lines.append(line)
                continue

            if not line.strip():
                in_desc = True
                continue

            if COMMAND_LINE.match(line):
                continue

            m = USAGE_LINE.match(line)
            if m:
                usage.append(m.group(1).strip())
                continue

            m = USAGE_CONT.match(line)
            if m:
                usage.append(m.group(1).strip())
                continue

            m = ALIASES_LINE.match(line)
            if m:
                raw_aliases = m.group(1).strip()
                # Separators: ", " or " or "
                for part in re.split(r",\s*|\s+or\s+", raw_aliases):
                    part = part.strip()
                    if part:
                        aliases.append(part)
                continue

            # Unrecognised header line -- fold into description.
            desc_lines.append(line)
            in_desc = True

    if usage:
        # Single-line usage stays a scalar; multi-line becomes a sequence.
        entry["usage"] = usage[0] if len(usage) == 1 else usage
    if aliases:
        entry["aliases"] = aliases
    desc = "\n".join(desc_lines).strip("\n")
    if desc:
        entry["description"] = desc
    return entry


def convert_help():
    import yaml as pyyaml
    out = {}
    flagged = []
    if not os.path.isdir(HELPFILES):
        with open(HELP_YAML, "w") as f:
            pyyaml.safe_dump({}, f)
        print(f"Wrote {HELP_YAML} (empty -- no helpfiles directory)")
        return
    # Build entries in alphabetical order at the top level. parse_help_file
    # constructs each entry with keys in the desired order: usage, aliases,
    # description. We rely on Python 3.7+ dict insertion-order preservation
    # and emit with sort_keys=False so entry-level keys keep that order
    # while top-level command names stay alphabetical (because we iterate
    # sorted listdir below).
    for fname in sorted(os.listdir(HELPFILES)):
        path = os.path.join(HELPFILES, fname)
        if not os.path.isfile(path):
            continue
        try:
            out[fname] = parse_help_file(path)
        except Exception as e:
            flagged.append((fname, str(e)))
    with open(HELP_YAML, "w") as f:
        pyyaml.safe_dump(out, f, sort_keys=False, default_flow_style=False,
                         allow_unicode=True, width=10000)
    print(f"Wrote {HELP_YAML} ({len(out)} entries)")
    if flagged:
        print("WARNING: the following help files did not parse cleanly and "
              "should be reviewed manually:")
        for fname, err in flagged:
            print(f"  {fname}: {err}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--keep-legacy", action="store_true",
        help="Don't delete legacy files after conversion (development use only).",
    )
    args = parser.parse_args()

    if yaml_already_present():
        print("Already migrated -- YAML files exist. Aborting.")
        return 0

    if not legacy_present():
        print("Nothing to convert.")
        return 0

    if not confirm():
        print("Aborted.")
        return 1

    backup_dir = make_backup()
    try:
        convert_config()
        convert_rooms()
        convert_help()
    except Exception:
        for p in (CONFIG_YAML, ROOMS_YAML, HELP_YAML):
            if os.path.exists(p):
                os.remove(p)
        print(
            f"Conversion failed; legacy files left in place. "
            f"Backup at {backup_dir}."
        )
        raise

    if not args.keep_legacy:
        cleanup_legacy()
        print("Legacy files removed.")
    else:
        print("--keep-legacy: legacy files left in place.")

    print("Done. Three YAML files written to files/datafiles/.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

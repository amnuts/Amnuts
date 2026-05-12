# Localisation tools

Three small scripts that support the bulk Phase 5 sweep and the
translator workflow that follows. All three are dev-only Python 3,
require PyYAML, and are not bundled with the talker binary.

## extract.py

Suggests catalog keys for a single C source file:

    python tools/locale/extract.py src/commands/quit.c

Output is YAML you can paste into `files/langs/en_GB/strings.yml`,
review, and edit. The keys are suggestions — change them to match
the project's `<command>.<context>.<variant>` convention.

## refs.py

Cross-checks `lang_*("key", ...)` invocations in source against the
catalog (`files/langs/en_GB/strings.yml`). Reports:

- Keys used in source but missing from the default catalog.
- Keys in the default catalog with no source caller (orphans).

    python tools/locale/refs.py

Run before committing each conversion.

## check.py

Validates a non-default locale's `strings.yml` against the default's
signatures. Loads both files, runs the same format-signature
extraction the C-side loader uses, and reports the keys a real
talker boot would drop.

    python tools/locale/check.py files/langs/fr/strings.yml

Useful for translators before they push a new locale.

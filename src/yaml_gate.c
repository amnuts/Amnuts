#include "defines.h"
#include "globals.h"
#include "prototypes.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static int
file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

static int
legacy_R_files_present(void)
{
    DIR *d = opendir(DATAFILES);
    if (!d) {
        return 0;
    }
    struct dirent *e;
    int found = 0;
    while ((e = readdir(d))) {
        size_t len = strlen(e->d_name);
        if (len > 2 && !strcmp(e->d_name + len - 2, ".R")) {
            found = 1;
            break;
        }
    }
    closedir(d);
    return found;
}

static int
helpfiles_dir_nonempty(void)
{
    DIR *d = opendir(HELPFILES);
    if (!d) {
        return 0;
    }
    struct dirent *e;
    int found = 0;
    while ((e = readdir(d))) {
        if (strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) {
            found = 1;
            break;
        }
    }
    closedir(d);
    return found;
}

void
yaml_check_migration_gate(void)
{
    int has_config = file_exists(DATAFILES "/config.yaml");
    int has_rooms  = file_exists(DATAFILES "/rooms.yaml");
    int has_help   = file_exists(DATAFILES "/help.yaml");
    int yaml_complete = has_config && has_rooms && has_help;
    int yaml_partial  = (has_config || has_rooms || has_help) && !yaml_complete;

    if (yaml_complete) {
        return;
    }

    int legacy_config_present = file_exists(DATAFILES "/config");
    int legacy_R_present      = legacy_R_files_present();
    int legacy_help_present   = helpfiles_dir_nonempty();
    int any_legacy = legacy_config_present || legacy_R_present || legacy_help_present;

    if (yaml_partial) {
        fprintf(stderr,
            "Amnuts: incomplete YAML configuration in %s.\n  config.yaml: %s\n  rooms.yaml:  %s\n  help.yaml:   %s\nAll three are required.\n",
            DATAFILES,
            has_config ? "found" : "MISSING",
            has_rooms  ? "found" : "MISSING",
            has_help   ? "found" : "MISSING"
        );
        boot_exit(1);
    }

    if (any_legacy) {
        fprintf(stderr,
            "Amnuts: legacy data files detected but no YAML configuration.\nBack up your data, then run the converter:\n    python utils/convert_to_yaml.py\nOnce it completes, start the talker again.\n"
        );
        boot_exit(1);
    }

    fprintf(stderr,
        "Amnuts: no configuration found in %s.\nOn a fresh install, copy the samples to active names:\n    cp %s/config.yaml.sample %s/config.yaml\n    cp %s/rooms.yaml.sample  %s/rooms.yaml\n    cp %s/help.yaml.sample   %s/help.yaml\n",
        DATAFILES, DATAFILES, DATAFILES, DATAFILES, DATAFILES, DATAFILES, DATAFILES
    );
    boot_exit(1);
}

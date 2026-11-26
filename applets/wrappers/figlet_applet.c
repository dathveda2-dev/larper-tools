#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "figlet_standard_flf.h"

extern int figlet_upstream_main(int argc, char **argv);

int figlet_main(int argc, char **argv) {
    /* If user explicitly passed -d or the installed font dir has standard.flf, run directly */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            return figlet_upstream_main(argc, argv);
        }
    }

    char installed_font[512];
    snprintf(installed_font, sizeof(installed_font), "%s/standard.flf", LARPER_FIGLET_FONTDIR);
    if (access(installed_font, R_OK) == 0) {
        return figlet_upstream_main(argc, argv);
    }

    /* Fallback: materialize embedded standard.flf so single-binary execution always works */
    char tmpdir[] = "/tmp/larper-figlet-XXXXXX";
    if (!mkdtemp(tmpdir)) {
        return figlet_upstream_main(argc, argv);
    }
    char flf_path[512];
    snprintf(flf_path, sizeof(flf_path), "%s/standard.flf", tmpdir);
    FILE *f = fopen(flf_path, "wb");
    if (f) {
        fwrite(figlet_standard_flf_data, 1, figlet_standard_flf_data_len, f);
        fclose(f);
    }

    char **new_argv = calloc((size_t)argc + 4, sizeof(char *));
    if (!new_argv) {
        unlink(flf_path);
        rmdir(tmpdir);
        return 1;
    }
    new_argv[0] = argv[0];
    new_argv[1] = "-d";
    new_argv[2] = tmpdir;
    for (int i = 1; i < argc; i++) {
        new_argv[i + 2] = argv[i];
    }
    new_argv[argc + 2] = NULL;

    int rc = figlet_upstream_main(argc + 2, new_argv);
    free(new_argv);
    unlink(flf_path);
    rmdir(tmpdir);
    return rc;
}

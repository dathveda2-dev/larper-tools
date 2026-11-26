/*
 * larper-tools: BusyBox-style multicall binary bundling the 10 most iconic
 * Linux terminal LARP / poser tools built directly from upstream sources.
 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "applets.h"

#define LARPER_VERSION "1.0.0"

static const Applet applets[] = {
#ifdef CONFIG_CMATRIX
    { "cmatrix",   NULL,    "Matrix digital rain (abishekvashok/cmatrix)",         cmatrix_main   },
#endif
#ifdef CONFIG_SL
    { "sl",        NULL,    "Steam locomotive typo punishment (mtoyoda/sl)",       sl_main        },
#endif
#ifdef CONFIG_CBONSAI
    { "cbonsai",   NULL,    "Procedural ASCII bonsai tree (jallbrit/cbonsai)",     cbonsai_main   },
#endif
#ifdef CONFIG_NMS
    { "nms",       NULL,    "Sneakers movie decryption effect (no-more-secrets)",  nms_main       },
#endif
#ifdef CONFIG_TTY_CLOCK
    { "tty-clock", NULL,    "Giant ncurses digital clock (xorg62/tty-clock)",      tty_clock_main },
#endif
#ifdef CONFIG_LOLCAT
    { "lolcat",    NULL,    "Fast C rainbow terminal filter (jaseg/lolcat)",       lolcat_main    },
#endif
#ifdef CONFIG_FIGLET
    { "figlet",    NULL,    "Large ASCII art banners (cmatsuoka/figlet)",          figlet_main    },
#endif
#ifdef CONFIG_NYANCAT
    { "nyancat",   NULL,    "Animated ANSI Nyan Cat (klange/nyancat)",             nyancat_main   },
#endif
#ifdef CONFIG_PIPES
    { "pipes.sh",  "pipes", "Animated neon terminal pipes (pipeseroni/pipes.sh)",  pipes_main     },
#endif
#ifdef CONFIG_NEOFETCH
    { "neofetch",  NULL,    "Linux rice system info flex (dylanaraps/neofetch)",   neofetch_main  },
#endif
    { NULL, NULL, NULL, NULL }
};

static const Applet *find_applet(const char *name) {
    for (int i = 0; applets[i].name != NULL; i++) {
        if (strcmp(applets[i].name, name) == 0) return &applets[i];
        if (applets[i].alias && strcmp(applets[i].alias, name) == 0) return &applets[i];
    }
    return NULL;
}

static void print_usage(void) {
    printf("larper-tools v%s (BusyBox-style multicall binary)\n\n", LARPER_VERSION);
    printf("Usage: larper-tools [function [arguments]...]\n");
    printf("   or: larper-tools --list\n");
    printf("   or: larper-tools --install [-s] [DIR]\n");
    printf("   or: function [arguments]...  (via BusyBox-style symlinks)\n\n");
    printf("Currently defined functions:\n");
    int count = 0;
    for (int i = 0; applets[i].name != NULL; i++) {
        printf("  %-12s  %s\n", applets[i].name, applets[i].desc);
        count++;
    }
    if (count == 0) {
        printf("  (none enabled! Run 'make menuconfig' or 'make defconfig')\n");
    }
}

static int do_install_symlinks(const char *dir) {
    char self_path[4096];
    ssize_t n = readlink("/proc/self/exe", self_path, sizeof(self_path) - 1);
    if (n < 0) {
        perror("larper-tools: readlink(/proc/self/exe)");
        return 1;
    }
    self_path[n] = '\0';

    int rc = 0;
    for (int i = 0; applets[i].name != NULL; i++) {
        const char *names[2] = { applets[i].name, applets[i].alias };
        for (int k = 0; k < 2; k++) {
            if (!names[k]) continue;
            char link_path[4096];
            snprintf(link_path, sizeof(link_path), "%s/%s", dir, names[k]);
            unlink(link_path);
            if (symlink("larper-tools", link_path) != 0) {
                fprintf(stderr, "larper-tools: symlink %s -> larper-tools failed: %s\n",
                        link_path, strerror(errno));
                rc = 1;
            } else {
                printf("  LN  %s -> larper-tools\n", link_path);
            }
        }
    }
    return rc;
}

int main(int argc, char **argv) {
    char *argv0_copy = strdup(argv[0]);
    const char *prog = argv0_copy ? basename(argv0_copy) : argv[0];

    /* BusyBox multicall dispatch via argv[0] */
    const Applet *ap = find_applet(prog);
    if (ap) {
        free(argv0_copy);
        return ap->entry(argc, argv);
    }
    free(argv0_copy);

    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_usage();
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0) {
        printf("larper-tools v%s\n", LARPER_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "--list") == 0) {
        for (int i = 0; applets[i].name != NULL; i++) {
            printf("%s\n", applets[i].name);
            if (applets[i].alias) printf("%s\n", applets[i].alias);
        }
        return 0;
    }

    if (strcmp(argv[1], "--install") == 0) {
        const char *dir = "/usr/local/bin";
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-s") == 0) continue;
            dir = argv[i];
        }
        return do_install_symlinks(dir);
    }

    ap = find_applet(argv[1]);
    if (!ap) {
        fprintf(stderr, "larper-tools: '%s': applet not found\n", argv[1]);
        fprintf(stderr, "Run 'larper-tools --list' to see enabled applets.\n");
        return 127;
    }

    return ap->entry(argc - 1, argv + 1);
}

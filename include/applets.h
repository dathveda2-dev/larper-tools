#ifndef LARPER_APPLETS_H
#define LARPER_APPLETS_H

#include "autoconf.h"

typedef int (*applet_fn)(int argc, char **argv);

typedef struct {
    const char *name;
    const char *alias;
    const char *desc;
    applet_fn   entry;
} Applet;

#ifdef CONFIG_CMATRIX
int cmatrix_main(int argc, char **argv);
#endif
#ifdef CONFIG_SL
int sl_main(int argc, char **argv);
#endif
#ifdef CONFIG_CBONSAI
int cbonsai_main(int argc, char **argv);
#endif
#ifdef CONFIG_NMS
int nms_main(int argc, char **argv);
#endif
#ifdef CONFIG_TTY_CLOCK
int tty_clock_main(int argc, char **argv);
#endif
#ifdef CONFIG_LOLCAT
int lolcat_main(int argc, char **argv);
#endif
#ifdef CONFIG_FIGLET
int figlet_main(int argc, char **argv);
#endif
#ifdef CONFIG_NYANCAT
int nyancat_main(int argc, char **argv);
#endif
#ifdef CONFIG_PIPES
int pipes_main(int argc, char **argv);
#endif
#ifdef CONFIG_NEOFETCH
int neofetch_main(int argc, char **argv);
#endif

#endif /* LARPER_APPLETS_H */

/*
 * BusyBox / Linux-kernel style ncurses menuconfig for larper-tools.
 * Parses Config.in, loads/saves .config.
 */
#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MENUS 32
#define MAX_ITEMS 128
#define MAX_STR   256
#define MAX_HELP  1024

typedef struct {
    char name[64];
    char prompt[MAX_STR];
    char help[MAX_HELP];
    bool value;
    bool def;
    int menu_idx;
} ConfigItem;

typedef struct {
    char title[MAX_STR];
} MenuGroup;

static char main_title[MAX_STR] = "larper-tools Configuration";
static MenuGroup menus[MAX_MENUS];
static int num_menus = 0;
static ConfigItem items[MAX_ITEMS];
static int num_items = 0;
static bool modified = false;

static char *trim(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    return s;
}

static bool extract_quoted(const char *src, char *dst, size_t dst_sz) {
    const char *q1 = strchr(src, '"');
    if (!q1) return false;
    const char *q2 = strchr(q1 + 1, '"');
    if (!q2) return false;
    size_t n = (size_t)(q2 - (q1 + 1));
    if (n >= dst_sz) n = dst_sz - 1;
    memcpy(dst, q1 + 1, n);
    dst[n] = '\0';
    return true;
}

static int parse_config_in(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "mconf: cannot open %s\n", path);
        return -1;
    }
    char line[512];
    int cur_menu = -1;
    int cur_item = -1;
    bool in_help = false;

    while (fgets(line, sizeof(line), f)) {
        if (in_help) {
            if (line[0] == '\t' || (line[0] == ' ' && line[1] == ' ') || line[0] == '\n') {
                if (cur_item >= 0) {
                    char *t = trim(line);
                    if (strlen(items[cur_item].help) + strlen(t) + 2 < MAX_HELP) {
                        if (items[cur_item].help[0] != '\0')
                            strcat(items[cur_item].help, "\n");
                        strcat(items[cur_item].help, t);
                    }
                }
                continue;
            }
            in_help = false;
        }

        char *t = trim(line);
        if (t[0] == '\0' || t[0] == '#') continue;

        if (strncmp(t, "mainmenu ", 9) == 0) {
            extract_quoted(t, main_title, sizeof(main_title));
        } else if (strncmp(t, "menu ", 5) == 0) {
            if (num_menus < MAX_MENUS) {
                extract_quoted(t, menus[num_menus].title, MAX_STR);
                cur_menu = num_menus++;
            }
            cur_item = -1;
        } else if (strcmp(t, "endmenu") == 0) {
            cur_menu = -1;
            cur_item = -1;
        } else if (strncmp(t, "config ", 7) == 0) {
            if (num_items < MAX_ITEMS) {
                cur_item = num_items++;
                memset(&items[cur_item], 0, sizeof(ConfigItem));
                strncpy(items[cur_item].name, trim(t + 7), 63);
                strncpy(items[cur_item].prompt, items[cur_item].name, MAX_STR - 1);
                items[cur_item].menu_idx = cur_menu;
            }
        } else if (cur_item >= 0 && strncmp(t, "bool ", 5) == 0) {
            extract_quoted(t, items[cur_item].prompt, MAX_STR);
        } else if (cur_item >= 0 && strncmp(t, "default ", 8) == 0) {
            char *v = trim(t + 8);
            items[cur_item].def = (v[0] == 'y');
            items[cur_item].value = items[cur_item].def;
        } else if (cur_item >= 0 && strcmp(t, "help") == 0) {
            in_help = true;
        }
    }
    fclose(f);
    return 0;
}

static void load_dot_config(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *t = trim(line);
        if (strncmp(t, "CONFIG_", 7) == 0) {
            char *eq = strchr(t, '=');
            if (!eq) continue;
            *eq = '\0';
            const char *name = t + 7;
            bool val = (eq[1] == 'y');
            for (int i = 0; i < num_items; i++) {
                if (strcmp(items[i].name, name) == 0) {
                    items[i].value = val;
                    break;
                }
            }
        } else if (strncmp(t, "# CONFIG_", 9) == 0 && strstr(t, " is not set")) {
            char name[64] = {0};
            const char *p = t + 9;
            size_t j = 0;
            while (*p && *p != ' ' && j < 63) name[j++] = *p++;
            for (int i = 0; i < num_items; i++) {
                if (strcmp(items[i].name, name) == 0) {
                    items[i].value = false;
                    break;
                }
            }
        }
    }
    fclose(f);
}

static int save_dot_config(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fprintf(f, "#\n# Automatically generated file; DO NOT EDIT.\n# larper-tools Configuration\n#\n");
    for (int i = 0; i < num_items; i++) {
        if (items[i].value)
            fprintf(f, "CONFIG_%s=y\n", items[i].name);
        else
            fprintf(f, "# CONFIG_%s is not set\n", items[i].name);
    }
    fclose(f);
    modified = false;
    return 0;
}

static void draw_box(int y, int x, int h, int w, const char *title) {
    attron(COLOR_PAIR(2));
    for (int r = y; r < y + h; r++) {
        mvhline(r, x, ' ', w);
    }
    mvaddch(y, x, ACS_ULCORNER);
    mvhline(y, x + 1, ACS_HLINE, w - 2);
    mvaddch(y, x + w - 1, ACS_URCORNER);
    for (int r = y + 1; r < y + h - 1; r++) {
        mvaddch(r, x, ACS_VLINE);
        mvaddch(r, x + w - 1, ACS_VLINE);
    }
    mvaddch(y + h - 1, x, ACS_LLCORNER);
    mvhline(y + h - 1, x + 1, ACS_HLINE, w - 2);
    mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);
    if (title && title[0]) {
        attron(A_BOLD | COLOR_PAIR(4));
        mvprintw(y, x + (w - (int)strlen(title) - 2) / 2, " %s ", title);
        attroff(A_BOLD | COLOR_PAIR(4));
    }
    attroff(COLOR_PAIR(2));
}

static void show_help(const ConfigItem *it) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int h = 12, w = cols - 12;
    if (w > 72) w = 72;
    if (w < 40) w = 40;
    int y = (rows - h) / 2;
    int x = (cols - w) / 2;
    char hdr[128];
    snprintf(hdr, sizeof(hdr), "HELP: CONFIG_%s", it->name);
    draw_box(y, x, h, w, hdr);
    attron(COLOR_PAIR(2));
    mvprintw(y + 2, x + 2, "%.*s", w - 4, it->prompt);
    int row = y + 4;
    const char *p = it->help[0] ? it->help : "No help available.";
    while (*p && row < y + h - 2) {
        const char *nl = strchr(p, '\n');
        int len = nl ? (int)(nl - p) : (int)strlen(p);
        if (len > w - 4) len = w - 4;
        mvprintw(row++, x + 2, "%.*s", len, p);
        if (!nl) break;
        p = nl + 1;
    }
    attron(A_BOLD | COLOR_PAIR(3));
    mvprintw(y + h - 2, x + w / 2 - 4, " < OK > ");
    attroff(A_BOLD | COLOR_PAIR(3));
    attroff(COLOR_PAIR(2));
    refresh();
    getch();
}

static bool confirm_save(void) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int h = 7, w = 52;
    int y = (rows - h) / 2;
    int x = (cols - w) / 2;
    bool yes = true;
    for (;;) {
        draw_box(y, x, h, w, "Save Configuration");
        attron(COLOR_PAIR(2));
        mvprintw(y + 2, x + 3, "Do you wish to save your new configuration?");
        if (yes) attron(A_BOLD | COLOR_PAIR(3)); else attron(COLOR_PAIR(2));
        mvprintw(y + 4, x + 13, " < Yes > ");
        attroff(A_BOLD | COLOR_PAIR(3));
        if (!yes) attron(A_BOLD | COLOR_PAIR(3)); else attron(COLOR_PAIR(2));
        mvprintw(y + 4, x + 29, " < No > ");
        attroff(A_BOLD | COLOR_PAIR(3));
        refresh();
        int ch = getch();
        if (ch == KEY_LEFT || ch == KEY_RIGHT || ch == '\t') yes = !yes;
        else if (ch == 'y' || ch == 'Y') return true;
        else if (ch == 'n' || ch == 'N') return false;
        else if (ch == '\n' || ch == '\r') return yes;
    }
}

int main(int argc, char **argv) {
    const char *config_in = (argc > 1) ? argv[1] : "Config.in";
    const char *dot_config = (argc > 2) ? argv[2] : ".config";

    if (parse_config_in(config_in) != 0) return 1;
    load_dot_config(dot_config);

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_BLUE);   /* Desktop background */
        init_pair(2, COLOR_BLACK, COLOR_WHITE);  /* Dialog body */
        init_pair(3, COLOR_WHITE, COLOR_BLUE);   /* Selected row */
        init_pair(4, COLOR_BLUE, COLOR_WHITE);   /* Titles / accents */
        init_pair(5, COLOR_YELLOW, COLOR_BLUE);  /* Top banner */
    }

    int cur_menu = -1; /* -1 = top-level menu list */
    int sel = 0;

    for (;;) {
        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        bkgd(COLOR_PAIR(1));
        erase();

        attron(A_BOLD | COLOR_PAIR(5));
        mvprintw(0, 1, "%s", main_title);
        attroff(A_BOLD | COLOR_PAIR(5));
        attron(COLOR_PAIR(1));
        mvhline(1, 0, ACS_HLINE, cols);

        int bh = rows - 4;
        int bw = cols - 4;
        if (bh < 12) bh = 12;
        if (bw < 50) bw = 50;
        int by = 2, bx = 2;

        const char *box_title = (cur_menu < 0) ? ".config - larper-tools" : menus[cur_menu].title;
        draw_box(by, bx, bh, bw, box_title);

        attron(COLOR_PAIR(2));
        mvprintw(by + 1, bx + 2, "Arrow keys navigate. <Enter> selects submenus. <Space> toggles [*]/[ ].");
        mvprintw(by + 2, bx + 2, "<H> or <?> for Help. <S> saves. <Esc>/<Q> exits.");
        mvhline(by + 3, bx + 1, ACS_HLINE, bw - 2);

        int list_idx[MAX_ITEMS];
        int count = 0;
        if (cur_menu < 0) {
            count = num_menus;
        } else {
            for (int i = 0; i < num_items; i++) {
                if (items[i].menu_idx == cur_menu) list_idx[count++] = i;
            }
        }
        if (sel >= count) sel = count > 0 ? count - 1 : 0;
        if (sel < 0) sel = 0;

        int list_h = bh - 7;
        for (int i = 0; i < count && i < list_h; i++) {
            int r = by + 4 + i;
            if (i == sel) attron(A_BOLD | COLOR_PAIR(3));
            else attron(COLOR_PAIR(2));
            mvhline(r, bx + 2, ' ', bw - 4);
            if (cur_menu < 0) {
                int enabled = 0, total = 0;
                for (int k = 0; k < num_items; k++) {
                    if (items[k].menu_idx == i) {
                        total++;
                        if (items[k].value) enabled++;
                    }
                }
                mvprintw(r, bx + 4, "%-48.*s (%d/%d) --->", bw - 20, menus[i].title, enabled, total);
            } else {
                ConfigItem *it = &items[list_idx[i]];
                mvprintw(r, bx + 4, "[%c] %.*s", it->value ? '*' : ' ', bw - 12, it->prompt);
            }
            if (i == sel) attroff(A_BOLD | COLOR_PAIR(3));
            else attroff(COLOR_PAIR(2));
        }

        attron(COLOR_PAIR(2));
        mvhline(by + bh - 3, bx + 1, ACS_HLINE, bw - 2);
        mvprintw(by + bh - 2, bx + 4, "<Select>    <Exit>    <Help>    <Save>    %s",
                 modified ? "[MODIFIED]" : "[SAVED]   ");
        attroff(COLOR_PAIR(2));
        refresh();

        int ch = getch();
        if (ch == KEY_UP || ch == 'k') {
            if (sel > 0) sel--;
        } else if (ch == KEY_DOWN || ch == 'j') {
            if (sel < count - 1) sel++;
        } else if (ch == '\n' || ch == '\r' || ch == KEY_RIGHT) {
            if (cur_menu < 0 && count > 0) {
                cur_menu = sel;
                sel = 0;
            } else if (cur_menu >= 0 && count > 0) {
                items[list_idx[sel]].value = !items[list_idx[sel]].value;
                modified = true;
            }
        } else if (ch == ' ') {
            if (cur_menu >= 0 && count > 0) {
                items[list_idx[sel]].value = !items[list_idx[sel]].value;
                modified = true;
            } else if (cur_menu < 0 && count > 0) {
                cur_menu = sel;
                sel = 0;
            }
        } else if (ch == 'y' || ch == 'Y') {
            if (cur_menu >= 0 && count > 0) {
                items[list_idx[sel]].value = true;
                modified = true;
            }
        } else if (ch == 'n' || ch == 'N') {
            if (cur_menu >= 0 && count > 0) {
                items[list_idx[sel]].value = false;
                modified = true;
            }
        } else if (ch == 'h' || ch == 'H' || ch == '?') {
            if (cur_menu >= 0 && count > 0) {
                show_help(&items[list_idx[sel]]);
            }
        } else if (ch == 's' || ch == 'S') {
            save_dot_config(dot_config);
        } else if (ch == 27 || ch == 'q' || ch == 'Q' || ch == KEY_LEFT) {
            if (cur_menu >= 0) {
                sel = cur_menu;
                cur_menu = -1;
            } else {
                if (modified) {
                    if (confirm_save()) save_dot_config(dot_config);
                }
                break;
            }
        }
    }

    endwin();
    printf("\n*** End of larper-tools configuration.\n");
    printf("*** Execute 'make' or 'su -c \"make clean install\"' to build and install.\n\n");
    return 0;
}

#
# larper-tools — BusyBox-style multicall build system
#
# Typical workflow:
#   make fetch
#   make menuconfig
#   make
#   su -c "make clean install"
#

CC          ?= gcc
LD          ?= ld
OBJCOPY     ?= objcopy
STRIP       ?= strip
PKG_CONFIG  ?= pkg-config

PREFIX      ?= /usr/local
BINDIR      := $(DESTDIR)$(PREFIX)/bin
DATADIR     := $(DESTDIR)$(PREFIX)/share/larper-tools

BUILD       := build
SRC_ROOT    := sources
TARGET      := larper-tools
MCONF       := scripts/kconfig/mconf

NCURSES_CFLAGS := $(shell $(PKG_CONFIG) --cflags ncursesw 2>/dev/null)
NCURSES_LIBS   := $(shell $(PKG_CONFIG) --libs ncursesw 2>/dev/null || echo "-lncursesw -ltinfow")
PANEL_CFLAGS   := $(shell $(PKG_CONFIG) --cflags panelw 2>/dev/null)
PANEL_LIBS     := $(shell $(PKG_CONFIG) --libs panelw 2>/dev/null || echo "-lpanelw")

CFLAGS         ?= -O2
APPLET_CFLAGS  := $(CFLAGS) -w -fcommon
MAIN_CFLAGS    := $(CFLAGS) -Wall -Wextra -Iinclude

# Sync .config -> include/autoconf.h + build/config.mk when building
NO_CONFIG_TARGETS := fetch pull menuconfig defconfig allyesconfig allnoconfig clean distclean mrproper help uninstall scripts/kconfig/mconf
ifeq ($(filter $(NO_CONFIG_TARGETS),$(MAKECMDGOALS)),)
  ifeq ($(wildcard .config),)
    $(info *** No .config found — generating default configuration (make defconfig))
    _DEF := $(shell python3 scripts/gen-autoconf.py defconfig Config.in .config)
  endif
  _SYNC := $(shell python3 scripts/gen-autoconf.py sync Config.in .config include/autoconf.h $(BUILD)/config.mk)
  -include $(BUILD)/config.mk
endif

# Also sync config if 'install' is combined with 'clean' (e.g. `su -c "make clean install"`)
ifneq ($(filter install,$(MAKECMDGOALS)),)
  ifeq ($(wildcard .config),)
    _DEF := $(shell python3 scripts/gen-autoconf.py defconfig Config.in .config)
  endif
  _SYNC := $(shell python3 scripts/gen-autoconf.py sync Config.in .config include/autoconf.h $(BUILD)/config.mk)
  -include $(BUILD)/config.mk
endif

include applets/rules.mk

UNIQUE_LIBS := $(sort $(APPLET_LIBS))

.PHONY: all fetch pull menuconfig defconfig allyesconfig allnoconfig check-sources clean distclean mrproper install uninstall test help

all: check-sources $(TARGET)

$(BUILD):
	@mkdir -p $(BUILD)

check-sources:
	@if [ ! -d "$(SRC_ROOT)/cmatrix" ] || [ ! -d "$(SRC_ROOT)/neofetch" ]; then \
		echo "ERROR: Upstream sources not found in $(SRC_ROOT)/."; \
		echo "Run 'make fetch' first to pull all 10 upstream repositories."; \
		exit 1; \
	fi

fetch pull:
	@bash scripts/fetch-sources.sh

$(MCONF): scripts/kconfig/mconf.c
	@echo "  HOSTCC  $@"
	@$(CC) -O2 -Wall $(NCURSES_CFLAGS) $< -o $@ $(NCURSES_LIBS)

menuconfig: $(MCONF)
	@$(MCONF) Config.in .config
	@python3 scripts/gen-autoconf.py sync Config.in .config include/autoconf.h $(BUILD)/config.mk

defconfig:
	@python3 scripts/gen-autoconf.py defconfig Config.in .config
	@python3 scripts/gen-autoconf.py sync Config.in .config include/autoconf.h $(BUILD)/config.mk

allyesconfig:
	@python3 scripts/gen-autoconf.py allyesconfig Config.in .config
	@python3 scripts/gen-autoconf.py sync Config.in .config include/autoconf.h $(BUILD)/config.mk

allnoconfig:
	@python3 scripts/gen-autoconf.py allnoconfig Config.in .config
	@python3 scripts/gen-autoconf.py sync Config.in .config include/autoconf.h $(BUILD)/config.mk

$(BUILD)/main.o: src/main.c include/applets.h include/autoconf.h | $(BUILD)
	@echo "  CC      src/main.c"
	@$(CC) $(MAIN_CFLAGS) -c src/main.c -o $@

$(TARGET): $(BUILD)/main.o $(APPLET_OBJS)
	@echo "  LINK    $(TARGET)"
	@$(CC) $(CFLAGS) -o $@ $(BUILD)/main.o $(APPLET_OBJS) $(UNIQUE_LIBS)
ifeq ($(CONFIG_STRIP_BINARY),y)
	@echo "  STRIP   $(TARGET)"
	@$(STRIP) $@
endif
	@echo ""
	@echo "=== Built $(TARGET) ($$(du -h $(TARGET) | cut -f1)) ==="
	@echo "Enabled applets: $$(./$(TARGET) --list | tr '\n' ' ')"

# Note: `make clean install` works just like BusyBox/suckless:
# clean removes build objects while keeping .config and sources/.
clean:
	@echo "  CLEAN   $(BUILD)/ $(TARGET) $(MCONF)"
	@rm -rf $(BUILD) $(TARGET) $(MCONF)

distclean: clean
	@echo "  CLEAN   .config include/autoconf.h"
	@rm -f .config .config.old include/autoconf.h

mrproper: distclean
	@echo "  CLEAN   $(SRC_ROOT)/"
	@rm -rf $(SRC_ROOT)

install: check-sources $(TARGET)
	@echo "  INSTALL $(BINDIR)/$(TARGET)"
	@install -d -m 0755 $(BINDIR)
	@install -m 0755 $(TARGET) $(BINDIR)/$(TARGET)
ifeq ($(CONFIG_FIGLET),y)
	@install -d -m 0755 $(DATADIR)/figlet
	@cp -f $(SRC_ROOT)/figlet/fonts/*.flf $(DATADIR)/figlet/ 2>/dev/null || true
	@cp -f $(SRC_ROOT)/figlet/fonts/*.flc $(DATADIR)/figlet/ 2>/dev/null || true
endif
ifeq ($(CONFIG_INSTALL_SYMLINKS),y)
	@./$(TARGET) --install -s $(BINDIR)
endif
	@echo "=== Installed larper-tools into $(BINDIR) ==="

uninstall:
	@echo "  UNINSTALL $(BINDIR)/$(TARGET)"
	@if [ -x "$(BINDIR)/$(TARGET)" ]; then \
		for ap in $$($(BINDIR)/$(TARGET) --list); do \
			rm -f "$(BINDIR)/$$ap"; \
		done; \
	fi
	@rm -f "$(BINDIR)/$(TARGET)"
	@rm -rf "$(DATADIR)"

test: $(TARGET)
	@echo "=== Testing larper-tools multicall binary ==="
	@./$(TARGET) --version
	@./$(TARGET) --list
	@echo "--- figlet ---"
	@./$(TARGET) figlet "LARP" | head -n 6
	@echo "--- lolcat ---"
	@echo "RAINBOW HACKER POSER" | ./$(TARGET) lolcat
	@echo "--- neofetch --stdout ---"
	@./$(TARGET) neofetch --stdout | head -n 10
	@echo "--- cbonsai -p (static print) ---"
	@./$(TARGET) cbonsai -p | head -n 8
	@echo "--- pipes.sh -h ---"
	@./$(TARGET) pipes.sh -h | head -n 5
	@echo "--- cmatrix -V ---"
	@./$(TARGET) cmatrix -V
	@echo "--- nms -v ---"
	@./$(TARGET) nms -v
	@echo "--- nyancat -h ---"
	@./$(TARGET) nyancat -h | head -n 5
	@echo "--- tty-clock -i ---"
	@./$(TARGET) tty-clock -i
	@echo "--- BusyBox argv[0] symlink dispatch test ---"
	@TMPD=$$(mktemp -d) && cp $(TARGET) "$$TMPD/$(TARGET)" && "$$TMPD/$(TARGET)" --install -s "$$TMPD" >/dev/null && \
		"$$TMPD/figlet" "OK" | head -n 2 && \
		echo "SYMLINK OK" | "$$TMPD/lolcat" >/dev/null && \
		rm -rf "$$TMPD"
	@echo "=== ALL TESTS PASSED ==="

help:
	@echo "larper-tools BusyBox-style build targets:"
	@echo "  make fetch          - Clone the 10 upstream Linux larp/poser repos into sources/"
	@echo "  make menuconfig     - Interactive ncurses BusyBox-style configuration menu"
	@echo "  make defconfig      - Enable all 10 applets by default (.config)"
	@echo "  make allyesconfig   - Enable all options"
	@echo "  make allnoconfig    - Disable all options"
	@echo "  make                - Build the single larper-tools multicall binary"
	@echo "  make test           - Run automated verification on all enabled applets"
	@echo "  make clean          - Remove build artifacts (keeps .config and sources/)"
	@echo "  make distclean      - Also remove .config and include/autoconf.h"
	@echo "  make mrproper       - Also remove downloaded sources/"
	@echo "  make install        - Install binary + BusyBox symlinks (PREFIX=/usr/local)"
	@echo "  make uninstall      - Remove installed binary and symlinks"

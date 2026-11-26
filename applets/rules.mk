# Per-applet compilation & symbol-isolation rules for larper-tools.
# Each upstream C tool is compiled with -Dmain=<applet>_main and localized via
# `ld -r` + `objcopy --keep-global-symbol=<applet>_main` so no internal symbols
# ever collide across the 10 bundled tools.

APPLET_OBJS :=
APPLET_LIBS :=

# 1. cmatrix (abishekvashok/cmatrix)
ifeq ($(CONFIG_CMATRIX),y)
APPLET_OBJS += $(BUILD)/applet_cmatrix.o
APPLET_LIBS += $(NCURSES_LIBS)

$(BUILD)/applet_cmatrix.o: $(SRC_ROOT)/cmatrix/cmatrix.c | $(BUILD)
	@echo "  CC      cmatrix"
	@$(CC) $(APPLET_CFLAGS) $(NCURSES_CFLAGS) \
		-DEXCLUDE_CONFIG_H -DVERSION='"2.0"' \
		-DHAVE_SYS_IOCTL_H -DHAVE_UNISTD_H -DHAVE_TERMIOS_H \
		-DHAVE_NCURSES_H -DHAVE_USE_DEFAULT_COLORS -DHAVE_RESIZETERM \
		-Dmain=cmatrix_main \
		-c $< -o $(BUILD)/cmatrix_raw.o
	@$(OBJCOPY) --keep-global-symbol=cmatrix_main $(BUILD)/cmatrix_raw.o $@
endif

# 2. sl (mtoyoda/sl)
ifeq ($(CONFIG_SL),y)
APPLET_OBJS += $(BUILD)/applet_sl.o
APPLET_LIBS += $(NCURSES_LIBS)

$(BUILD)/applet_sl.o: $(SRC_ROOT)/sl/sl.c | $(BUILD)
	@echo "  CC      sl"
	@$(CC) $(APPLET_CFLAGS) $(NCURSES_CFLAGS) \
		-I$(SRC_ROOT)/sl -Dmain=sl_main \
		-c $< -o $(BUILD)/sl_raw.o
	@$(OBJCOPY) --keep-global-symbol=sl_main $(BUILD)/sl_raw.o $@
endif

# 3. cbonsai (jallbrit/cbonsai)
ifeq ($(CONFIG_CBONSAI),y)
APPLET_OBJS += $(BUILD)/applet_cbonsai.o
APPLET_LIBS += $(PANEL_LIBS) $(NCURSES_LIBS)

$(BUILD)/applet_cbonsai.o: $(SRC_ROOT)/cbonsai/cbonsai.c | $(BUILD)
	@echo "  CC      cbonsai"
	@$(CC) $(APPLET_CFLAGS) $(NCURSES_CFLAGS) $(PANEL_CFLAGS) \
		-DCBONSAI_VERSION='"1.3.1"' -Dmain=cbonsai_main \
		-c $< -o $(BUILD)/cbonsai_raw.o
	@$(OBJCOPY) --keep-global-symbol=cbonsai_main $(BUILD)/cbonsai_raw.o $@
endif

# 4. nms / no-more-secrets (bartobri/no-more-secrets)
ifeq ($(CONFIG_NMS),y)
APPLET_OBJS += $(BUILD)/applet_nms.o

NMS_DIR := $(SRC_ROOT)/no-more-secrets/src
NMS_SUB_OBJS := \
	$(BUILD)/nms_input.o \
	$(BUILD)/nms_error.o \
	$(BUILD)/nms_charset.o \
	$(BUILD)/nms_termio.o \
	$(BUILD)/nms_effect.o \
	$(BUILD)/nms_main.o

$(BUILD)/nms_input.o: $(NMS_DIR)/input.c | $(BUILD)
	@$(CC) $(APPLET_CFLAGS) -I$(NMS_DIR) -c $< -o $@
$(BUILD)/nms_error.o: $(NMS_DIR)/error.c | $(BUILD)
	@$(CC) $(APPLET_CFLAGS) -I$(NMS_DIR) -c $< -o $@
$(BUILD)/nms_charset.o: $(NMS_DIR)/nmscharset.c | $(BUILD)
	@$(CC) $(APPLET_CFLAGS) -I$(NMS_DIR) -c $< -o $@
$(BUILD)/nms_termio.o: $(NMS_DIR)/nmstermio.c | $(BUILD)
	@$(CC) $(APPLET_CFLAGS) -I$(NMS_DIR) -c $< -o $@
$(BUILD)/nms_effect.o: $(NMS_DIR)/nmseffect.c | $(BUILD)
	@$(CC) $(APPLET_CFLAGS) -I$(NMS_DIR) -c $< -o $@
$(BUILD)/nms_main.o: $(NMS_DIR)/nms.c | $(BUILD)
	@$(CC) $(APPLET_CFLAGS) -I$(NMS_DIR) -Dmain=nms_main -c $< -o $@

$(BUILD)/applet_nms.o: $(NMS_SUB_OBJS)
	@echo "  LD      nms"
	@$(LD) -r $(NMS_SUB_OBJS) -o $(BUILD)/nms_merged.o
	@$(OBJCOPY) --keep-global-symbol=nms_main $(BUILD)/nms_merged.o $@
endif

# 5. tty-clock (xorg62/tty-clock)
ifeq ($(CONFIG_TTY_CLOCK),y)
APPLET_OBJS += $(BUILD)/applet_tty_clock.o
APPLET_LIBS += $(NCURSES_LIBS)

$(BUILD)/applet_tty_clock.o: $(SRC_ROOT)/tty-clock/ttyclock.c | $(BUILD)
	@echo "  CC      tty-clock"
	@$(CC) $(APPLET_CFLAGS) $(NCURSES_CFLAGS) \
		-I$(SRC_ROOT)/tty-clock -Dmain=tty_clock_main \
		-c $< -o $(BUILD)/tty_clock_raw.o
	@$(OBJCOPY) --keep-global-symbol=tty_clock_main $(BUILD)/tty_clock_raw.o $@
endif

# 6. lolcat (jaseg/lolcat)
ifeq ($(CONFIG_LOLCAT),y)
APPLET_OBJS += $(BUILD)/applet_lolcat.o
APPLET_LIBS += -lm

$(BUILD)/applet_lolcat.o: $(SRC_ROOT)/lolcat/lolcat.c | $(BUILD)
	@echo "  CC      lolcat"
	@$(CC) $(APPLET_CFLAGS) -std=c11 -I$(SRC_ROOT)/lolcat -Dmain=lolcat_main \
		-c $< -o $(BUILD)/lolcat_raw.o
	@$(OBJCOPY) --keep-global-symbol=lolcat_main $(BUILD)/lolcat_raw.o $@
endif

# 7. figlet (cmatsuoka/figlet)
ifeq ($(CONFIG_FIGLET),y)
APPLET_OBJS += $(BUILD)/applet_figlet.o

FIGLET_DIR := $(SRC_ROOT)/figlet
FIGLET_FONTDIR := $(PREFIX)/share/larper-tools/figlet
FIGLET_CFLAGS := $(APPLET_CFLAGS) -std=gnu89 -DTLF_FONTS \
	-DDEFAULTFONTDIR='"$(FIGLET_FONTDIR)"' -DDEFAULTFONTFILE='"standard"' \
	-I$(FIGLET_DIR)

FIGLET_SUB_OBJS := \
	$(BUILD)/figlet_main.o \
	$(BUILD)/figlet_zipio.o \
	$(BUILD)/figlet_crc.o \
	$(BUILD)/figlet_inflate.o \
	$(BUILD)/figlet_utf8.o \
	$(BUILD)/figlet_wrapper.o

$(BUILD)/figlet_standard_flf.h: $(FIGLET_DIR)/fonts/standard.flf | $(BUILD)
	@python3 scripts/embed-script.py $< figlet_standard_flf_data $@

$(BUILD)/figlet_main.o: $(FIGLET_DIR)/figlet.c | $(BUILD)
	@$(CC) $(FIGLET_CFLAGS) -Dmain=figlet_upstream_main -c $< -o $@
$(BUILD)/figlet_zipio.o: $(FIGLET_DIR)/zipio.c | $(BUILD)
	@$(CC) $(FIGLET_CFLAGS) -c $< -o $@
$(BUILD)/figlet_crc.o: $(FIGLET_DIR)/crc.c | $(BUILD)
	@$(CC) $(FIGLET_CFLAGS) -c $< -o $@
$(BUILD)/figlet_inflate.o: $(FIGLET_DIR)/inflate.c | $(BUILD)
	@$(CC) $(FIGLET_CFLAGS) -c $< -o $@
$(BUILD)/figlet_utf8.o: $(FIGLET_DIR)/utf8.c | $(BUILD)
	@$(CC) $(FIGLET_CFLAGS) -c $< -o $@
$(BUILD)/figlet_wrapper.o: applets/wrappers/figlet_applet.c $(BUILD)/figlet_standard_flf.h | $(BUILD)
	@$(CC) $(APPLET_CFLAGS) -I$(BUILD) -DLARPER_FIGLET_FONTDIR='"$(FIGLET_FONTDIR)"' -c $< -o $@

$(BUILD)/applet_figlet.o: $(FIGLET_SUB_OBJS)
	@echo "  LD      figlet"
	@$(LD) -r $(FIGLET_SUB_OBJS) -o $(BUILD)/figlet_merged.o
	@$(OBJCOPY) --keep-global-symbol=figlet_main $(BUILD)/figlet_merged.o $@
endif

# 8. nyancat (klange/nyancat)
ifeq ($(CONFIG_NYANCAT),y)
APPLET_OBJS += $(BUILD)/applet_nyancat.o

$(BUILD)/applet_nyancat.o: $(SRC_ROOT)/nyancat/src/nyancat.c | $(BUILD)
	@echo "  CC      nyancat"
	@$(CC) $(APPLET_CFLAGS) -std=gnu99 -I$(SRC_ROOT)/nyancat/src -Dmain=nyancat_main \
		-c $< -o $(BUILD)/nyancat_raw.o
	@$(OBJCOPY) --keep-global-symbol=nyancat_main $(BUILD)/nyancat_raw.o $@
endif

# 9. pipes.sh (pipeseroni/pipes.sh) - embedded directly into the binary
ifeq ($(CONFIG_PIPES),y)
APPLET_OBJS += $(BUILD)/applet_pipes.o

$(BUILD)/pipes_embedded.h: $(SRC_ROOT)/pipes.sh/pipes.sh | $(BUILD)
	@echo "  EMBED   pipes.sh"
	@python3 scripts/embed-script.py $< pipes_sh_data $@

$(BUILD)/applet_pipes.o: applets/wrappers/pipes_applet.c $(BUILD)/pipes_embedded.h | $(BUILD)
	@echo "  CC      pipes.sh"
	@$(CC) $(APPLET_CFLAGS) -I$(BUILD) -Iapplets/wrappers -c $< -o $(BUILD)/pipes_raw.o
	@$(OBJCOPY) --keep-global-symbol=pipes_main $(BUILD)/pipes_raw.o $@
endif

# 10. neofetch (dylanaraps/neofetch) - embedded directly into the binary
ifeq ($(CONFIG_NEOFETCH),y)
APPLET_OBJS += $(BUILD)/applet_neofetch.o

$(BUILD)/neofetch_embedded.h: $(SRC_ROOT)/neofetch/neofetch | $(BUILD)
	@echo "  EMBED   neofetch"
	@python3 scripts/embed-script.py $< neofetch_data $@

$(BUILD)/applet_neofetch.o: applets/wrappers/neofetch_applet.c $(BUILD)/neofetch_embedded.h | $(BUILD)
	@echo "  CC      neofetch"
	@$(CC) $(APPLET_CFLAGS) -I$(BUILD) -Iapplets/wrappers -c $< -o $(BUILD)/neofetch_raw.o
	@$(OBJCOPY) --keep-global-symbol=neofetch_main $(BUILD)/neofetch_raw.o $@
endif

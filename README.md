# larper-tools

A **BusyBox-style multicall single binary** that pulls the upstream source code of the **10 most iconic Linux terminal LARP / poser tools**, lets you configure which applets to compile in via an interactive ncurses `make menuconfig`, and compiles them all into ONE standalone executable (`larper-tools`) with BusyBox `argv[0]` symlink dispatch.

## Included Upstream Tools (10 Applets)

| # | Applet | Upstream | Description |
|---|--------|----------|-------------|
| 1 | `cmatrix` | [abishekvashok/cmatrix](https://github.com/abishekvashok/cmatrix) | Classic Matrix digital rain (C + ncurses) |
| 2 | `sl` | [mtoyoda/sl](https://github.com/mtoyoda/sl) | Steam Locomotive typo punishment (C + ncurses) |
| 3 | `cbonsai` | [jallbrit/cbonsai](https://gitlab.com/jallbrit/cbonsai) | Procedural ASCII bonsai tree (C + ncurses/panel) |
| 4 | `nms` | [bartobri/no-more-secrets](https://github.com/bartobri/no-more-secrets) | *Sneakers* movie decryption effect (pure C) |
| 5 | `tty-clock` | [xorg62/tty-clock](https://github.com/xorg62/tty-clock) | Giant ncurses digital clock (C + ncurses) |
| 6 | `lolcat` | [jaseg/lolcat](https://github.com/jaseg/lolcat) | Blazing-fast C rainbow terminal filter (pure C) |
| 7 | `figlet` | [cmatsuoka/figlet](https://github.com/cmatsuoka/figlet) | Large ASCII art banners with embedded `standard.flf` (pure C) |
| 8 | `nyancat` | [klange/nyancat](https://github.com/klange/nyancat) | Animated ANSI Nyan Cat (pure C) |
| 9 | `pipes.sh` (`pipes`) | [pipeseroni/pipes.sh](https://github.com/pipeseroni/pipes.sh) | Animated neon terminal pipes (embedded in binary) |
| 10 | `neofetch` | [dylanaraps/neofetch](https://github.com/dylanaraps/neofetch) | Classic Linux rice system info flex (embedded in binary) |

## Quickstart (BusyBox / Suckless Workflow)

```bash
cd larper-tools

# 1. Pull all 10 upstream source repositories into sources/
make fetch

# 2. Configure enabled applets interactively (ncurses menuconfig)
make menuconfig
# (or skip menuconfig and enable all 10 with: make defconfig)

# 3. Clean build & install into /usr/local/bin with BusyBox symlinks
su -c "make clean install"
# (or install to a custom prefix without root:)
make clean install PREFIX="$HOME/.local"
```

## Usage

Once built or installed, invoke any tool either as a subcommand or directly via its BusyBox symlink:

```bash
larper-tools --list
larper-tools cmatrix -C cyan
ls -la | larper-tools nms -a
larper-tools figlet "HACKER" | larper-tools lolcat
larper-tools neofetch

# Or directly via symlink after `make install`:
cmatrix
cbonsai -l
pipes.sh
sl
```

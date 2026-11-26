#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="${ROOT_DIR}/sources"
UPDATE=0

if [[ "${1:-}" == "--update" ]]; then
    UPDATE=1
fi

mkdir -p "${SRC_DIR}"

# Name | Git URL
REPOS=(
    "cmatrix|https://github.com/abishekvashok/cmatrix.git"
    "sl|https://github.com/mtoyoda/sl.git"
    "cbonsai|https://gitlab.com/jallbrit/cbonsai.git"
    "no-more-secrets|https://github.com/bartobri/no-more-secrets.git"
    "tty-clock|https://github.com/xorg62/tty-clock.git"
    "lolcat|https://github.com/jaseg/lolcat.git"
    "figlet|https://github.com/cmatsuoka/figlet.git"
    "nyancat|https://github.com/klange/nyancat.git"
    "pipes.sh|https://github.com/pipeseroni/pipes.sh.git"
    "neofetch|https://github.com/dylanaraps/neofetch.git"
)

echo "=== larper-tools: fetching upstream sources into ${SRC_DIR} ==="

for entry in "${REPOS[@]}"; do
    IFS='|' read -r name url <<< "${entry}"
    target="${SRC_DIR}/${name}"
    if [[ -d "${target}/.git" ]]; then
        if [[ "${UPDATE}" -eq 1 ]]; then
            echo "  [UPDATE] ${name} (${url})"
            git -C "${target}" pull --ff-only --depth 1 || true
        else
            echo "  [OK]     ${name} (already cloned)"
        fi
    else
        echo "  [CLONE]  ${name} <- ${url}"
        git clone --depth 1 "${url}" "${target}"
    fi
done

echo "=== All 10 upstream sources ready ==="

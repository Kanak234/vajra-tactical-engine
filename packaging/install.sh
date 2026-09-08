#!/usr/bin/env bash
# Installs Vajra system-wide (or to a prefix of your choice).
#   sudo ./packaging/install.sh              -> /usr/local
#   ./packaging/install.sh ~/.local          -> user-local, no sudo
set -euo pipefail

PREFIX="${1:-/usr/local}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo ">> building release"
cmake -S "$ROOT" -B "$ROOT/build" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX"
cmake --build "$ROOT/build" -j"$(nproc)"

echo ">> installing to $PREFIX"
cmake --install "$ROOT/build"

install -Dm644 "$ROOT/packaging/vajra.desktop" \
        "$PREFIX/share/applications/vajra.desktop"

echo ">> done. Run with: $PREFIX/bin/vajra"

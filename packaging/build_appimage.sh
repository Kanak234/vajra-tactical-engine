#!/usr/bin/env bash
# Builds a self-contained AppImage. Run from the project root:
#   ./packaging/build_appimage.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build-appimage"
APPDIR="$ROOT/Vajra.AppDir"

command -v cmake >/dev/null || { echo "cmake not found"; exit 1; }

echo ">> configuring release build"
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr
echo ">> building"
cmake --build "$BUILD" -j"$(nproc)"

echo ">> staging AppDir"
rm -rf "$APPDIR"
DESTDIR="$APPDIR" cmake --install "$BUILD"

install -Dm644 "$ROOT/packaging/vajra.desktop" "$APPDIR/vajra.desktop"
# Minimal placeholder icon so the AppImage tooling is satisfied.
if [ ! -f "$APPDIR/vajra.png" ]; then
    printf '\x89PNG\r\n\x1a\n' > "$APPDIR/vajra.png"
fi

cat > "$APPDIR/AppRun" <<'RUN'
#!/bin/sh
HERE="$(dirname "$(readlink -f "$0")")"
# Assets live beside the binary; cd so relative paths resolve either way.
cd "$HERE/usr/share/vajra" 2>/dev/null || true
exec "$HERE/usr/bin/vajra" "$@"
RUN
chmod +x "$APPDIR/AppRun"

if command -v appimagetool >/dev/null; then
    echo ">> packing AppImage"
    appimagetool "$APPDIR" "$ROOT/Vajra-x86_64.AppImage"
    echo ">> done: Vajra-x86_64.AppImage"
else
    echo ">> appimagetool not installed."
    echo "   Get it from https://github.com/AppImage/AppImageKit/releases"
    echo "   The staged AppDir is ready at: $APPDIR"
fi

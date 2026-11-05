#!/usr/bin/env bash
# Downloads raylib into vendor/raylib as an internal (vendored) dependency.
# vendor/ is gitignored, so run this once after cloning the repo.
#
#   sh setup-deps.sh
#
# Detects the platform and pulls the matching official raylib 5.5 release.
set -e

RAYLIB_VERSION="5.5"
VENDOR_DIR="$(cd "$(dirname "$0")" && pwd)/vendor"
DEST="$VENDOR_DIR/raylib"

UNAME_S="$(uname -s 2>/dev/null || echo Windows_NT)"
case "$UNAME_S" in
    Darwin)                 PKG="raylib-${RAYLIB_VERSION}_macos"          ; EXT="tar.gz" ;;
    Linux)                  PKG="raylib-${RAYLIB_VERSION}_linux_amd64"    ; EXT="tar.gz" ;;
    MINGW*|MSYS*|CYGWIN*|Windows_NT) PKG="raylib-${RAYLIB_VERSION}_win64_mingw-w64" ; EXT="zip" ;;
    *) echo "Unsupported platform: $UNAME_S" >&2 ; exit 1 ;;
esac

URL="https://github.com/raysan5/raylib/releases/download/${RAYLIB_VERSION}/${PKG}.${EXT}"

if [ -f "$DEST/include/raylib.h" ]; then
    echo "raylib already present at $DEST — nothing to do."
    exit 0
fi

echo "Downloading $URL"
mkdir -p "$VENDOR_DIR"
cd "$VENDOR_DIR"
ARCHIVE="raylib-dl.${EXT}"
curl -fL -o "$ARCHIVE" "$URL"

echo "Extracting..."
if [ "$EXT" = "zip" ]; then
    # Git Bash/MinGW may not ship unzip. Prefer Windows' ZIP-capable tar over
    # the GNU tar that can also be on PATH; PowerShell covers older Windows.
    WINDOWS_TAR="${SYSTEMROOT:-C:/Windows}/System32/tar.exe"
    if command -v cygpath >/dev/null 2>&1; then
        WINDOWS_TAR="$(cygpath -u "$WINDOWS_TAR")"
    fi
    if command -v unzip >/dev/null 2>&1; then
        unzip -qo "$ARCHIVE"
    elif [ -x "$WINDOWS_TAR" ]; then
        "$WINDOWS_TAR" -xf "$ARCHIVE"
    elif command -v powershell.exe >/dev/null 2>&1; then
        powershell.exe -NoProfile -NonInteractive -Command \
            '$ErrorActionPreference = "Stop"; Expand-Archive -LiteralPath "raylib-dl.zip" -DestinationPath "." -Force'
    else
        echo "Cannot extract ZIP: need unzip, Windows tar.exe, or powershell.exe." >&2
        exit 1
    fi
else
    tar -xzf "$ARCHIVE"
fi

if [ ! -f "$PKG/include/raylib.h" ]; then
    echo "Extraction failed: $PKG/include/raylib.h is missing; leaving $DEST untouched." >&2
    exit 1
fi

rm -rf "$DEST"
mv "$PKG" "$DEST"
rm -f "$ARCHIVE"

echo "raylib $RAYLIB_VERSION installed at $DEST"

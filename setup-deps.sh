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
    unzip -q "$ARCHIVE"
else
    tar -xzf "$ARCHIVE"
fi

rm -rf "$DEST"
mv "$PKG" "$DEST"
rm -f "$ARCHIVE"

echo "raylib $RAYLIB_VERSION installed at $DEST"

#!/usr/bin/env bash
# Installs the Steamworks SDK (C++) into vendor/steamworks as an internal
# (vendored) dependency. vendor/ is gitignored, so run this once after cloning.
#
#   sh setup-steamworks.sh [path-to-sdk-zip-or-folder]
#
# Valve gates the official download behind a partner login, so this script
# cannot fetch it for you. Two ways to get the SDK in:
#
#   1. Preferred — download steamworks_sdk_<version>.zip while logged in at
#      https://partner.steamgames.com/downloads/list, then either drop it in the
#      repo root (or vendor/) and run this script bare, or pass its path — or the
#      path to an already-extracted sdk/ folder — as the first argument.
#
#   2. Fallback — with no zip present, this script pulls the redistributables
#      from the public SteamworksSDKCI mirror. Same headers and same signed
#      Valve binaries, repackaged; use it for development, but ship from the
#      official zip you accepted the SDK Access Agreement for.
#
# Everything is built in a staging dir and swapped into place only once it is
# known good, so a failed or interrupted run always leaves the existing install
# untouched rather than half-written.
#
# Resulting layout (headers include as <steam/steam_api.h>):
#
#   vendor/steamworks/include/steam/*.h
#   vendor/steamworks/lib/   steam_api64.lib, libsteam_api.so, libsteam_api.dylib
#   vendor/steamworks/bin/   steam_api64.dll  (copied next to the exe at build time)
set -e

MIRROR_VERSION="1.64"
ROOT="$(cd "$(dirname "$0")" && pwd)"
VENDOR_DIR="$ROOT/vendor"
DEST="$VENDOR_DIR/steamworks"

# An explicit path argument means "replace what's there", so only short-circuit
# on an existing install when the script is run bare.
if [ -z "${1:-}" ] && [ -f "$DEST/include/steam/steam_api.h" ]; then
    echo "Steamworks SDK already present at $DEST — nothing to do."
    echo "Pass a path to an official SDK zip or folder to replace it."
    exit 0
fi

WORK="$VENDOR_DIR/.steamworks-unpack"
STAGE="$VENDOR_DIR/.steamworks-stage"
# Never leave scratch dirs behind, on success or failure. $DEST is only ever
# touched by the swap at the very end, so an abort here cannot damage it.
trap 'rm -rf "$WORK" "$STAGE"' EXIT
rm -rf "$WORK" "$STAGE"
mkdir -p "$WORK" "$STAGE/include" "$STAGE/lib" "$STAGE/bin"

# MSYS/Git Bash: accept C:\foo\bar and C:/foo/bar as well as /c/foo/bar.
to_unix_path() {
    if [ -n "$1" ] && command -v cygpath >/dev/null 2>&1; then
        cygpath -u "$1"
    else
        printf '%s\n' "$1"
    fi
}

# unzip is not installed everywhere (Git Bash ships without it); Windows has
# bsdtar in System32, which reads zips fine. Steam's zips use backslash path
# separators, which makes both warn and exit 1 while still extracting
# correctly — so don't let set -e abort on that.
extract_zip() {
    if command -v unzip >/dev/null 2>&1; then
        unzip -q "$1" -d "$2" || true
    elif [ -x /c/Windows/System32/tar.exe ]; then
        mkdir -p "$2"
        ( cd "$2" && /c/Windows/System32/tar.exe -xf "$(cygpath -w "$1")" ) || true
    else
        echo "Need unzip or Windows bsdtar to unpack $1" >&2
        exit 1
    fi
}

# Prefer an official SDK the user downloaded themselves: an explicit path passed
# as $1 (zip or already-extracted folder), else a zip sitting in the repo root.
SOURCE="$(to_unix_path "${1:-}")"
if [ -z "$SOURCE" ]; then
    SOURCE="$(ls "$ROOT"/steamworks_sdk_*.zip "$VENDOR_DIR"/steamworks_sdk_*.zip 2>/dev/null | head -n 1 || true)"
fi

if [ -n "$SOURCE" ] && [ ! -e "$SOURCE" ]; then
    echo "Not found: $SOURCE" >&2
    echo "Quote Windows paths so bash keeps the backslashes:" >&2
    echo "  sh setup-steamworks.sh \"C:\\Users\\you\\Downloads\\steamworks_sdk_162.zip\"" >&2
    exit 1
fi

SDK=""
if [ -d "$SOURCE" ]; then
    echo "Using extracted SDK: $SOURCE"
    SDK="$SOURCE"
elif [ -n "$SOURCE" ]; then
    echo "Using official SDK zip: $SOURCE"
    extract_zip "$SOURCE" "$WORK"
    SDK="$WORK"
fi

if [ -n "$SDK" ]; then
    # The zip nests everything under sdk/; an extracted folder may or may not.
    [ -d "$SDK/public/steam" ] || SDK="$SDK/sdk"
    if [ ! -d "$SDK/public/steam" ]; then
        echo "No public/steam found under $SOURCE — is that the Steamworks SDK?" >&2
        exit 1
    fi

    cp -r "$SDK/public/steam" "$STAGE/include/"
    # The headers ship with a nested lib/ tree of encrypted-app-ticket binaries;
    # keep the include dir headers-only.
    rm -rf "$STAGE/include/steam/lib"

    RB="$SDK/redistributable_bin"
    cp "$RB/win64/steam_api64.dll"     "$STAGE/bin/" 2>/dev/null || true
    cp "$RB/win64/steam_api64.lib"     "$STAGE/lib/" 2>/dev/null || true
    cp "$RB/linux64/libsteam_api.so"   "$STAGE/lib/" 2>/dev/null || true
    cp "$RB/osx/libsteam_api.dylib"    "$STAGE/lib/" 2>/dev/null || true

    APPTICKET="$SDK/public/steam/lib"
    cp "$APPTICKET/win64/sdkencryptedappticket64.dll"    "$STAGE/bin/" 2>/dev/null || true
    cp "$APPTICKET/win64/sdkencryptedappticket64.lib"    "$STAGE/lib/" 2>/dev/null || true
    cp "$APPTICKET/linux64/libsdkencryptedappticket.so"  "$STAGE/lib/" 2>/dev/null || true
    cp "$APPTICKET/osx/libsdkencryptedappticket.dylib"   "$STAGE/lib/" 2>/dev/null || true
else
    case "$(uname -m 2>/dev/null || echo x86_64)" in
        arm64|aarch64) ARCH="arm64" ;;
        *)             ARCH="x64"   ;;
    esac
    PKG="SteamworksSDK-v${MIRROR_VERSION}.0_${ARCH}.zip"
    URL="https://github.com/julianxhokaxhiu/SteamworksSDKCI/releases/download/${MIRROR_VERSION}/${PKG}"

    echo "No steamworks_sdk_*.zip found in $ROOT — falling back to the public mirror."
    echo "Downloading $URL"
    curl -fL -o "$WORK/sdk.zip" "$URL"
    extract_zip "$WORK/sdk.zip" "$WORK"

    cp -r "$WORK/include/steam" "$STAGE/include/"
    cp "$WORK"/lib/steam/* "$STAGE/lib/"
    cp "$WORK"/bin/steam/* "$STAGE/bin/"
fi

# Only now, with a verified staging tree, is it safe to replace the install.
if [ ! -f "$STAGE/include/steam/steam_api.h" ]; then
    echo "Install failed: no steam_api.h was produced; leaving $DEST as it was." >&2
    exit 1
fi

rm -rf "$DEST"
mv "$STAGE" "$DEST"

echo "Steamworks SDK installed at $DEST"
echo "  headers: $(ls "$DEST"/include/steam/*.h | wc -l)   libs: $(ls "$DEST"/lib | wc -l)   bin: $(ls "$DEST"/bin | wc -l)"

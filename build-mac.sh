# Steamworks is optional — see setup-steamworks.sh. Guard Steam code with #ifdef STEAMWORKS_AVAILABLE.
STEAM_INC=""
STEAM_LIB=""
if [ -f vendor/steamworks/include/steam/steam_api.h ]; then
    STEAM_INC="-Ivendor/steamworks/include -DSTEAMWORKS_AVAILABLE=1"
    STEAM_LIB="-Lvendor/steamworks/lib -lsteam_api -Wl,-rpath,@loader_path"
    cp -f vendor/steamworks/lib/libsteam_api.dylib .
fi

g++ $(find . -name "*.cpp" -not -path "./vendor/*") -o therewillnotbeafoldernamedthis \
-Ivendor/raylib/include $STEAM_INC \
-Lvendor/raylib/lib $STEAM_LIB \
-lraylib \
-framework OpenGL \
-framework Cocoa \
-framework IOKit \
-framework CoreVideo && ./therewillnotbeafoldernamedthis

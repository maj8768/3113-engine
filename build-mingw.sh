# Steamworks is optional — see setup-steamworks.sh. Guard Steam code with #ifdef STEAMWORKS_AVAILABLE.
STEAM_INC=""
STEAM_LIB=""
if [ -f vendor/steamworks/include/steam/steam_api.h ]; then
    STEAM_INC="-Ivendor/steamworks/include -DSTEAMWORKS_AVAILABLE=1"
    STEAM_LIB="vendor/steamworks/bin/steam_api64.dll"
    cp -f vendor/steamworks/bin/steam_api64.dll .
fi

g++ $(find . -name "*.cpp" -not -path "./vendor/*") -o therewillnotbeafoldernamedthis.exe -Ivendor/raylib/include $STEAM_INC -Lvendor/raylib/lib $STEAM_LIB -lraylib -lopengl32 -lgdi32 -lwinmm -lws2_32 && ./therewillnotbeafoldernamedthis.exe

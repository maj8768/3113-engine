g++ $(find . -name "*.cpp") -o therewillnotbeafoldernamedthis.exe -Ivendor/raylib/include -Lvendor/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm && ./therewillnotbeafoldernamedthis.exe

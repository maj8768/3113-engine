g++ $(find . -name "*.cpp") -o therewillnotbeafoldernamedthis \
-Ivendor/raylib/include \
-Lvendor/raylib/lib \
-lraylib \
-framework OpenGL \
-framework Cocoa \
-framework IOKit \
-framework CoreVideo && ./therewillnotbeafoldernamedthis

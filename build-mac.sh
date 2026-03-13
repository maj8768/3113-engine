g++ $(find . -name "*.cpp") -o therewillnotbeafoldernamedthis \
-I/opt/homebrew/include \
-L/opt/homebrew/lib \
-lraylib \
-framework OpenGL \
-framework Cocoa \
-framework IOKit \
-framework CoreVideo && ./therewillnotbeafoldernamedthis

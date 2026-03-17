# Linux

gcc main.c ClosedGL.c -o main -lX11 -lGL

# macOS

clang main.c ClosedGL.c -o main -framework Cocoa -framework OpenGL

# Windows (mingw)

gcc main.c ClosedGL.c -o main -lopengl32 -lgdi32

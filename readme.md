# Linux

gcc -I. examples/main.c src/closedgl.c src/platform_x11.c -o test -lX11 -lGL -lm

# Windows (mingw)

gcc -I. examples/main.c src/closedgl.c src/platform_win.c -o test -lopengl32 -lgdi32 -luser32

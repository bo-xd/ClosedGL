#include "../ClosedGl.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(void) {
    ClosedGL_Window win;
    ClosedGL_CreateWindow(&win, 800, 600, "ClosedGL Example");
    int vsync = 1;
    ClosedGL_SetVSync(&win, vsync);
    int running = 1;
    int fullscreen = 0;
    double lastTime = ClosedGL_GetTime();
    double fpsAccum = 0.0;
    int frames = 0;
    glViewport(0, 0, win.width, win.height);
    glEnable(GL_DEPTH_TEST);
    while (running) {
        ClosedGL_Event event;
        while (ClosedGL_PollEvent(&win, &event)) {
            switch (event.type) {
                case CLOSEDGL_EVENT_QUIT:
                    running = 0;
                    break;
                case CLOSEDGL_EVENT_KEYDOWN: {
                    int k = event.data.key.key;
                    if (k == 27 || k == 'q' || k == 'Q') { running = 0; }
                    else if (k == 'f' || k == 'F') {
                        fullscreen = !fullscreen;
                        ClosedGL_SetFullscreen(&win, fullscreen);
                    } else if (k == 'v' || k == 'V') {
                        vsync = !vsync;
                        ClosedGL_SetVSync(&win, vsync);
                        printf("VSync: %s\n", vsync ? "On" : "Off");
                    }
                    break;
                }
                case CLOSEDGL_EVENT_WINDOW_RESIZE:
                    glViewport(0, 0, event.data.size.width, event.data.size.height);
                    break;
                case CLOSEDGL_EVENT_MOUSE_MOVE:
                    break;
                case CLOSEDGL_EVENT_MOUSE_BUTTON_DOWN:
                    printf("Mouse button %d down at %d,%d\n", event.data.mouse.button, event.data.mouse.x, event.data.mouse.y);
                    break;
                default:
                    break;
            }
        }
        double now = ClosedGL_GetTime();
        double t = now;
        glClearColor((float)(0.2 + 0.1 * (sin(t) * 0.5 + 0.5)),
                     (float)(0.3 + 0.1 * (cos(t * 1.3) * 0.5 + 0.5)),
                     0.35f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glRotatef((float)(t * 30.0), 0.0f, 0.0f, 1.0f);
        glBegin(GL_TRIANGLES);
          glColor3f(1.0f, 0.2f, 0.2f);
          glVertex2f(-0.6f, -0.4f);
          glColor3f(0.2f, 1.0f, 0.2f);
          glVertex2f(0.6f, -0.4f);
          glColor3f(0.2f, 0.2f, 1.0f);
          glVertex2f(0.0f, 0.6f);
        glEnd();
        ClosedGL_SwapBuffers(&win);
        frames++;
        fpsAccum += (ClosedGL_GetTime() - lastTime);
        if (fpsAccum >= 1.0) {
            printf("FPS: %d\n", frames);
            frames = 0;
            fpsAccum = 0.0;
        }
        lastTime = ClosedGL_GetTime();
    }
    ClosedGL_DestroyWindow(&win);
    return 0;
}

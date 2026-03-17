#include "ClosedGl.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  ClosedGL_Window win;
  ClosedGL_CreateWindow(&win, 800, 600, "ClosedGL Test");
  printf("Window created successfully!\n");

  int running = 1;
  while (running) {
    XEvent event;
    XNextEvent(win.unix_gl.display, &event);

    switch (event.type) {
      case Expose:
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ClosedGL_SwapBuffers(&win);
        break;
      case KeyPress:
        printf("Key pressed, closing window...\n");
        running = 0;
        break;
      case ClientMessage:
        running = 0;
        break;
    }
  }

  return 0;
}

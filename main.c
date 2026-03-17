#include "ClosedGl.h"
#include <stdio.h>

int main() {
  ClosedGL_Window win;
  ClosedGL_CreateWindow(&win, 800, 600, "ClosedGL Test");
  printf("Window created!\n");

  int running = 1;
  while (running) {
    ClosedGL_Event event;
    while (ClosedGL_PollEvent(&win, &event)) {
      if (event.type == CLOSEDGL_EVENT_QUIT) running = 0;
      if (event.type == CLOSEDGL_EVENT_KEYPRESS) { printf("Key: %d\n", event.key); running = 0; }
    }
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ClosedGL_SwapBuffers(&win);
  }

  ClosedGL_DestroyWindow(&win);
  return 0;
}

#pragma once

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#elif __APPLE__
#include <OpenGL/gl.h>
#include <Cocoa/Cocoa.h>
#else
#include <X11/Xlib.h>
#include <GL/glx.h>
#endif

typedef struct {
  int width;
  int height;
  union {
#ifdef _WIN32
    struct {
      HWND hwnd;
      HDC hdc;
      HGLRC hrc;
      HINSTANCE hinstance;
    } win;
#elif __APPLE__
    struct {
      NSWindow *window;
      NSOpenGLContext *context;
      NSApplication *app;
    } mac;
#else
    struct {
      Display *display;
      Window window;
      GLXContext context;
    } unix_gl;
#endif
  };
} ClosedGL_Window;

typedef enum {
  CLOSEDGL_EVENT_NONE,
  CLOSEDGL_EVENT_QUIT,
  CLOSEDGL_EVENT_KEYPRESS,
  CLOSEDGL_EVENT_EXPOSE,
} ClosedGL_EventType;

typedef struct {
  ClosedGL_EventType type;
  int key;
} ClosedGL_Event;

void ClosedGL_CreateWindow(ClosedGL_Window *win, int width, int height, const char *title);
void ClosedGL_SwapBuffers(ClosedGL_Window *win);
int ClosedGL_PollEvent(ClosedGL_Window *win, ClosedGL_Event *event);
void ClosedGL_DestroyWindow(ClosedGL_Window *win);

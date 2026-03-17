#pragma once
#ifdef _WIN32
#include <GL/gl.h>
#include <windows.h>
#else
#include <GL/glx.h>
#include <X11/Xlib.h>
#endif

typedef struct {
  int width;
  int height;
  union {
#ifdef _WIN32
  struct {
      WND hwnd;
      HDC hdc;
      HGLRC hrc;
  } win;
#else
  struct {
      Display *display;
      Window window;
      GLXContext context;
  } unix_gl;
#endif
  };
} ClosedGL_Window;

void ClosedGL_CreateWindow(ClosedGL_Window *win, int width, int height, const char *title);
void ClosedGL_SwapBuffers(ClosedGL_Window *win);

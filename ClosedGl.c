#include "ClosedGl.h"
#include <stdio.h>
#include <stdlib.h>

void ClosedGL_CreateWindow(ClosedGL_Window *win, int width, int height, const char *title) {
  win->width = width;
  win->height = height;
#ifdef _WIN32
// TODO: add windows implementation;
#else
  win->unix_gl.display = XOpenDisplay(NULL);
  if (!win->unix_gl.display) {
    exit(1);
  }
  GLint visual_attrs[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
  XVisualInfo *vi = glXChooseVisual(win->unix_gl.display, 0, visual_attrs);
  if (!vi) {
    exit(1);
  }
  Colormap cmap = XCreateColormap(win->unix_gl.display, RootWindow(win->unix_gl.display, vi->screen), vi->visual, AllocNone);
  XSetWindowAttributes swa;
  swa.colormap = cmap;
  swa.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;
  win->unix_gl.window = XCreateWindow(win->unix_gl.display, RootWindow(win->unix_gl.display, vi->screen), 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
  XMapWindow(win->unix_gl.display, win->unix_gl.window);
  XStoreName(win->unix_gl.display, win->unix_gl.window, title);
  win->unix_gl.context = glXCreateContext(win->unix_gl.display, vi, NULL, GL_TRUE);
  glXMakeCurrent(win->unix_gl.display, win->unix_gl.window, win->unix_gl.context);
#endif
}

void ClosedGL_SwapBuffers(ClosedGL_Window *win) {
#ifdef _WIN32
  SwapBuffers(win->win.hdc);
#else
  glXSwapBuffers(win->unix_gl.display, win->unix_gl.window);
#endif
}

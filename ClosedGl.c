#include "ClosedGl.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_CLOSE: PostQuitMessage(0); return 0;
    default: return DefWindowProc(hwnd, msg, wparam, lparam);
  }
}

void ClosedGL_CreateWindow(ClosedGL_Window *win, int width, int height, const char *title) {
  win->width = width;
  win->height = height;
  win->win.hinstance = GetModuleHandle(NULL);

  WNDCLASS wc = {0};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = win->win.hinstance;
  wc.lpszClassName = "ClosedGL";
  wc.style = CS_OWNDC;
  RegisterClass(&wc);

  win->win.hwnd = CreateWindow("ClosedGL", title, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, win->win.hinstance, NULL);
  win->win.hdc = GetDC(win->win.hwnd);

  PIXELFORMATDESCRIPTOR pfd = {0};
  pfd.nSize = sizeof(pfd);
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 32;
  pfd.cDepthBits = 24;

  int fmt = ChoosePixelFormat(win->win.hdc, &pfd);
  SetPixelFormat(win->win.hdc, fmt, &pfd);
  win->win.hrc = wglCreateContext(win->win.hdc);
  wglMakeCurrent(win->win.hdc, win->win.hrc);
}

void ClosedGL_SwapBuffers(ClosedGL_Window *win) {
  SwapBuffers(win->win.hdc);
}

int ClosedGL_PollEvent(ClosedGL_Window *win, ClosedGL_Event *event) {
  MSG msg;
  event->type = CLOSEDGL_EVENT_NONE;
  if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT) { event->type = CLOSEDGL_EVENT_QUIT; return 1; }
    if (msg.message == WM_KEYDOWN) { event->type = CLOSEDGL_EVENT_KEYPRESS; event->key = (int)msg.wParam; return 1; }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    return 1;
  }
  return 0;
}

void ClosedGL_DestroyWindow(ClosedGL_Window *win) {
  wglMakeCurrent(NULL, NULL);
  wglDeleteContext(win->win.hrc);
  ReleaseDC(win->win.hwnd, win->win.hdc);
  DestroyWindow(win->win.hwnd);
}

#elif __APPLE__

void ClosedGL_CreateWindow(ClosedGL_Window *win, int width, int height, const char *title) {
  win->width = width;
  win->height = height;
  win->mac.app = [NSApplication sharedApplication];

  NSRect frame = NSMakeRect(0, 0, width, height);
  win->mac.window = [[NSWindow alloc] initWithContentRect:frame styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable backing:NSBackingStoreBuffered defer:NO];
  [win->mac.window setTitle:[NSString stringWithUTF8String:title]];
  [win->mac.window makeKeyAndOrderFront:nil];

  NSOpenGLPixelFormatAttribute attrs[] = {NSOpenGLPFADoubleBuffer, NSOpenGLPFADepthSize, 24, 0};
  NSOpenGLPixelFormat *fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
  win->mac.context = [[NSOpenGLContext alloc] initWithFormat:fmt shareContext:nil];
  [win->mac.context makeCurrentContext];
}

void ClosedGL_SwapBuffers(ClosedGL_Window *win) {
  [win->mac.context flushBuffer];
}

int ClosedGL_PollEvent(ClosedGL_Window *win, ClosedGL_Event *event) {
  event->type = CLOSEDGL_EVENT_NONE;
  NSEvent *e = [win->mac.app nextEventMatchingMask:NSEventMaskAny untilDate:nil inMode:NSDefaultRunLoopMode dequeue:YES];
  if (!e) return 0;
  if (e.type == NSEventTypeKeyDown) { event->type = CLOSEDGL_EVENT_KEYPRESS; event->key = e.keyCode; return 1; }
  if (e.type == NSEventTypeApplicationDefined) { event->type = CLOSEDGL_EVENT_QUIT; return 1; }
  [win->mac.app sendEvent:e];
  return 1;
}

void ClosedGL_DestroyWindow(ClosedGL_Window *win) {
  [win->mac.window close];
}

#else

void ClosedGL_CreateWindow(ClosedGL_Window *win, int width, int height, const char *title) {
  win->width = width;
  win->height = height;
  win->unix_gl.display = XOpenDisplay(NULL);
  if (!win->unix_gl.display) { exit(1); }

  GLint visual_attrs[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
  XVisualInfo *vi = glXChooseVisual(win->unix_gl.display, 0, visual_attrs);
  if (!vi) { exit(1); }

  Colormap cmap = XCreateColormap(win->unix_gl.display, RootWindow(win->unix_gl.display, vi->screen), vi->visual, AllocNone);
  XSetWindowAttributes swa;
  swa.colormap = cmap;
  swa.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;

  win->unix_gl.window = XCreateWindow(win->unix_gl.display, RootWindow(win->unix_gl.display, vi->screen), 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
  XMapWindow(win->unix_gl.display, win->unix_gl.window);
  XStoreName(win->unix_gl.display, win->unix_gl.window, title);
  win->unix_gl.context = glXCreateContext(win->unix_gl.display, vi, NULL, GL_TRUE);
  glXMakeCurrent(win->unix_gl.display, win->unix_gl.window, win->unix_gl.context);
}

void ClosedGL_SwapBuffers(ClosedGL_Window *win) {
  glXSwapBuffers(win->unix_gl.display, win->unix_gl.window);
}

int ClosedGL_PollEvent(ClosedGL_Window *win, ClosedGL_Event *event) {
  event->type = CLOSEDGL_EVENT_NONE;
  if (!XPending(win->unix_gl.display)) return 0;
  XEvent e;
  XNextEvent(win->unix_gl.display, &e);
  if (e.type == KeyPress) { event->type = CLOSEDGL_EVENT_KEYPRESS; event->key = (int)e.xkey.keycode; return 1; }
  if (e.type == Expose) { event->type = CLOSEDGL_EVENT_EXPOSE; return 1; }
  if (e.type == DestroyNotify) { event->type = CLOSEDGL_EVENT_QUIT; return 1; }
  return 1;
}

void ClosedGL_DestroyWindow(ClosedGL_Window *win) {
  glXMakeCurrent(win->unix_gl.display, None, NULL);
  glXDestroyContext(win->unix_gl.display, win->unix_gl.context);
  XDestroyWindow(win->unix_gl.display, win->unix_gl.window);
  XCloseDisplay(win->unix_gl.display);
}

#endif

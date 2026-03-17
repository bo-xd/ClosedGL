#pragma once
#include <stdint.h>

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
        struct { HWND hwnd; HDC hdc; HGLRC hrc; HINSTANCE hinstance; } win;
#elif __APPLE__
        struct { void *window; void *context; void *app; } mac;
#else
        struct { Display *display; Window window; GLXContext context; int screen; } unix_gl;
#endif
    } handle;
} ClosedGL_Window;

typedef enum {
    CLOSEDGL_EVENT_NONE = 0,
    CLOSEDGL_EVENT_QUIT,
    CLOSEDGL_EVENT_KEYDOWN,
    CLOSEDGL_EVENT_KEYUP,
    CLOSEDGL_EVENT_MOUSE_MOVE,
    CLOSEDGL_EVENT_MOUSE_BUTTON_DOWN,
    CLOSEDGL_EVENT_MOUSE_BUTTON_UP,
    CLOSEDGL_EVENT_WINDOW_RESIZE,
    CLOSEDGL_EVENT_EXPOSE
} ClosedGL_EventType;

typedef struct {
    ClosedGL_EventType type;
    union {
        struct { int key; int scancode; int mods; } key;
        struct { int x, y; int button; int mods; } mouse;
        struct { int width, height; } size;
    } data;
    uint64_t timestamp_ms;
} ClosedGL_Event;

void ClosedGL_CreateWindow(ClosedGL_Window *win, int width, int height, const char *title);
void ClosedGL_SwapBuffers(ClosedGL_Window *win);
int  ClosedGL_PollEvent(ClosedGL_Window *win, ClosedGL_Event *event);
void ClosedGL_DestroyWindow(ClosedGL_Window *win);
void ClosedGL_SetTitle(ClosedGL_Window *win, const char *title);
void ClosedGL_SetFullscreen(ClosedGL_Window *win, int enable);
void ClosedGL_SetVSync(ClosedGL_Window *win, int enable);
double ClosedGL_GetTime(void);
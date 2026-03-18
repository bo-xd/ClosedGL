#include "../ClosedGl.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#include <string.h>
#include <time.h>

static int (*glXSwapIntervalSGI)(int) = NULL;
static void (*glXSwapIntervalEXT)(Display*, GLXDrawable, int) = NULL;
static int (*glXSwapIntervalMESA)(unsigned int) = NULL;

static void load_swap_interval_extensions(Display *dpy) {
    if (!dpy) return;
    if (!glXSwapIntervalSGI) {
        void *p = (void*)glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalSGI");
        if (p) glXSwapIntervalSGI = (int (*)(int))p;
    }
    if (!glXSwapIntervalEXT) {
        void *p = (void*)glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalEXT");
        if (p) glXSwapIntervalEXT = (void (*)(Display*, GLXDrawable, int))p;
    }
    if (!glXSwapIntervalMESA) {
        void *p = (void*)glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalMESA");
        if (p) glXSwapIntervalMESA = (int (*)(unsigned int))p;
    }
}

static void send_netwm_fullscreen(Display *dpy, Window win, int enable) {
    Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
    Atom fs_atom = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
    XEvent xev;
    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.xclient.window = win;
    xev.xclient.message_type = wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = enable ? 1 : 0;
    xev.xclient.data.l[1] = (long)fs_atom;
    xev.xclient.data.l[2] = 0;
    xev.xclient.data.l[3] = 0;
    xev.xclient.data.l[4] = 0;
    Window root = DefaultRootWindow(dpy);
    XSendEvent(dpy, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
    XFlush(dpy);
}

static uint64_t monotonic_ms(void) {
#if defined(CLOCK_MONOTONIC)
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000ULL);
    }
#endif
    return (uint64_t)time(NULL) * 1000ULL;
}

int platform_create_window(ClosedGL_Window *win, int width, int height, const char *title) {
    if (!win) return 0;
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) return 0;
    int screen = DefaultScreen(dpy);
    static int visual_attribs[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    XVisualInfo *vi = glXChooseVisual(dpy, screen, visual_attribs);
    if (!vi) { XCloseDisplay(dpy); return 0; }
    Colormap cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);
    XSetWindowAttributes swa;
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask;
    Window root = RootWindow(dpy, vi->screen);
    Window xwin = XCreateWindow(dpy, root, 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
    if (!xwin) { XFree(vi); XCloseDisplay(dpy); return 0; }
    if (title && title[0]) XStoreName(dpy, xwin, title);
    Atom wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, xwin, &wm_delete, 1);
    XMapWindow(dpy, xwin);
    XFlush(dpy);
    GLXContext ctx = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    if (!ctx) { XDestroyWindow(dpy, xwin); XFree(vi); XCloseDisplay(dpy); return 0; }
    if (!glXMakeCurrent(dpy, xwin, ctx)) { glXDestroyContext(dpy, ctx); XDestroyWindow(dpy, xwin); XFree(vi); XCloseDisplay(dpy); return 0; }
    win->width = width;
    win->height = height;
    win->handle.unix_gl.display = dpy;
    win->handle.unix_gl.window = xwin;
    win->handle.unix_gl.context = ctx;
    load_swap_interval_extensions(dpy);
    XFree(vi);
    return 1;
}

void platform_swap_buffers(ClosedGL_Window *win) {
    if (!win) return;
    Display *dpy = win->handle.unix_gl.display;
    Window xwin = win->handle.unix_gl.window;
    if (!dpy || !xwin) return;
    glXSwapBuffers(dpy, xwin);
}

int platform_poll_event(ClosedGL_Window *win, ClosedGL_Event *event) {
    if (!win || !event) return 0;
    Display *dpy = win->handle.unix_gl.display;
    if (!dpy) return 0;
    if (!XPending(dpy)) { event->type = CLOSEDGL_EVENT_NONE; return 0; }
    XEvent xe;
    XNextEvent(dpy, &xe);
    event->timestamp_ms = monotonic_ms();
    switch (xe.type) {
        case Expose:
            event->type = CLOSEDGL_EVENT_EXPOSE;
            return 1;
        case ConfigureNotify:
            event->type = CLOSEDGL_EVENT_WINDOW_RESIZE;
            event->data.size.width = xe.xconfigure.width;
            event->data.size.height = xe.xconfigure.height;
            win->width = xe.xconfigure.width;
            win->height = xe.xconfigure.height;
            return 1;
        case KeyPress: {
            KeySym ks = NoSymbol;
            XLookupString(&xe.xkey, NULL, 0, &ks, NULL);
            event->type = CLOSEDGL_EVENT_KEYDOWN;
            event->data.key.key = (int)ks;
            event->data.key.scancode = 0;
            event->data.key.mods = 0;
            return 1;
        }
        case KeyRelease: {
            KeySym ks = NoSymbol;
            XLookupString(&xe.xkey, NULL, 0, &ks, NULL);
            event->type = CLOSEDGL_EVENT_KEYUP;
            event->data.key.key = (int)ks;
            event->data.key.scancode = 0;
            event->data.key.mods = 0;
            return 1;
        }
        case ButtonPress:
            event->type = CLOSEDGL_EVENT_MOUSE_BUTTON_DOWN;
            event->data.mouse.button = (int)xe.xbutton.button;
            event->data.mouse.x = xe.xbutton.x;
            event->data.mouse.y = xe.xbutton.y;
            event->data.mouse.mods = 0;
            return 1;
        case ButtonRelease:
            event->type = CLOSEDGL_EVENT_MOUSE_BUTTON_UP;
            event->data.mouse.button = (int)xe.xbutton.button;
            event->data.mouse.x = xe.xbutton.x;
            event->data.mouse.y = xe.xbutton.y;
            event->data.mouse.mods = 0;
            return 1;
        case MotionNotify:
            event->type = CLOSEDGL_EVENT_MOUSE_MOVE;
            event->data.mouse.x = xe.xmotion.x;
            event->data.mouse.y = xe.xmotion.y;
            event->data.mouse.button = 0;
            event->data.mouse.mods = 0;
            return 1;
        case ClientMessage: {
            Atom wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
            if ((Atom)xe.xclient.data.l[0] == wm_delete) {
                event->type = CLOSEDGL_EVENT_QUIT;
                return 1;
            }
            event->type = CLOSEDGL_EVENT_NONE;
            return 1;
        }
        case DestroyNotify:
            event->type = CLOSEDGL_EVENT_QUIT;
            return 1;
        default:
            event->type = CLOSEDGL_EVENT_NONE;
            return 1;
    }
}

void platform_destroy_window(ClosedGL_Window *win) {
    if (!win) return;
    Display *dpy = win->handle.unix_gl.display;
    if (!dpy) return;
    GLXContext ctx = win->handle.unix_gl.context;
    Window xwin = win->handle.unix_gl.window;
    if (ctx) {
        glXMakeCurrent(dpy, None, NULL);
        glXDestroyContext(dpy, ctx);
        win->handle.unix_gl.context = NULL;
    }
    if (xwin) {
        XDestroyWindow(dpy, xwin);
        win->handle.unix_gl.window = 0;
    }
    XCloseDisplay(dpy);
    win->handle.unix_gl.display = NULL;
}

void platform_set_title(ClosedGL_Window *win, const char *title) {
    if (!win || !title) return;
    Display *dpy = win->handle.unix_gl.display;
    Window xwin = win->handle.unix_gl.window;
    if (!dpy || !xwin) return;
    XStoreName(dpy, xwin, title);
    XFlush(dpy);
}

void platform_set_fullscreen(ClosedGL_Window *win, int enable) {
    if (!win) return;
    Display *dpy = win->handle.unix_gl.display;
    Window xwin = win->handle.unix_gl.window;
    if (!dpy || !xwin) return;
    send_netwm_fullscreen(dpy, xwin, enable ? 1 : 0);
}

void platform_set_vsync(ClosedGL_Window *win, int enable) {
    if (!win) return;
    Display *dpy = win->handle.unix_gl.display;
    Window xwin = win->handle.unix_gl.window;
    if (!dpy || !xwin) return;
    if (glXSwapIntervalEXT) {
        glXSwapIntervalEXT(dpy, (GLXDrawable)xwin, enable ? 1 : 0);
    } else if (glXSwapIntervalSGI) {
        glXSwapIntervalSGI(enable ? 1 : 0);
    } else if (glXSwapIntervalMESA) {
        glXSwapIntervalMESA(enable ? 1U : 0U);
    }
}

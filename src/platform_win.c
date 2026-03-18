#include "../ClosedGl.h"
#include <windows.h>

#ifndef APIENTRY
#define APIENTRY __stdcall
#endif

typedef BOOL (APIENTRY *PFNWGLSWAPINTERVALEXTPROC)(int interval);

static const char *CLASS_NAME = "ClosedGLPlatformWindowClass";
static HINSTANCE g_instance = NULL;
static PFNWGLSWAPINTERVALEXTPROC pfnWglSwapIntervalEXT = NULL;

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

static void ensure_wgl_extensions(ClosedGL_Window *win);

static LRESULT CALLBACK ClosedGL_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

int platform_create_window(ClosedGL_Window *win, int width, int height, const char *title) {
    if (!win) return 0;
    if (!g_instance) g_instance = GetModuleHandle(NULL);

    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = ClosedGL_WndProc;
    wc.hInstance = g_instance;
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClass(&wc)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            return 0;
        }
    }

    DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    DWORD exStyle = 0;

    RECT wr = { 0, 0, width, height };
    AdjustWindowRectEx(&wr, style, FALSE, exStyle);

    HWND hwnd = CreateWindowEx(
        exStyle,
        CLASS_NAME,
        title ? title : "ClosedGL",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        NULL,
        NULL,
        g_instance,
        NULL
    );

    if (!hwnd) return 0;

    HDC hdc = GetDC(hwnd);
    if (!hdc) {
        DestroyWindow(hwnd);
        return 0;
    }

    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int fmt = ChoosePixelFormat(hdc, &pfd);
    if (fmt == 0) {
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        return 0;
    }
    if (!SetPixelFormat(hdc, fmt, &pfd)) {
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        return 0;
    }

    HGLRC hrc = wglCreateContext(hdc);
    if (!hrc) {
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        return 0;
    }
    if (!wglMakeCurrent(hdc, hrc)) {
        wglDeleteContext(hrc);
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        return 0;
    }

    win->width = width;
    win->height = height;
    win->handle.win.hwnd = hwnd;
    win->handle.win.hdc = hdc;
    win->handle.win.hrc = hrc;
    win->handle.win.hinstance = g_instance;

    ensure_wgl_extensions(win);

    return 1;
}

void platform_swap_buffers(ClosedGL_Window *win) {
    if (!win) return;
    HWND hwnd = win->handle.win.hwnd;
    HDC hdc = win->handle.win.hdc;
    if (hdc && hwnd) {
        SwapBuffers(hdc);
    }
}

int platform_poll_event(ClosedGL_Window *win, ClosedGL_Event *event) {
    if (!win || !event) return 0;

    MSG msg;
    if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        event->type = CLOSEDGL_EVENT_NONE;
        return 0;
    }

    if (msg.message == WM_QUIT) {
        event->type = CLOSEDGL_EVENT_QUIT;
        return 1;
    }

    switch (msg.message) {
        case WM_SIZE: {
            int w = LOWORD(msg.lParam);
            int h = HIWORD(msg.lParam);
            event->type = CLOSEDGL_EVENT_WINDOW_RESIZE;
            event->data.size.width = w;
            event->data.size.height = h;
            win->width = w;
            win->height = h;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            return 1;
        }
        case WM_KEYDOWN: {
            event->type = CLOSEDGL_EVENT_KEYDOWN;
            event->data.key.key = (int)msg.wParam;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            return 1;
        }
        case WM_KEYUP: {
            event->type = CLOSEDGL_EVENT_KEYUP;
            event->data.key.key = (int)msg.wParam;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            return 1;
        }
        case WM_MOUSEMOVE: {
            event->type = CLOSEDGL_EVENT_MOUSE_MOVE;
            event->data.mouse.x = GET_X_LPARAM(msg.lParam);
            event->data.mouse.y = GET_Y_LPARAM(msg.lParam);
            event->data.mouse.button = 0;
            event->data.mouse.mods = 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            return 1;
        }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN: {
            event->type = CLOSEDGL_EVENT_MOUSE_BUTTON_DOWN;
            event->data.mouse.x = GET_X_LPARAM(msg.lParam);
            event->data.mouse.y = GET_Y_LPARAM(msg.lParam);
            if (msg.message == WM_LBUTTONDOWN) event->data.mouse.button = 1;
            else if (msg.message == WM_RBUTTONDOWN) event->data.mouse.button = 2;
            else event->data.mouse.button = 3;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            return 1;
        }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP: {
            event->type = CLOSEDGL_EVENT_MOUSE_BUTTON_UP;
            event->data.mouse.x = GET_X_LPARAM(msg.lParam);
            event->data.mouse.y = GET_Y_LPARAM(msg.lParam);
            if (msg.message == WM_LBUTTONUP) event->data.mouse.button = 1;
            else if (msg.message == WM_RBUTTONUP) event->data.mouse.button = 2;
            else event->data.mouse.button = 3;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            return 1;
        }
        case WM_PAINT: {
            event->type = CLOSEDGL_EVENT_EXPOSE;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            return 1;
        }
        default:
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            event->type = CLOSEDGL_EVENT_NONE;
            return 1;
    }
}

void platform_destroy_window(ClosedGL_Window *win) {
    if (!win) return;
    HWND hwnd = win->handle.win.hwnd;
    HDC hdc = win->handle.win.hdc;
    HGLRC hrc = win->handle.win.hrc;

    if (hrc) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hrc);
        win->handle.win.hrc = NULL;
    }
    if (hdc && hwnd) {
        ReleaseDC(hwnd, hdc);
        win->handle.win.hdc = NULL;
    }
    if (hwnd) {
        DestroyWindow(hwnd);
        win->handle.win.hwnd = NULL;
    }

    if (g_instance) {
        UnregisterClass(CLASS_NAME, g_instance);
    }
}

void platform_set_title(ClosedGL_Window *win, const char *title) {
    if (!win || !title) return;
    HWND hwnd = win->handle.win.hwnd;
    if (hwnd) SetWindowText(hwnd, title);
}

void platform_set_fullscreen(ClosedGL_Window *win, int enable) {
    if (!win) return;
    HWND hwnd = win->handle.win.hwnd;
    if (!hwnd) return;

    if (enable) {
        DWORD style = GetWindowLong(hwnd, GWL_STYLE);
        DWORD exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        MONITORINFO mi = { sizeof(mi) };
        HMONITOR hm = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
        if (GetMonitorInfo(hm, &mi)) {
            SetWindowLong(hwnd, GWL_STYLE, style & ~(WS_OVERLAPPEDWINDOW));
            SetWindowLong(hwnd, GWL_EXSTYLE, exStyle & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
            SetWindowPos(hwnd, HWND_TOP,
                mi.rcMonitor.left,
                mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }
    } else {
        DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        SetWindowLong(hwnd, GWL_STYLE, style);
        SetWindowPos(hwnd, NULL, 0, 0, win->width, win->height, SWP_NOMOVE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
}

void platform_set_vsync(ClosedGL_Window *win, int enable) {
    (void)win;
    if (pfnWglSwapIntervalEXT) {
        pfnWglSwapIntervalEXT(enable ? 1 : 0);
    }
}

static void ensure_wgl_extensions(ClosedGL_Window *win) {
    (void)win;
    if (!pfnWglSwapIntervalEXT) {
        pfnWglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    }
}

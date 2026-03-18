#include "../ClosedGl.h"
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

int platform_create_window(ClosedGL_Window *win, int width, int height, const char *title);
void platform_swap_buffers(ClosedGL_Window *win);
int platform_poll_event(ClosedGL_Window *win, ClosedGL_Event *event);
void platform_destroy_window(ClosedGL_Window *win);
void platform_set_title(ClosedGL_Window *win, const char *title);
void platform_set_fullscreen(ClosedGL_Window *win, int enable);
void platform_set_vsync(ClosedGL_Window *win, int enable);

void ClosedGL_CreateWindow(ClosedGL_Window *win, int width, int height, const char *title) {
    if (!win) return;
    memset(win, 0, sizeof(*win));
    win->width = width;
    win->height = height;
    if (!platform_create_window(win, width, height, title)) {
        platform_destroy_window(win);
        memset(win, 0, sizeof(*win));
    }
}

void ClosedGL_SwapBuffers(ClosedGL_Window *win) {
    if (!win) return;
    platform_swap_buffers(win);
}

int ClosedGL_PollEvent(ClosedGL_Window *win, ClosedGL_Event *event) {
    if (!win || !event) return 0;
    return platform_poll_event(win, event);
}

void ClosedGL_DestroyWindow(ClosedGL_Window *win) {
    if (!win) return;
    platform_destroy_window(win);
    memset(win, 0, sizeof(*win));
}

void ClosedGL_SetTitle(ClosedGL_Window *win, const char *title) {
    if (!win || !title) return;
    platform_set_title(win, title);
}

void ClosedGL_SetFullscreen(ClosedGL_Window *win, int enable) {
    if (!win) return;
    platform_set_fullscreen(win, enable ? 1 : 0);
}

void ClosedGL_SetVSync(ClosedGL_Window *win, int enable) {
    if (!win) return;
    platform_set_vsync(win, enable ? 1 : 0);
}

double ClosedGL_GetTime(void) {
#ifdef _WIN32
    static int initialized = 0;
    static double freq = 0.0;
    static LARGE_INTEGER start = {0};
    LARGE_INTEGER now;
    if (!initialized) {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        freq = (double)f.QuadPart;
        QueryPerformanceCounter(&start);
        initialized = 1;
    }
    QueryPerformanceCounter(&now);
    return (double)(now.QuadPart - start.QuadPart) / freq;
#elif defined(__APPLE__)
    static uint64_t start = 0;
    static mach_timebase_info_data_t tb = {0,0};
    uint64_t now = mach_absolute_time();
    if (start == 0) {
        start = now;
        (void)mach_timebase_info(&tb);
    }
    uint64_t diff = now - start;
    double nsecs = (double)diff * (double)tb.numer / (double)tb.denom;
    return nsecs * 1e-9;
#else
    struct timespec ts;
    static struct timespec start = {0,0};
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return (double)time(NULL);
    }
    if (start.tv_sec == 0 && start.tv_nsec == 0) start = ts;
    double sec = (double)(ts.tv_sec - start.tv_sec) + (double)(ts.tv_nsec - start.tv_nsec) * 1e-9;
    return sec;
#endif
}

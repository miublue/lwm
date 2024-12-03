#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include "lwm.h"
#include "config.h"

// client list can be resized anyhow
#define NUM_CLIENTS 256

static Display *display;
static Window root;
static unsigned int screen_w, screen_h;
static workspace_t workspaces[10] = {0};
static XColor border_normal, border_select;
static int cur_ws = 0;
static const int topgap = BOTTOMBAR? 0 : TOPGAP;

#define CURWS workspaces[cur_ws]
#define WSWIN(wn) CURWS.list[wn]

static void (*events[LASTEvent])(XEvent *ev) = {
    [ConfigureRequest] = configure_request,
    [KeyPress]         = key_press,
    [MapRequest]       = map_request,
    [UnmapNotify]      = unmap_notify,
    [DestroyNotify]    = destroy_notify,
};

void exec(const arg_t arg) {
    if (fork() != 0) return;
    if (display) close(ConnectionNumber(display));
    setsid();
    execvp((char*)arg.com[0], (char**)arg.com);
}

void tile_mode(const arg_t arg) {
    if (CURWS.size) WSWIN(CURWS.cur).is_full = 0;
    CURWS.mode = arg.i;
    tile();
}

void incmaster(const arg_t arg) {
    if (CURWS.masterw < 100 && arg.i < 0) return;
    if (CURWS.masterw > screen_w-100 && arg.i > 0) return;
    if (WSWIN(CURWS.cur).is_full) return;
    CURWS.masterw += arg.i;
    tile();
}

void nmaster(const arg_t arg) {
    CURWS.nmaster += arg.i;
    if (CURWS.nmaster < 0) CURWS.nmaster = 0;
    tile();
}

void win_next(const arg_t arg) {
    if (CURWS.size < 2) return;
    if (WSWIN(CURWS.cur).is_full) return;
    if (++CURWS.cur >= CURWS.size) CURWS.cur = 0;
    win_focus(CURWS.cur);
}

void win_prev(const arg_t arg) {
    if (CURWS.size < 2) return;
    if (WSWIN(CURWS.cur).is_full) return;
    if (--CURWS.cur < 0) CURWS.cur = CURWS.size-1;
    win_focus(CURWS.cur);
}

void win_rotate(const arg_t arg) {
    if (CURWS.size < 2) return;
    if (WSWIN(CURWS.cur).is_full) return;
    int idx = (CURWS.cur + arg.i);
    if (idx < 0) idx = (CURWS.size-1);
    if (idx >= CURWS.size) idx = 0;
    client_t cur = CURWS.list[CURWS.cur];
    CURWS.list[CURWS.cur] = CURWS.list[idx];
    CURWS.list[idx] = cur;
    win_focus(idx);
    tile();
}

void win_kill(const arg_t arg) {
    if (CURWS.size == 0) return;
    XKillClient(display, WSWIN(CURWS.cur).wn);
}

void win_full(const arg_t arg) {
    if (CURWS.size == 0) return;
    if (WSWIN(CURWS.cur).is_full = !WSWIN(CURWS.cur).is_full)
        XMoveResizeWindow(display, WSWIN(CURWS.cur).wn,
            -BORDER_SIZE, -BORDER_SIZE, screen_w, screen_h);
    else tile();
}

void win_to_ws(const arg_t arg) {
    if (arg.i == cur_ws || CURWS.size == 0) return;
    int ws = cur_ws;
    Window wn = WSWIN(CURWS.cur).wn;
    XUnmapWindow(display, wn);
    cur_ws = arg.i;
    win_add(wn);
    cur_ws = ws;
    win_del(CURWS.cur);
}

void ws_go(const arg_t arg) {
    if (arg.i == cur_ws) return;
    for (int i = 0; i < CURWS.size; ++i)
        XUnmapWindow(display, WSWIN(i).wn);
    cur_ws = arg.i;
    for (int i = 0; i < CURWS.size; ++i)
        XMapWindow(display, WSWIN(i).wn);
    if (CURWS.size) win_focus(CURWS.cur);
    tile();
}

static void configure_request(XEvent *ev) {
    XConfigureRequestEvent *cr = &ev->xconfigurerequest;
    XConfigureWindow(display, cr->window, cr->value_mask, &(XWindowChanges) {
        .x = cr->x,
        .y = cr->y,
        .width = cr->width,
        .height = cr->height,
        .sibling = cr->above,
        .stack_mode = cr->detail,
    });
    tile();
}

static void key_press(XEvent *ev) {
    KeySym keysym = XkbKeycodeToKeysym(display, ev->xkey.keycode, 0, 0);
    for (int i = 0; i < LEN(keys); ++i) {
        if (keys[i].keysym == keysym && mod_clean(keys[i].mod) == mod_clean(ev->xkey.state))
            keys[i].fun(keys[i].arg);
    }
}

static void map_request(XEvent *ev) {
    Window wn = ev->xmaprequest.window;
    if (wn == None || client_from_window(wn) != -1) return;
    XSelectInput(display, wn, StructureNotifyMask|EnterWindowMask);
    if (CURWS.size) WSWIN(CURWS.cur).is_full = 0;
    win_add(wn);
    XSetWindowBorderWidth(display, wn, BORDER_SIZE);
    XSetWindowBorder(display, wn, border_normal.pixel);
    XMapWindow(display, wn);
    win_focus(CURWS.size-1);
    tile();
}

static void unmap_notify(XEvent *ev) {
    int w = client_from_window(ev->xunmap.window);
    win_del(w);
}

static void destroy_notify(XEvent *ev) {
    int w = client_from_window(ev->xdestroywindow.window);
    win_del(w);
}

static void grab_input() {
    XUngrabKey(display, AnyKey, AnyModifier, root);
    KeyCode code;

    for (int i = 0; i < LEN(keys); ++i) {
        if ((code = XKeysymToKeycode(display, keys[i].keysym)))
            XGrabKey(display, code, keys[i].mod, root, True, GrabModeAsync, GrabModeAsync);
    }
}

static void win_add(Window w) {
    if (w == None) return;
    if (CURWS.size) WSWIN(CURWS.cur).is_full = 0;
    if (++CURWS.size >= CURWS.alloc)
        CURWS.list = realloc(CURWS.list, (CURWS.alloc += NUM_CLIENTS));
    client_t client = {
        .wn = w,
        .is_full = 0,
    };
    CURWS.list[CURWS.size-1] = client;
}

static void win_del(int w) {
    if (w < 0 || CURWS.size == 0) return;
    CURWS.size--;
    for (int i = w; i < CURWS.size; ++i)
        CURWS.list[i] = CURWS.list[i+1];
    if (CURWS.size == 0) CURWS.cur = 0;
    else if (CURWS.cur >= CURWS.size) CURWS.cur = CURWS.size-1;
    win_focus(CURWS.cur);
    tile();
}

static void win_focus(int w) {
    if (w < 0 || CURWS.size == 0) {
        XSetInputFocus(display, root, RevertToParent, CurrentTime);
        CURWS.cur = 0;
        return;
    }

    XRaiseWindow(display, WSWIN(w).wn);
    XSetInputFocus(display, WSWIN(w).wn, RevertToParent, CurrentTime);
    CURWS.cur = w;

    XSetWindowAttributes attr;
    for (int i = 0; i < CURWS.size; ++i) {
        attr.border_pixel = (i == CURWS.cur)? border_select.pixel : border_normal.pixel;
        XChangeWindowAttributes(display, WSWIN(i).wn, CWBorderPixel, &attr);
    }
}

static void tile() {
    if (CURWS.size && WSWIN(CURWS.cur).is_full) return;
    switch (CURWS.mode) {
    case MODE_MONOCLE: return tile_monocle();
    case MODE_NSTACK: return tile_nstack();
    }
}

static void tile_monocle() {
    const int gapsz = (BORDER_SIZE+GAPSIZE)*2;
    for (int i = 0; i < CURWS.size; ++i) {
        XMoveResizeWindow(display, WSWIN(i).wn, GAPSIZE, topgap+GAPSIZE,
                screen_w-gapsz, screen_h-TOPGAP-gapsz);
    }
}

static void tile_nstack() {
    if (CURWS.size < 2)
        return tile_monocle();

    const int gapsz = (BORDER_SIZE+GAPSIZE)*2;
    int y_space = screen_h-TOPGAP-GAPSIZE;
    int stack_w = screen_w-CURWS.masterw;
    int stack_h, master_h;

    // if there are only master clients or if there is only stack
    if (CURWS.nmaster >= CURWS.size || CURWS.nmaster == 0) {
        stack_h = y_space / CURWS.size;
        for (int i = 0; i < CURWS.size; ++i) {
            XMoveResizeWindow(display, WSWIN(i).wn,
                GAPSIZE, topgap+GAPSIZE + stack_h*i,
                screen_w-gapsz, stack_h-(BORDER_SIZE*2)-GAPSIZE);
        }
        return;
    }

    master_h = y_space / CURWS.nmaster;
    stack_h = y_space / (CURWS.size-CURWS.nmaster);

    for (int i = 0; i < CURWS.nmaster; ++i) {
        XMoveResizeWindow(display, WSWIN(i).wn,
            GAPSIZE, topgap+GAPSIZE + master_h*i,
            CURWS.masterw-(BORDER_SIZE*2)-GAPSIZE, master_h-(BORDER_SIZE*2)-GAPSIZE);
    }
    for (int i = CURWS.nmaster; i < CURWS.size; ++i) {
        XMoveResizeWindow(display, WSWIN(i).wn,
            CURWS.masterw+GAPSIZE, topgap+GAPSIZE + stack_h*(i-CURWS.nmaster),
            stack_w-gapsz, stack_h-(BORDER_SIZE*2)-GAPSIZE);
    }
}

static int client_from_window(Window wn) {
    for (int i = 0; i < CURWS.size; ++i) {
        if (WSWIN(i).wn == wn) return i;
    }
    return -1;
}

int main() {
    if (!(display = XOpenDisplay(0))) return 1;
    signal(SIGCHLD, SIG_IGN);
    XSetErrorHandler(xerror);
    system(INIT_SCRIPT);

    int s = DefaultScreen(display);
    root = RootWindow(display, s);
    screen_w = XDisplayWidth(display, s);
    screen_h = XDisplayHeight(display, s);

    XAllocNamedColor(display, DefaultColormap(display, s),
        BORDER_NORMAL, &border_normal, &border_normal);
    XAllocNamedColor(display, DefaultColormap(display, s),
        BORDER_SELECT, &border_select, &border_select);

    XSelectInput(display, root, SubstructureRedirectMask);
    XDefineCursor(display, root, XCreateFontCursor(display, 68));
    grab_input();

    for (int i = 0; i < 10; ++i) {
        workspaces[i].alloc = NUM_CLIENTS;
        workspaces[i].size = workspaces[i].cur = 0;
        workspaces[i].list = calloc(NUM_CLIENTS, sizeof(client_t));
        workspaces[i].mode = DEFAULT_MODE;
        workspaces[i].masterw = screen_w * MASTERW;
        workspaces[i].nmaster = NMASTER;
    }

    XEvent ev;
    for (;;) {
        XNextEvent(display, &ev);
        if (events[ev.type]) events[ev.type](&ev);
    }

    for (int i = 0; i < 10; ++i)
        free(workspaces[i].list);
    XCloseDisplay(display);
    return 0;
}

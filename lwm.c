#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include "lwm.h"
#include "config.h"

// XXX: i figured i could keep a second array with the indexes of all clients in
//      rendering order, so `tile` would raise them in order based on the array.

// client list can be resized anyhow
#define NUM_CLIENTS 256

static Display *display;
static Window root;
static unsigned int screen_w, screen_h;
static workspace_t workspaces[10] = {0};
static XColor border_normal, border_select;
static XButtonEvent button_event;
static XWindowAttributes hover_attr;
static int cur_ws = 0, focus_on_hover = FOCUS_ON_HOVER;
static const int topgap = BOTTOMBAR? 0 : TOPGAP;

#define CURWS workspaces[cur_ws]
#define WSWIN(wn) CURWS.list[wn]

static void (*events[LASTEvent])(XEvent *ev) = {
    [ConfigureRequest] = configure_request,
    [MapRequest]       = map_request,
    [UnmapNotify]      = unmap_notify,
    [DestroyNotify]    = destroy_notify,
    [MotionNotify]     = motion_notify,
    [EnterNotify]      = enter_notify,
    [KeyPress]         = key_press,
    [ButtonPress]      = button_press,
    [ButtonRelease]    = button_release,
};

void exec(const arg_t arg) {
    if (fork() != 0) return;
    if (display) close(ConnectionNumber(display));
    setsid();
    execvp((char*)arg.com[0], (char**)arg.com);
}

void tile_mode(const arg_t arg) {
    if (CURWS.mode != MODE_FLOAT) CURWS.prev_mode = CURWS.mode;
    CURWS.mode = arg.i;
    if (CURWS.size && WSWIN(CURWS.cur).is_full) return;
    focus_on_hover = 0;
    tile();
    win_focus(CURWS.cur);
    focus_on_hover = FOCUS_ON_HOVER;
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
    if (CURWS.size < 2 || (CURWS.size && WSWIN(CURWS.cur).is_full)) return;
    focus_on_hover = 0;
    win_focus(CURWS.cur+1 >= CURWS.size? 0 : CURWS.cur+1);
    focus_on_hover = FOCUS_ON_HOVER;
}

void win_prev(const arg_t arg) {
    if (CURWS.size < 2 || (CURWS.size && WSWIN(CURWS.cur).is_full)) return;
    focus_on_hover = 0;
    win_focus(CURWS.cur-1 < 0? CURWS.size-1 : CURWS.cur-1);
    focus_on_hover = FOCUS_ON_HOVER;
}

void win_rotate(const arg_t arg) {
    if (CURWS.size < 2 || (CURWS.size && WSWIN(CURWS.cur).is_full)) return;
    focus_on_hover = 0;
    int idx = (CURWS.cur + arg.i);
    if (idx < 0) idx = (CURWS.size-1);
    if (idx >= CURWS.size) idx = 0;
    client_t cur = CURWS.list[CURWS.cur];
    CURWS.list[CURWS.cur] = CURWS.list[idx];
    CURWS.list[idx] = cur;
    win_focus(idx);
    tile();
    focus_on_hover = FOCUS_ON_HOVER;
}

void win_kill(const arg_t arg) {
    if (CURWS.size == 0) return;
    XKillClient(display, WSWIN(CURWS.cur).wn);
}

void win_full(const arg_t arg) {
    if (CURWS.size == 0) return;
    focus_on_hover = 0;
    if ((WSWIN(CURWS.cur).is_full = !WSWIN(CURWS.cur).is_full)) {
        XRaiseWindow(display, WSWIN(CURWS.cur).wn);
        XMoveResizeWindow(display, WSWIN(CURWS.cur).wn,
            -BORDER_SIZE, -BORDER_SIZE, screen_w, screen_h);
    } else {
        if (!WSWIN(CURWS.cur).is_float) XLowerWindow(display, WSWIN(CURWS.cur).wn);
        tile();
    }
    focus_on_hover = FOCUS_ON_HOVER;
}

void win_float(const arg_t arg) {
    if (CURWS.size == 0 || (CURWS.size && WSWIN(CURWS.cur).is_full)) return;
    focus_on_hover = 0;
    if ((WSWIN(CURWS.cur).is_float = !WSWIN(CURWS.cur).is_float))
        XRaiseWindow(display, WSWIN(CURWS.cur).wn);
    else
        XLowerWindow(display, WSWIN(CURWS.cur).wn);
    tile();
    focus_on_hover = FOCUS_ON_HOVER;
}

void win_center(const arg_t arg) {
    if (CURWS.size == 0) return;
    client_t *c = &WSWIN(CURWS.cur);
    c->x = (screen_w/2) - (c->w/2);
    c->y = (screen_h/2) - (c->h/2);
    if (c->is_full || !c->is_float) return;
    tile();
}

void win_to_ws(const arg_t arg) {
    if (arg.i == cur_ws || CURWS.size == 0) return;
    int ws = cur_ws;
    client_t c = WSWIN(CURWS.cur);
    XUnmapWindow(display, c.wn);
    cur_ws = arg.i;
    win_add(c.wn);
    client_t *w = &WSWIN(CURWS.size-1);
    w->is_float = c.is_float;
    w->x = c.x; w->y = c.y; w->w = c.w; w->h = c.h;
    cur_ws = ws;
    win_del(CURWS.cur);
}

void ws_go(const arg_t arg) {
    if (arg.i == cur_ws) return;
    focus_on_hover = 0;
    for (int i = 0; i < CURWS.size; ++i) XUnmapWindow(display, WSWIN(i).wn);
    cur_ws = arg.i;
    for (int i = 0; i < CURWS.size; ++i) XMapWindow(display, WSWIN(i).wn);
    if (CURWS.size) win_focus(CURWS.cur);
    tile();
    focus_on_hover = FOCUS_ON_HOVER;
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
    set_client_size(CURWS.cur);
    win_center((const arg_t){0});
    tile();
}

static void unmap_notify(XEvent *ev) {
    win_del(client_from_window(ev->xdestroywindow.window));
}

static void destroy_notify(XEvent *ev) {
    win_del(client_from_window(ev->xdestroywindow.window));
}

static void motion_notify(XEvent *ev) {
    int c = client_from_window(button_event.subwindow);
    if (c == -1 || (c != -1 && WSWIN(c).is_full)) return;
    WSWIN(c).is_float = 1;
    XRaiseWindow(display, button_event.subwindow);
    const int xdiff = ev->xbutton.x_root - button_event.x_root;
    const int ydiff = ev->xbutton.y_root - button_event.y_root;
    XMoveResizeWindow(display, button_event.subwindow,
        hover_attr.x + (button_event.button==1 ? xdiff : 0),
        hover_attr.y + (button_event.button==1 ? ydiff : 0),
        MAX(50, hover_attr.width  + (button_event.button==3 ? xdiff : 0)),
        MAX(50, hover_attr.height + (button_event.button==3 ? ydiff : 0)));
    set_client_size(c);
    tile();
}

static void enter_notify(XEvent *ev) {
    if (button_event.subwindow != None || !focus_on_hover) return;
    while (XCheckTypedEvent(display, EnterNotify, ev));
    win_focus(client_from_window(ev->xcrossing.window));
}

static void key_press(XEvent *ev) {
    KeySym keysym = XkbKeycodeToKeysym(display, ev->xkey.keycode, 0, 0);
    for (int i = 0; i < LEN(keys); ++i) {
        if (keys[i].keysym == keysym && mod_clean(keys[i].mod) == mod_clean(ev->xkey.state))
            keys[i].fun(keys[i].arg);
    }
}

static void button_press(XEvent *ev) {
    if (ev->xbutton.subwindow == None) return;
    XGetWindowAttributes(display, ev->xbutton.subwindow, &hover_attr);
    button_event = ev->xbutton;
    int c = client_from_window(ev->xbutton.subwindow);
    win_focus(c);
    set_client_size(c);
}

static void button_release(XEvent *ev) {
    button_event.subwindow = None;
}

static void grab_input(void) {
    const unsigned int mask = ButtonPressMask|ButtonReleaseMask|PointerMotionMask;
    KeyCode code;
    XUngrabKey(display, AnyKey, AnyModifier, root);
    XUngrabButton(display, AnyButton, AnyModifier, root);
    for (int i = 0; i < LEN(keys); ++i) {
        if ((code = XKeysymToKeycode(display, keys[i].keysym)))
            XGrabKey(display, code, keys[i].mod, root, 1, GrabModeAsync, GrabModeAsync);
    }
    XGrabButton(display, AnyButton, MOD, root, 1, mask, GrabModeAsync, GrabModeAsync, None, None);
}

static void win_add(Window w) {
    if (w == None) return;
    if (CURWS.size) WSWIN(CURWS.cur).is_full = 0;
    if (++CURWS.size >= CURWS.alloc) CURWS.list = realloc(CURWS.list, (CURWS.alloc += NUM_CLIENTS));
    client_t client = {
        .wn = w,
        .is_full = 0,
        .is_float = (CURWS.mode == MODE_FLOAT),
    };
    CURWS.list[CURWS.size-1] = client;
}

static void win_del(int w) {
    if (w < 0 || CURWS.size == 0) return;
    focus_on_hover = 0;
    CURWS.size--;
    for (int i = w; i < CURWS.size; ++i) CURWS.list[i] = CURWS.list[i+1];
    if (CURWS.size == 0) CURWS.cur = 0;
    else if (CURWS.cur >= CURWS.size) CURWS.cur = CURWS.size-1;
    if (CURWS.prev >= CURWS.size) CURWS.prev = CURWS.cur;
    win_focus(CURWS.cur);
    tile();
    focus_on_hover = FOCUS_ON_HOVER;
}

static void win_focus(int w) {
    if (w < 0 || CURWS.size == 0) {
        XSetInputFocus(display, root, RevertToParent, CurrentTime);
        CURWS.cur = 0;
        return;
    }
    focus_on_hover = 0;
    if (WSWIN(w).is_float) XRaiseWindow(display, WSWIN(w).wn);
    else if (!WSWIN(CURWS.cur).is_float) CURWS.prev = CURWS.cur;
    XSetInputFocus(display, WSWIN(w).wn, RevertToParent, CurrentTime);
    XSetWindowBorder(display, WSWIN(CURWS.cur).wn, border_normal.pixel);
    XSetWindowBorder(display, WSWIN(w).wn, border_select.pixel);
    CURWS.cur = w;
    // XXX: i still don't like this.
    for (int i = 0; i < CURWS.size; ++i) {
        if (!WSWIN(i).is_float && i != CURWS.cur && i != CURWS.prev)
            XLowerWindow(display, WSWIN(i).wn);
    }
    focus_on_hover = FOCUS_ON_HOVER;
}

static void tile(void) {
    if ((CURWS.size == 0) || (CURWS.size && WSWIN(CURWS.cur).is_full)) return;
    for (int i = 0; i < CURWS.size; ++i) {
        if (WSWIN(i).wn == None) {
            win_del(i);
            --i; // continue from previous window
        }
    }
    switch (CURWS.mode == MODE_FLOAT? CURWS.prev_mode : CURWS.mode) {
    case MODE_MONOCLE: tile_monocle(); break;
    case MODE_NSTACK: tile_nstack(); break;
    default: break;
    }
    for (int i = 0; i < CURWS.size; ++i) {
        if (!WSWIN(i).is_float) continue;
        XMoveResizeWindow(display, WSWIN(i).wn, WSWIN(i).x, WSWIN(i).y, WSWIN(i).w, WSWIN(i).h);
    }
    if (WSWIN(CURWS.cur).is_float) XRaiseWindow(display, WSWIN(CURWS.cur).wn);
}

static void tile_monocle(void) {
    const int gapsz = (BORDER_SIZE+GAPSIZE)*2;
    // XXX: gotta find a clever way to raise windows properly in monocle mode
    //      whilst still keeping them underneath floating clients.
    for (int i = 0; i < CURWS.size; ++i) {
        if (WSWIN(i).is_float) continue;
        XMoveResizeWindow(display, WSWIN(i).wn, GAPSIZE, topgap+GAPSIZE,
            screen_w-gapsz, screen_h-TOPGAP-gapsz);
    }
}

static void tile_nstack(void) {
    const int gapdist = (BORDER_SIZE+GAPSIZE)*2;
    const int stackgap = (BORDER_SIZE*2)+GAPSIZE;
    const int y_space = screen_h-TOPGAP-GAPSIZE;
    const int stack_w = screen_w-CURWS.masterw;
    int stack_h, master_h;
    int num_master = 0, num_tiled = 0, first_stack = -1;

    for (int i = 0; i < CURWS.size; ++i) {
        if (WSWIN(i).is_float) continue;
        ++num_tiled;
        if (num_master < CURWS.nmaster) ++num_master;
        else if (first_stack == -1) first_stack = i;
    }

    if (num_tiled < 2) return tile_monocle();

    // if there are only master or stacked clients
    if (num_master >= num_tiled || num_master == 0 || first_stack == -1) {
        stack_h = y_space / num_tiled;
        for (int i = 0, cur = 0; i < CURWS.size && cur < num_tiled; ++i) {
            if (WSWIN(i).is_float) continue;
            XMoveResizeWindow(display, WSWIN(i).wn,
                GAPSIZE, (topgap+GAPSIZE) + (stack_h * cur),
                screen_w - gapdist, stack_h - stackgap);
            ++cur; // cur is only incremented if client is not floating
        }
        return;
    }

    master_h = y_space / num_master;
    stack_h = y_space / (num_tiled - num_master);

    for (int i = 0, cur = 0; i < CURWS.size && cur < num_tiled; ++i) {
        if (WSWIN(i).is_float) continue;
        if (cur < num_master) {
            XMoveResizeWindow(display, WSWIN(i).wn,
                GAPSIZE, (topgap+GAPSIZE) + (master_h * cur),
                CURWS.masterw - stackgap, master_h - stackgap);
        } else {
            XMoveResizeWindow(display, WSWIN(i).wn,
                CURWS.masterw+GAPSIZE, (topgap+GAPSIZE) + (stack_h * (cur-num_master)),
                stack_w - gapdist, stack_h - stackgap);
        }
        ++cur;
    }
}

static int client_from_window(Window wn) {
    for (int i = 0; i < CURWS.size; ++i)
        if (WSWIN(i).wn == wn) return i;
    return -1;
}

static void set_client_size(int w) {
    if (w < 0 || CURWS.size == 0 || w >= CURWS.size) return;
    client_t *c = &WSWIN(w);
    XWindowAttributes attr;
    XGetWindowAttributes(display, c->wn, &attr);
    c->x = attr.x;
    c->y = attr.y;
    c->w = attr.width;
    c->h = attr.height;
}

int main(void) {
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
    grab_input();

    for (int i = 0; i < 10; ++i) {
        workspaces[i].alloc = NUM_CLIENTS;
        workspaces[i].size = workspaces[i].cur = workspaces[i].prev = 0;
        workspaces[i].list = calloc(NUM_CLIENTS, sizeof(client_t));
        workspaces[i].mode = workspaces[i].prev_mode = DEFAULT_MODE;
        workspaces[i].masterw = screen_w * MASTERW;
        workspaces[i].nmaster = NMASTER;
    }

    XEvent ev;
    for (;;) {
        XNextEvent(display, &ev);
        if (events[ev.type]) events[ev.type](&ev);
    }

    for (int i = 0; i < 10; ++i) free(workspaces[i].list);
    XCloseDisplay(display);
    return 0;
}

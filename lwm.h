#ifndef __LWM_H
#define __LWM_H
#include <X11/Xlib.h>

// That's 100 windows per workspace. If you need that many,
// or even more than that, you may have a serious problem.
#define MAX_WINDOWS 100
#define MAX_WORKSPACES 10

#define MIN(a, b) ((a) < (b)? (a) : (b))
#define MAX(a, b) ((a) > (b)? (a) : (b))
#define LEN(x) (sizeof(x) / sizeof((x)[0]))
#define mod_clean(mask) (mask & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

enum { MODE_MONOCLE, MODE_NSTACK, MODE_FLOAT };

struct client {
    Window wn;
    int is_full, is_float;
    int x, y, w, h;
};

struct workspace {
    unsigned char prev, cur, mode, prev_mode, size;
    int masterw, nmaster;
    struct client list[MAX_WINDOWS];
};

union arg {
    const char **com;
    const int i;
};

struct key {
    unsigned int mod;
    KeySym keysym;
    void (*fun)(const union arg arg);
    const union arg arg;
};

void exec(const union arg arg);
void tile_mode(const union arg arg);
void incmaster(const union arg arg);
void nmaster(const union arg arg);
void win_next(const union arg arg);
void win_prev(const union arg arg);
void win_rotate(const union arg arg);
void win_kill(const union arg arg);
void win_full(const union arg arg);
void win_float(const union arg arg);
void win_center(const union arg arg);
void win_to_ws(const union arg arg);
void ws_go(const union arg arg);

static void configure_request(XEvent *ev);
static void map_request(XEvent *ev);
static void unmap_notify(XEvent *ev);
static void destroy_notify(XEvent *ev);
static void motion_notify(XEvent *ev);
static void enter_notify(XEvent *ev);
static void key_press(XEvent *ev);
static void button_press(XEvent *ev);
static void button_release(XEvent *ev);

static void grab_input(void);
static void win_add(Window w);
static void win_del(int w);
static void win_focus(int w);
static void retile(void);
static void tile(void);
static void tile_monocle(void);
static void tile_nstack(void);
static int client_from_window(Window w);
static void set_client_size(int w);

static int xerror() { return 0; }
#endif

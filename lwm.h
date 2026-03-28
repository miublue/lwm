#ifndef __LWM_H
#define __LWM_H
#include <X11/Xlib.h>

// That's 100 windows per workspace. If you need that many,
// or even more than that, you may have a serious problem.
#define MAX_WINDOWS 100

#define MIN(a, b) ((a) < (b)? (a) : (b))
#define MAX(a, b) ((a) > (b)? (a) : (b))
#define LEN(x) (sizeof(x) / sizeof((x)[0]))
#define mod_clean(mask) (mask & \
        (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

enum { MODE_MONOCLE, MODE_NSTACK, MODE_FLOAT };

typedef struct {
    Window wn;
    int is_full, is_float;
    int x, y, w, h;
} client_t;

typedef struct {
    unsigned char prev, cur, mode, prev_mode, size;
    int masterw, nmaster;
    client_t list[MAX_WINDOWS];
} workspace_t;

typedef struct {
    const char **com;
    const int i;
} arg_t;

struct key_t {
    unsigned int mod;
    KeySym keysym;
    void (*fun)(const arg_t arg);
    const arg_t arg;
};

void exec(const arg_t arg);
void tile_mode(const arg_t arg);
void incmaster(const arg_t arg);
void nmaster(const arg_t arg);
void win_next(const arg_t arg);
void win_prev(const arg_t arg);
void win_rotate(const arg_t arg);
void win_kill(const arg_t arg);
void win_full(const arg_t arg);
void win_float(const arg_t arg);
void win_center(const arg_t arg);
void win_to_ws(const arg_t arg);
void ws_go(const arg_t arg);

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

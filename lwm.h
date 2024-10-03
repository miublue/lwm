#ifndef LWM_H
#define LWM_H
#include <X11/Xlib.h>

#define MIN(a, b) ((a) < (b)? (a) : (b))
#define MAX(a, b) ((a) > (b)? (a) : (b))
#define LEN(x) (sizeof(x) / sizeof((x)[0]))
#define mod_clean(mask) (mask & \
        (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

enum {
    MODE_MONOCLE,
    MODE_NSTACK,
};

typedef struct {
    Window wn;
    int is_full;
} client_t;

typedef struct {
    size_t alloc, size;
    client_t *list;
    int cur, mode, master_w, nmaster;
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
void win_to_ws(const arg_t arg);
void ws_go(const arg_t arg);

static void configure_request(XEvent *ev);
static void key_press(XEvent *ev);
static void map_request(XEvent *ev);
static void unmap_notify(XEvent *ev);
static void destroy_notify(XEvent *ev);

static void grab_input();
static void win_add(Window w);
static void win_del(int w);
static void win_focus(int w);
static void tile();
static void tile_monocle();
static void tile_nstack();
static int client_from_window(Window w);

static int xerror() { return 0; }
#endif

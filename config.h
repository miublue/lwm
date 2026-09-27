#ifndef __CONFIG_H
#define __CONFIG_H
#include <X11/Xlib.h>

#define INIT_SCRIPT "$HOME/.lwmrc &"
#define BOTTOM_BAR 0
#define BAR_SIZE 20
#define GAP_SIZE 5
#define MASTER_SCALE 0.5
#define NUM_MASTER 1
#define NEW_WINDOW_FIRST 0
#define DEFAULT_MODE MODE_NSTACK
#define FOCUS_ON_HOVER 1
#define BORDER_SIZE 2
#define BORDER_NORMAL "#000000"
#define BORDER_SELECT "#ffffff"
#define MOD Mod4Mask

const char *term[] = { "lterm", 0 };
const char *menu[] = { "dmenu_run", 0 };

#define DESKTOPCHANGE(key, ws) \
    {MOD, key, ws_switch, {.i = ws}}, \
    {MOD|ShiftMask, key, win_to_ws, {.i = ws}}

static struct key keys[] = {
    {MOD,           XK_Return, exec,         {.com = term}},
    {MOD,           XK_d,      exec,         {.com = menu}},

    {MOD,           XK_Tab,    win_switch,   {.i = +1}},
    {MOD|ShiftMask, XK_Tab,    win_switch,   {.i = -1}},

    {MOD,           XK_j,      win_switch,   {.i = +1}},
    {MOD,           XK_k,      win_switch,   {.i = -1}},
    {MOD|ShiftMask, XK_j,      win_rotate,   {.i = +1}},
    {MOD|ShiftMask, XK_k,      win_rotate,   {.i = -1}},

    {MOD,           XK_l,      master_scale, {.f = +0.05}},
    {MOD,           XK_h,      master_scale, {.f = -0.05}},

    {MOD|ShiftMask, XK_l,      num_master,   {.i = +1}},
    {MOD|ShiftMask, XK_h,      num_master,   {.i = -1}},

    {MOD,           XK_f,      win_full,     {0}},
    {MOD,           XK_space,  win_float,    {0}},
    {MOD,           XK_c,      win_center,   {0}},
    {MOD,           XK_q,      win_kill,     {0}},

    {MOD|ShiftMask, XK_f,      tile_mode,    {.i = MODE_FLOAT}},
    {MOD|ShiftMask, XK_t,      tile_mode,    {.i = MODE_NSTACK}},
    {MOD|ShiftMask, XK_m,      tile_mode,    {.i = MODE_MONOCLE}},

    DESKTOPCHANGE(XK_1, 0), DESKTOPCHANGE(XK_2, 1), DESKTOPCHANGE(XK_3, 2),
    DESKTOPCHANGE(XK_4, 3), DESKTOPCHANGE(XK_5, 4), DESKTOPCHANGE(XK_6, 5),
    DESKTOPCHANGE(XK_7, 6), DESKTOPCHANGE(XK_8, 7), DESKTOPCHANGE(XK_9, 8),
    DESKTOPCHANGE(XK_0, 9),
};

#endif

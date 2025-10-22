#ifndef __CONFIG_H
#define __CONFIG_H
#include <X11/Xlib.h>

#define BOTTOMBAR 0
#define TOPGAP 18
#define GAPSIZE 5
#define MASTERW 0.5
#define NMASTER 1
#define DEFAULT_MODE MODE_NSTACK
#define FOCUS_ON_HOVER 1
#define BORDER_SIZE 2
#define BORDER_NORMAL "#666666"
#define BORDER_SELECT "#ff5599"
#define MOD Mod4Mask

#define INIT_SCRIPT "$HOME/.lwmrc &"

const char *term[] = { "lterm", 0 };
const char *menu[] = { "dmenu_run", 0 };

#define DESKTOPCHANGE(key, ws) \
    {MOD, key, ws_go, {.i = ws}}, \
    {MOD|ShiftMask, key, win_to_ws, {.i = ws}}

static struct key_t keys[] = {
    {MOD,           XK_Return, exec,       {.com = term}},
    {MOD,           XK_d,      exec,       {.com = menu}},

    {MOD,           XK_Tab,    win_next,   {0}},
    {MOD|ShiftMask, XK_Tab,    win_prev,   {0}},

    {MOD,           XK_j,      win_next,   {0}},
    {MOD,           XK_k,      win_prev,   {0}},

    {MOD|ShiftMask, XK_j,      win_rotate, {.i =  1}},
    {MOD|ShiftMask, XK_k,      win_rotate, {.i = -1}},

    {MOD,           XK_l,      incmaster,  {.i =  20}},
    {MOD,           XK_h,      incmaster,  {.i = -20}},

    {MOD|ShiftMask, XK_l,      nmaster,    {.i =  1}},
    {MOD|ShiftMask, XK_h,      nmaster,    {.i = -1}},

    {MOD,           XK_f,      win_full,   {0}},
    {MOD,           XK_q,      win_kill,   {0}},

    {MOD|ShiftMask, XK_t,      tile_mode,  {.i = MODE_NSTACK}},
    {MOD|ShiftMask, XK_m,      tile_mode,  {.i = MODE_MONOCLE}},

    DESKTOPCHANGE(XK_1, 0),
    DESKTOPCHANGE(XK_2, 1),
    DESKTOPCHANGE(XK_3, 2),
    DESKTOPCHANGE(XK_4, 3),
    DESKTOPCHANGE(XK_5, 4),
    DESKTOPCHANGE(XK_6, 5),
    DESKTOPCHANGE(XK_7, 6),
    DESKTOPCHANGE(XK_8, 7),
    DESKTOPCHANGE(XK_9, 8),
    DESKTOPCHANGE(XK_0, 9),
};

#endif

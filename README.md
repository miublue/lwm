<h1 align="center">lwm</h1>
<p align="center">Tiny X11 tiling window manager.</p>
<img src="./img.png"/>

- Programs shown in the screenshot are [lbar](https://github.com/miublue/lbar),
[led](https://github.com/miublue/led), [lfm](https://github.com/miublue/lfm) and
[lterm](https://github.com/miublue/lterm).

## Features

lwm is a minimalistic window manager (~500LoC) with support for:
- master-stack tiling, monocle and floating modes;
- multiple master windows on tiling mode;
- mixed floating/tiling modes;
- fullscreen windows;
- 10 workspaces;
- configurable gaps around windows and border width;
- configurable space for bar on top/bottom of the screen;

## Installation
Compile with:
```sh
make install
```

## Configuration
- Simply edit `config.h` and recompile.

## Known Issues
- XRaiseWindow and XLowerWindow usage is somewhat chaotic.
- There are a few too many loops.
- No support for ICCCM or EWMH.

Pull requests are welcome.


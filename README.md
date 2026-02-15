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
- fullscreen windows;
- 10 workspaces;

## Installation
Compile with:
```sh
make install
```

## Configuration
- Simply edit `config.h` and recompile.

## Known Issues
- XRaiseWindow and XLowerWindow usage is rather chaotic.
- Focusing between tiled and floating windows may raise or lower wrong window.
- No support for ICCCM or EWMH.

Pull requests are welcome.


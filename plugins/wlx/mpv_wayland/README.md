# mpv_wayland Double Commander Plugin

A native WLX (Lister) plugin for Double Commander allowing video playback in the Quick View panel. This plugin uses the `libmpv` Render API and `QOpenGLWidget`, offering seamless, high-performance video embedding, particularly well-suited for Wayland and HiDPI displays.

This plugin evolved from the `mpv_alt` plugin, modernizing video rendering by removing legacy X11 window-ID (`wid`) embedding, making it fully compatible with Wayland compositors.

## Features
- Native Qt6 rendering using `QOpenGLWidget` and the `mpv_render_context`.
- Wayland native rendering and HiDPI pixel ratio awareness.
- Full On-Screen Controller (OSC) support via Qt mouse event mapping.
- Dedicated keyboard handling bypassing the Double Commander global intercepts.
- Synchronized rendering updates utilizing the Qt `QueuedConnection` mechanism.

## Dependencies
- **Qt6**: Core, Gui, Widgets, OpenGLWidgets
- **libmpv**: `mpv` development libraries (`libmpv-dev` / `mpv`)
- **CMake**: `cmake` and `extra-cmake-modules` (KDE CMake modules)
- **Make** or **Ninja**

**Debian/Ubuntu:**
```bash
sudo apt install build-essential cmake extra-cmake-modules qt6-base-dev libmpv-dev
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake extra-cmake-modules qt6-base mpv
```

## Compilation

```bash
mkdir build
cd build
cmake ..
make
```

The resulting build will yield a file named `mpv_wayland.wlx` inside the `build/` directory.

## Installation

1. Open Double Commander.
2. Navigate to **Configuration > Options > Plugins > Plugins WLX**.
3. Click **Add** and select the newly built `mpv_wayland.wlx` file.
4. Ensure it has priority over other viewer plugins for video extensions (e.g., `.mkv`, `.mp4`).

## Keyboard and Controls
Hovering the mouse across the bottom of the video panel triggers the `libmpv` On Screen Controller (OSC).
Alternatively, you may click on the video panel to grab the keyboard context. This enables `libmpv`'s default keybindings (e.g., Space for Play/Pause, arrow keys for seeking).

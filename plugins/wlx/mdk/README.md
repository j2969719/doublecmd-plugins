# MDK Wayland WLX Plugin for Double Commander

A high-performance multimedia viewer plugin (WLX) for Double Commander on Linux, powered by the [MDK SDK](https://github.com/wang-bin/mdk-sdk) and pure Qt6.

This plugin allows you to instantly preview video and audio files directly in Double Commander's Quick View panel. It is specifically designed to work flawlessly natively on Wayland without suffering from the LCL/Qt6 compatibility crashes that plague other media plugins.

## Features

- **Wayland Native**: Renders video correctly on Wayland compositors using `QOpenGLWidget` and hardware acceleration.
- **Out-of-Process Isolation**: Dynamically loads MDK via `dlopen(..., RTLD_LOCAL)` to ensure its internal `libc++` dependencies don't conflict with Double Commander's `libstdc++`.
- **Media Controls**: Includes a built-in control bar with Play/Pause, Infinite Loop (∞ ⟳), an interactive seek slider, and a time duration readout.
- **Hardware Acceleration**: Automatically attempts to use `VAAPI`, `VDPAU`, `CUDA`, `dav1d`, and `FFmpeg` decoders for silky smooth playback with low CPU usage.

## Supported Formats

The plugin automatically detects and plays a wide array of formats, including:
- **Video:** MP4, MKV, AVI, WEBM, FLV, MOV, WMV, MPEG, MPG, M4V, TS, VOB
- **Audio:** MP3, FLAC, WAV, OGG, M4A, AAC, WMA

## Prerequisites

- **Double Commander** (built with Qt6)
- **Qt6 Development Packages** (`qt6-base`, `qt6-multimedia`, `qt6-opengl`)
- **MDK SDK**: You need the MDK SDK headers to build, and `libmdk.so.0` installed in your system library path (or alongside the plugin) to run.

## Build Instructions

1. Ensure the MDK SDK headers are available. The `Makefile` currently expects them to be at `/home/pplupo/mdk-sdk/include`. Adjust this path in the `Makefile` if necessary.
2. Run `make`:
   ```bash
   make clean
   make
   ```
3. Run `make install`:
   ```bash
   make install
   ```
   This will automatically copy `wlx_mdk_wayland.wlx` into your Double Commander WLX plugins directory (`~/.config/doublecmd/plugins/wlx/`).

## Installation / Configuration in Double Commander

1. Open Double Commander.
2. Go to **Configuration > Options > Plugins > Plugins WLX (Lister)**.
3. Click **Add** and select the installed `wlx_mdk_wayland.wlx` file.
4. Ensure it is placed high enough in your plugin list so it takes priority for media files. The plugin exposes its own detect string, so Double Commander will automatically route compatible media files to it.

## Architecture

Unlike previous versions that relied on Pascal/Lazarus bindings (which conflict when Double Commander re-initializes its LCL widgetset), this plugin is written entirely in C++ using Qt6 directly. It builds a `QOpenGLWidget` as a child of the host panel and uses MDK's `MDK_RenderAPI_OpenGL` to render directly onto it, bypassing the unstable `winId()` Wayland surface conflicts.

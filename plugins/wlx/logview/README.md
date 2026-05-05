# logview — High-Performance Log Viewing WLX Plugin for Double Commander

This plugin provides a high-performance log viewing experience within Double Commander by wrapping the [klogg](https://github.com/variar/klogg) log explorer. It is designed to handle multi-gigabyte log files with ease, offering features like memory mapping, SIMD-accelerated text parsing, and asynchronous searching.

## Technical Implementation & Architecture

The plugin follows an **Out-of-Process (IPC) Architecture** to ensure the stability of Double Commander. Instead of loading the complex klogg engine directly into the host's memory space, it acts as a lightweight C-bridge.

### 1. Initialization and Process Spawning (`ListLoad`)
When Double Commander invokes `ListLoad`:
- The plugin stub creates a `QLocalServer` with a unique socket name (`klogg_wlx_socket_PID_RANDOM`).
- It identifies the host's display platform (Wayland vs. X11).
- It spawns `klogg` as a child process using `QProcess`.
- **Environment Variables**:
    - `KLOGG_WLX_SOCKET`: Path to the Unix domain socket for IPC.
    - `DC_WLX_WAYLAND_TOKEN`: (Wayland only) The xdg-foreign handle of the parent pane.
    - `QT_QPA_PLATFORM=xcb`: (X11/XWayland only) Forces klogg to use X11 for compatible window reparenting.

### 2. Window Embedding
Integration of the child window into the Lister pane is handled differently based on the platform:
- **X11 / XWayland**:
    - The stub uses a polling timer to monitor the child process.
    - It uses `xdotool` to find the top-level `WId` associated with the child PID.
    - Once found, it uses `QWindow::fromWinId` and `QWidget::createWindowContainer` to reparent the klogg window into the DC widget hierarchy.
- **Wayland (Native)**:
    - The stub utilizes `KWaylandExtras::exportWindow` to generate a string token for the parent pane's surface.
    - This token is passed to klogg, allowing it to use the `zxdg_importer_v2` protocol to establish a parent-child relationship across process boundaries.

### 3. IPC Communication (JSON-RPC)
The plugin stub and klogg communicate over a Unix domain socket using a lightweight JSON-RPC-like protocol.
- **Commands**:
    - `search`: Forwards `ListSearchText` parameters (pattern, case sensitivity, etc.).
    - `open`: Loads a new file into the existing klogg instance (`ListLoadNext`).
    - `quit`: Gracefully shuts down the klogg instance during `ListCloseWindow`.

## Dependencies

| Dependency | Purpose | Package (Arch/CachyOS) |
| :--- | :--- | :--- |
| **Qt 6** | Core framework and UI container | `qt6-base` |
| **KF6 WindowSystem** | Wayland token handling & window info | `kwindowsystem` |
| **klogg** | The actual log viewing engine | `klogg-bin-git` (AUR) |
| **xdotool** | Reliable X11 window discovery | `xdotool` |
| **pkg-config** | Build system discovery | `pkgconf` |

## Building

To build the plugin shared object:

```bash
cd src
make qt6
```

The resulting `logview_qt6.wlx` will be generated in the root of the `logview` directory.

## Installation in Double Commander

1. Copy `logview_qt6.wlx` to your plugins directory.
2. Open Double Commander.
3. Navigate to **Options → Plugins → WLX Plugins**.
4. Click **Add** and select `logview_qt6.wlx`.
5. Set a **Detect String** to identify log files, for example:
   ```
   EXT="LOG" | EXT="log" | EXT="syslog" | EXT="txt"
   ```

## Development History
- **Initial Branch**: `logview`
- **Design Inspiration**: Based on the architectural analysis for Wayland-native window embedding and high-performance log analysis.
- **Core Strategy**: Decoupled IPC architecture to prevent Double Commander from crashing on OOM during massive file parsing.

## License
GPL-3.0-or-later

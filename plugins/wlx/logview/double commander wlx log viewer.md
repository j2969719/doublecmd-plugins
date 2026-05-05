# **High-Performance Log Viewing in Double Commander: WLX Plugin Architecture and Qt6/Wayland Integration**

## **Introduction to the Double Commander Lister Architecture**

The evolution of twin-panel file managers on UNIX-like operating systems has historically been tethered to the X11 windowing system and GTK2 or Qt5 user interface toolkits. Double Commander represents a cross-platform, open-source file manager heavily inspired by Total Commander, aiming to replicate and extend its paradigm.1 The application natively processes archives as subdirectories, executes background file operations, and supports a rich plugin architecture encompassing WCX (Packer), WDX (Content), WFX (File System), and WLX (Lister) plugins.1

The native internal file viewer, invoked via the F3 hotkey, provides essential mechanisms for viewing files of arbitrary sizes in hexadecimal, binary, or plain text formats, alongside graphics rendering via libraries like librsvg and libturbojpeg.3 However, enterprise software development and system administration necessitate specialized functionalities for advanced log analysis. These requirements include the instantaneous loading of multi-gigabyte files via memory mapping, asynchronous Perl-Compatible Regular Expression (PCRE) searching, dynamic syntax highlighting, and continuous file tailing (the equivalent of tail \-f). Because the native cm\_View command lacks SIMD-accelerated text processing and robust background tailing, extending Double Commander via a custom WLX plugin is imperative.

As modern Linux desktop environments relentlessly transition toward pure Wayland compositors and the Qt6 framework, traditional mechanisms for plugin window embedding have been deprecated. Specifically, the legacy paradigm of X11 reparenting via an XID passed to the plugin is explicitly prohibited under Wayland's strict security and isolation models.4 This report provides an exhaustive, code-level architectural analysis for engineering a high-performance, Wayland-native log viewing WLX plugin for Double Commander. The analysis evaluates the existing open-source Linux WLX ecosystem, dissects the feasibility of cross-compiling legacy Total Commander plugins, and designs a comprehensive engineering roadmap for wrapping the high-performance Klogg application into a native Qt6/Wayland WLX shared library.

## **Phase 3: Engineering a Klogg WLX Plugin Wrapper**

Klogg represents the zenith of open-source, cross-platform log exploration. Forked from the glogg project, Klogg is built on the Qt framework and is engineered explicitly for extreme performance.21 Rather than loading files into the heap, Klogg relies exclusively on memory-mapped files (mmap), allowing it to effortlessly parse datasets exceeding 2.14 billion lines and 10+ GB in size.21 It leverages the simdutf library, utilizing ARM NEON and AVX-512 SIMD instructions to execute UTF-8 validation and text parsing at speeds exceeding one billion characters per second.22 Furthermore, it boasts an asynchronous PCRE search engine that isolates regular expression queries to background threads, ensuring the UI remains perfectly responsive.21

Transforming Klogg from a standalone executable into a dynamic shared library (.so) that strictly conforms to the Double Commander WLX C-API under a Wayland compositor requires a sophisticated architectural synthesis, spanning build systems, inter-process communication, and Wayland subsurface protocols.

### **3.1. Build System Integration and CMake Architecture**

To compile Klogg as a WLX plugin, its native CMake build system must undergo significant restructuring. Klogg is traditionally compiled as a standalone application via the add\_executable directive. This structure must be bifurcated into a static core library encapsulating the business logic and a dynamically loaded shared object library exposing the WLX C-API exports.

The CMake configuration must first define the core logic as an object library or static library: add\_library(klogg\_core STATIC ${KLOGG\_SOURCES}). Following this, a new target must be declared for the WLX wrapper: add\_library(klogg\_wlx SHARED wlx\_wrapper.cpp).

A critical vulnerability when loading Qt-based shared libraries into a host application is symbol collision. By default, Linux ELF shared objects export all symbols. If Klogg and Double Commander use different internal versions of a dependency, the runtime linker will resolve symbols unpredictably, causing immediate segmentation faults. Strict symbol visibility must be enforced via CMake:

CMake

set\_target\_properties(klogg\_wlx PROPERTIES CXX\_VISIBILITY\_PRESET hidden)  
set\_target\_properties(klogg\_wlx PROPERTIES VISIBILITY\_INLINES\_HIDDEN 1)

Only the explicit WLX C-API functions should be tagged with \_\_attribute\_\_((visibility("default"))). The wrapper must be compiled with the \-fPIC (Position Independent Code) flag to ensure it can be dynamically loaded into varying memory address spaces.6 The target must meticulously link against klogg\_core, Qt6::Core, Qt6::Widgets, Qt6::WaylandClient, and the simdutf engine.10

### **3.2. Process and Thread Management Architecture**

Integrating a massive, multi-threaded Qt framework application like Klogg directly into the host process of Double Commander introduces catastrophic stability risks.

If Klogg is compiled strictly in-process—meaning it is loaded via dlopen directly into Double Commander's memory space—it is forced to share the host's event loop. Double Commander initializes its own QApplication instance upon startup. The Qt framework mandates a strict singleton pattern for QApplication. If the Klogg plugin attempts to execute QApplication app(argc, argv); inside the WLX ListLoad initialization vector, the Qt runtime will detect the existing instance and trigger an immediate, fatal abort(). Conversely, if Klogg attempts to hijack Double Commander's existing qApp instance, it risks memory corruption and unpredictable event dispatching, particularly if Double Commander was compiled with a different minor version of Qt6.

To guarantee absolute stability, isolate memory spaces, and protect the file manager from potential crashes during the parsing of malformed regex over a 20 GB binary blob, the plugin must be engineered using an **Out-of-Process (IPC) Architecture**.

Under this paradigm, the klogg\_wlx.so file acts exclusively as a lightweight C-bridge (a stub). When Double Commander invokes ListLoad, the stub utilizes QProcess or POSIX fork()/exec() to spawn a daemonized, headless instance of Klogg, designated as klogg-wlx-server. The stub and the server process establish a high-speed communication channel via UNIX Domain Sockets (e.g., /tmp/klogg\_wlx\_socket\_PID). The stub translates synchronous WLX commands—such as ListSearchText and ListCloseWindow—into structured JSON-RPC or FlatBuffers payloads, transmitting them to the isolated Klogg server.7 This decoupled architecture ensures that if the Klogg server encounters an out-of-bounds memory access, the subprocess terminates gracefully, while Double Commander remains completely stable.

### **3.3. API Mapping: Bridging C to the Klogg IPC**

Double Commander dictates interaction via the Total Commander WLX C-API, documented in the WLX SDK headers (wlx.h).23 The lightweight plugin stub must export these specific extern "C" functions and bridge them to the Klogg server.

| WLX API Export | Architectural Implementation within the Klogg Wrapper |
| :---- | :---- |
| HWND \_stdcall ListLoad(HWND ParentWin, char\* FileToLoad, int ShowFlags) | Acts as the primary initialization vector. The stub intercepts the FileToLoad string and the ParentWin handle. It spawns the Klogg subprocess, passing the file path via IPC. Klogg maps the file to memory, initializes the simdutf encoding validation, and prepares the render buffer. The function must return a unique integer handle (identifying the plugin instance) back to Double Commander. |
| int \_stdcall ListSearchText(HWND ListWin, char\* SearchString, int SearchParameter) | Triggered by the F3 hotkey or internal search commands. The stub translates SearchParameter flags (e.g., case sensitivity, reverse traversal) into a JSON-RPC payload. Klogg receives the payload, executes the asynchronous PCRE search, and commands its internal QTableView to scroll to the matched coordinates. |
| void \_stdcall ListCloseWindow(HWND ListWin) | Triggered when the user dismisses the Lister pane. The stub transmits a termination signal across the UNIX socket. The Klogg server gracefully terminates its event loop, unmaps the log file, and exits, preventing zombie processes. |

### **3.4. Window Embedding under Wayland Compositors**

Resolving the visual integration of an out-of-process Qt6 application into the host's UI constitutes the most profound engineering challenge in this roadmap. The legacy WLX API was conceptualized during the Windows 95 era, relying on passing a parent HWND and expecting the plugin to forcibly draw a child window inside it. On Linux X11, this HWND was cast to an XID, enabling seamless reparenting via QX11EmbedContainer or direct Xlib calls.7

Wayland permanently severs this capability. Applications run in sandboxed contexts. A child window from Process B (Klogg) cannot simply attach itself to Process A (Double Commander) using an integer ID.4

To achieve native, performant embedding under Wayland without relying on the heavily abstracted XWayland compatibility layer, the architecture must implement the xdg-foreign-unstable-v2 protocol.25 This Wayland protocol allows a client to securely export a surface and generate a cryptographic string token, which can be passed to another process to establish a formal parent-child hierarchy.26

#### **The Implementation Sequence**

The integration requires a coordinated handshake between Double Commander and the Klogg subprocess.

First, the **Host Export**. Double Commander must expose the wl\_surface of its dedicated Lister pane. Utilizing Qt6's Wayland extensions or the KDE KWindowSystem library, the host invokes the zxdg\_exporter\_v2 protocol to generate the unique string token.25

C++

// Qt6 / KWindowSystem API utilization by Double Commander  
QString waylandToken \= KWaylandExtras::exportWindow(listerWidget-\>windowHandle());

Secondly, **Handle Translation**. The WLX API rigidly types the ParentWin parameter as an HWND (which compiles to uintptr\_t or unsigned long on Linux). A Wayland token is a string (e.g., wayland:xdg:1234abcd). Because the WLX C-API signature cannot be altered, Double Commander must pass this string token via a dedicated environment variable (e.g., DC\_WLX\_WAYLAND\_TOKEN) prior to executing ListLoad.

Thirdly, the **Client Import**. The out-of-process Klogg server reads the environment variable and retrieves the string token. It utilizes the zxdg\_importer\_v2 protocol to import the host's surface.26 In Qt6, a foreign window handle can be manipulated using QWindow::fromWinId().28 However, because the token is a string, advanced KDE libraries provide QString overloads in KWindowSystem::setMainWindow() to parse the Wayland token and correctly establish the transient parentage.27

Finally, **Embedding the QWidget**. Once the Klogg QWindow establishes the imported Wayland surface as its parent, it can be embedded seamlessly into Klogg's own rendering pipeline using QWidget::createWindowContainer().19 This securely locks Klogg's log-tailing interface directly inside Double Commander's panel, maintaining hardware-accelerated rendering without violating Wayland's security constraints.

If Double Commander fails to provide the xdg-foreign token, the plugin stub must possess a failsafe mechanism. It must forcefully downgrade the Klogg subprocess to XWayland by injecting the environment variable QT\_QPA\_PLATFORM=xcb before the fork(). This forces the Wayland compositor to allocate a legacy XID, allowing standard X11 reparenting mechanics to execute via XReparentWindow 7, ensuring legacy fallback capabilities at the expense of pure Wayland compliance.

## **Synthesis and Strategic Outlook**

The engineering of a high-performance log-viewing plugin for Double Commander on Linux transcends the simplistic cross-compilation of legacy Win32 plugins. The systemic disparities in memory allocation, character encoding paradigms, and the uncompromising shift from global X11 window hierarchies to the secure, isolated Wayland compositor model dictate an entirely modern architectural approach.

Wrapping the Klogg application presents the only viable, production-ready roadmap. By strictly decoupling the plugin into an out-of-process IPC server, the architecture guarantees impregnable stability for the Double Commander host process, immunizing it against memory exhaustion during the analysis of multi-gigabyte files. Resolving the Wayland embedding constraint via the xdg-foreign-unstable-v2 protocol ensures native Qt6 compatibility without relying on deprecated XWayland abstractions. This integration ultimately provides the Linux file management ecosystem with unprecedented forensic capabilities, seamlessly merging the fluid UI of a modern Qt6 application with the immense processing throughput of SIMD-accelerated text engines.

#### **Works cited**

1. Double Commander, accessed April 18, 2026, [https://doublecmd.sourceforge.io/](https://doublecmd.sourceforge.io/)  
2. Double Commander download | SourceForge.net, accessed April 18, 2026, [https://sourceforge.net/projects/doublecmd/](https://sourceforge.net/projects/doublecmd/)  
3. DC \- Built-in file viewer \- Double Commander, accessed April 18, 2026, [https://doublecmd.github.io/doc/en/viewer.html](https://doublecmd.github.io/doc/en/viewer.html)  
4. Wayland and Qt | Qt 6.11, accessed April 18, 2026, [https://doc.qt.io/qt-6/wayland-and-qt.html](https://doc.qt.io/qt-6/wayland-and-qt.html)  
5. Porting Qt applications to Wayland \- Martin's Blog, accessed April 18, 2026, [https://blog.martin-graesslin.com/blog/2015/07/porting-qt-applications-to-wayland/](https://blog.martin-graesslin.com/blog/2015/07/porting-qt-applications-to-wayland/)  
6. j2969719/doublecmd-plugins: Additions for Double Commander (third-party) \- GitHub, accessed April 18, 2026, [https://github.com/j2969719/doublecmd-plugins](https://github.com/j2969719/doublecmd-plugins)  
7. halfhope/doublecmd\_ooitv\_wlx\_viewer\_plugin \- GitHub, accessed April 18, 2026, [https://github.com/halfhope/doublecmd\_ooitv\_wlx\_viewer\_plugin](https://github.com/halfhope/doublecmd_ooitv_wlx_viewer_plugin)  
8. Plugin wlxwebkit \- Double Commander, accessed April 18, 2026, [https://doublecmd.h1n.ru/viewtopic.php?t=8657](https://doublecmd.h1n.ru/viewtopic.php?t=8657)  
9. wlx · GitHub Topics, accessed April 18, 2026, [https://github.com/topics/wlx](https://github.com/topics/wlx)  
10. qt6-base 6.11.0-2 (x86\_64) \- File List \- Arch Linux, accessed April 18, 2026, [https://archlinux.org/packages/extra/x86\_64/qt6-base/files/](https://archlinux.org/packages/extra/x86_64/qt6-base/files/)  
14. LogViewer 1.1.2 \- Total Commander, accessed April 18, 2026, [https://totalcmd.net/plugring/LogViewer.html](https://totalcmd.net/plugring/LogViewer.html)  
15. LogViewer 1.1.2 \- Total Commander, accessed April 18, 2026, [http://totalcmd.net/plugring/logviewer.html](http://totalcmd.net/plugring/logviewer.html)  
21. GitHub \- variar/klogg: Really fast log explorer based on glogg project, accessed April 18, 2026, [https://github.com/variar/klogg](https://github.com/variar/klogg)  
22. simdutf: Text processing at billions of characters per second \- GitHub, accessed April 18, 2026, [https://github.com/simdutf/simdutf](https://github.com/simdutf/simdutf)  
23. Plugins development · doublecmd/doublecmd Wiki \- GitHub, accessed April 18, 2026, [https://github.com/doublecmd/doublecmd/wiki/Plugins-development](https://github.com/doublecmd/doublecmd/wiki/Plugins-development)  
24. ghisler/WLX-SDK: Total Commander Lister Plugin Interface \- GitHub, accessed April 18, 2026, [https://github.com/ghisler/WLX-SDK](https://github.com/ghisler/WLX-SDK)  
25. Little Wayland Things \- Kai Uwe's Blog, accessed April 18, 2026, [https://blog.broulik.de/2024/11/little-wayland-things/](https://blog.broulik.de/2024/11/little-wayland-things/)  
26. XDG foreign protocol | Wayland Explorer, accessed April 18, 2026, [https://wayland.app/protocols/xdg-foreign-unstable-v2](https://wayland.app/protocols/xdg-foreign-unstable-v2)  
27. On the Road to Plasma 6, Vol. 5 \- Kai Uwe's Blog \- Broulik, accessed April 18, 2026, [https://blog.broulik.de/2024/01/on-the-road-to-plasma-6-vol-5/](https://blog.broulik.de/2024/01/on-the-road-to-plasma-6-vol-5/)  
28. QWindow Class | Qt GUI | Qt 6.11.0, accessed April 18, 2026, [https://doc.qt.io/qt-6/qwindow.html](https://doc.qt.io/qt-6/qwindow.html)
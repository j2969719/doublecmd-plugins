# KPart Host Double Commander Plugin

A "Universal KDE Wrapper" WLX (Lister) plugin for Double Commander. This plugin acts as a host for KDE KParts, allowing Double Commander to view any file type supported by a KDE application (like Okular for PDFs or LibreOffice for documents) directly in the Quick View panel.

## Features
- Dynamic MIME-type detection using `QMimeDatabase`.
- Automatic loading of the best available KDE KPart for the file type.
- Native Wayland/Qt6 embedding via in-process KPart hosting.

## Dependencies
- **Qt6**: Core, Gui, Widgets
- **KDE Frameworks 6 (KF6)**: Parts, KIO, CoreAddons
- **CMake**: `cmake` and `extra-cmake-modules`

## Compilation
```bash
mkdir build
cd build
cmake ..
make
```

The build will produce `kpart_host.wlx`.

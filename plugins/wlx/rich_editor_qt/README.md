# rich_editor_qt

A premium, high-performance Double Commander WLX plugin based on Native Qt6 and KDE's `KTextEditor` framework.

## Features
- Full syntax highlighting (powered by KSyntaxHighlighting).
- Native Qt6 embedding on Wayland (no LCL "floating window" or "two hearts" crashes).
- Toolbar with Save, Undo/Redo, Find/Replace, Word Wrap, and Read-Only toggle.
- Comprehensive Status Bar (Position, Syntax, Encoding, Mode).
- Advanced editing features via KTextEditor (block selection, indentation guides, code folding).

## Requirements
- `Qt6` (Core, Gui, Widgets)
- `KF6TextEditor` (KDE Frameworks 6)

## Building
```bash
mkdir build && cd build
cmake ..
make
```

## Installation
Copy `rich_editor_qt.wlx` to `~/.config/doublecmd/plugins/wlx/` and add it via Double Commander's Options -> Plugins -> WLX.

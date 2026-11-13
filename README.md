# Taskbar & Desktop by Monty  (for Winlator / Wine)

Star / support: https://github.com/monty786-dev/Winlator-Wine-Look-changer

Plain Win32 C, built with mingw-w64 (no .NET / WPF / UWP / DWM).

| Program | What it is |
|---|---|
| `MontySetup64.exe` / `MontySetup32.exe` | Installer (both apps embedded). Run once, press **Install / Update**. |
| `MontyShell.exe` | Taskbar, reveal bar, hover previews, Start panel, desktop mode, custom cursor, keyboard signal, **Settings**. |
| `WindowLookChanger.exe` | Title-bar skinner (started/controlled by MontyShell; transparency, themes, 25 icon presets). |

Open settings: run `MontyShell.exe` again (single instance).  Command line: `/bg` start hidden, `/settings`, `/restart`.

## Keyboard assist (important)
A Wine program cannot open Android's keyboard by itself. MontyShell detects text fields and writes
`kbd.show` / `kbd.hide` in its folder, sets named events `MontyKbdShow` / `MontyKbdHide`, and can run the command you enter in
Settings > Keyboard > "Bridge command". Winlator (or a helper script) must react to that signal to show Gboard.

## Build
`make all`  (needs gcc-mingw-w64-x86-64, gcc-mingw-w64-i686, binutils-mingw-w64-*).

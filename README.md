# Window Look Changer 2.1 (Wine / Winlator)

Plain Win32 C, built with mingw-w64 (no .NET / WPF / UWP / DWM).

* `WindowLookChangerSetup64.exe` / `...32.exe` - installer (embeds the app). Run it once, press **Install / Update**.
* Run `WindowLookChanger.exe` again any time to open the manager (single-instance).
* Tabs: **Themes** (gallery + presets), **Lab** (build your own theme, 25 presets per button, .ico/.png icons), **Options**.
* Command line: `/bg` start hidden, `/restore` restore original system colours, `file.wlct` import a theme.
* Setup: `/S` silent install, `/U` silent uninstall.

Build: `make all`.

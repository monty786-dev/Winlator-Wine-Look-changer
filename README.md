# Window Look Changer 2.0 (Wine / Winlator)

Plain Win32 C, built with mingw-w64 (no .NET / WPF / UWP / DWM).

* `WindowLookChangerSetup64.exe` / `...32.exe` - installer (embeds the app). Run it once, press **Install / Update**.
* Run `WindowLookChanger.exe` again any time to open the manager (it is single-instance).
* Command line: `/bg` start hidden, `/restore` restore original system colours, `file.wlct` import a theme.
* Setup: `/S` silent install, `/U` silent uninstall.

Build: `make all` (needs gcc-mingw-w64-x86-64, gcc-mingw-w64-i686, binutils-mingw-w64-*).

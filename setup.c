/*
 * Window Look Changer - Setup (plain Win32 C, mingw-w64).
 * Single-file installer: the application exe is embedded as resource #1 (RCDATA).
 *  Install  : copies the exe to %APPDATA%\WindowLookChanger, writes registry keys
 *             (settings, uninstall entry, optional autostart, .wlct association,
 *             optional Wine focus tweaks), then optionally launches the app.
 *  Uninstall: restores the original system colours/fonts, removes registry keys and files.
 *  Command line: /S silent install, /U silent uninstall.
 */
#define UNICODE
#define _UNICODE
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

#define APP_EXE  L"WindowLookChanger.exe"
#define KEY_APP  L"Software\\WindowLookChanger"
#define KEY_UNIN L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\WindowLookChanger"
#define KEY_RUN  L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
enum { ID_AUTOSTART = 201, ID_ASSOC, ID_TWEAK, ID_LAUNCH, ID_FOCUS, ID_INSTALL, ID_UNINST, ID_CLOSE, ID_LOG };

static wchar_t g_dir[MAX_PATH], g_exe[MAX_PATH], g_log[2048];
static HWND g_hLog, g_hCk[5];
static HFONT g_font;

static void Log(const wchar_t *fmt, ...) {
    wchar_t b[300]; va_list ap; va_start(ap, fmt); _vsnwprintf(b, 299, fmt, ap); va_end(ap); b[299] = 0;
    if (wcslen(g_log) + wcslen(b) + 3 < 2040) { wcscat(g_log, b); wcscat(g_log, L"\r\n"); }
    if (g_hLog) { SetWindowTextW(g_hLog, g_log); SendMessageW(g_hLog, EM_LINESCROLL, 0, 1000); }
}
static void InitDir(void) {
    wchar_t a[MAX_PATH]; DWORD n = GetEnvironmentVariableW(L"APPDATA", a, MAX_PATH);
    if (!n || n >= MAX_PATH) wcscpy(a, L"C:\\users\\Public");
    _snwprintf(g_dir, MAX_PATH - 1, L"%ls\\WindowLookChanger", a); g_dir[MAX_PATH - 1] = 0;
    _snwprintf(g_exe, MAX_PATH - 1, L"%ls\\%ls", g_dir, APP_EXE); g_exe[MAX_PATH - 1] = 0;
}
static BOOL RegStr(HKEY root, const wchar_t *sub, const wchar_t *name, const wchar_t *val) {
    HKEY k; LONG r = RegCreateKeyExW(root, sub, 0, NULL, 0, KEY_WRITE, NULL, &k, NULL);
    if (r != ERROR_SUCCESS) return FALSE;
    r = RegSetValueExW(k, name, 0, REG_SZ, (const BYTE *)val, (DWORD)((wcslen(val) + 1) * sizeof(wchar_t)));
    RegCloseKey(k); return r == ERROR_SUCCESS;
}
static BOOL RegDw(HKEY root, const wchar_t *sub, const wchar_t *name, DWORD v) {
    HKEY k; LONG r = RegCreateKeyExW(root, sub, 0, NULL, 0, KEY_WRITE, NULL, &k, NULL);
    if (r != ERROR_SUCCESS) return FALSE;
    r = RegSetValueExW(k, name, 0, REG_DWORD, (const BYTE *)&v, sizeof v);
    RegCloseKey(k); return r == ERROR_SUCCESS;
}
static void StopApp(void) {
    HWND w = FindWindowW(L"WLCMain", NULL); int i;
    if (!w) return;
    PostMessageW(w, WM_APP + 2, 0, 0);                    /* quit + restore all skinned windows */
    for (i = 0; i < 40 && FindWindowW(L"WLCMain", NULL); i++) Sleep(100);
    Log(L"Stopped running instance.");
}
static BOOL ExtractExe(void) {
    HRSRC r = FindResourceW(NULL, MAKEINTRESOURCEW(1), RT_RCDATA); HGLOBAL g; DWORD sz, wr; HANDLE f; void *p;
    if (!r) return FALSE;
    g = LoadResource(NULL, r); sz = SizeofResource(NULL, r); p = g ? LockResource(g) : NULL;
    if (!p || !sz) return FALSE;
    f = CreateFileW(g_exe, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return FALSE;
    WriteFile(f, p, sz, &wr, NULL); CloseHandle(f);
    return wr == sz;
}
static void RemoveTree(const wchar_t *dir) {
    WIN32_FIND_DATAW fd; wchar_t pat[MAX_PATH], p[MAX_PATH]; HANDLE h;
    _snwprintf(pat, MAX_PATH - 1, L"%ls\\*", dir); pat[MAX_PATH - 1] = 0;
    h = FindFirstFileW(pat, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
            _snwprintf(p, MAX_PATH - 1, L"%ls\\%ls", dir, fd.cFileName); p[MAX_PATH - 1] = 0;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) RemoveTree(p); else { SetFileAttributesW(p, FILE_ATTRIBUTE_NORMAL); DeleteFileW(p); }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    RemoveDirectoryW(dir);
}

static void DoInstall(BOOL autostart, BOOL assoc, BOOL tweak, BOOL launch, BOOL focus) {
    wchar_t t[MAX_PATH * 2], self[MAX_PATH], setup[MAX_PATH];
    Log(L"Installing to %ls", g_dir);
    StopApp();
    CreateDirectoryW(g_dir, NULL);
    _snwprintf(t, MAX_PATH - 1, L"%ls\\themes", g_dir); CreateDirectoryW(t, NULL);
    _snwprintf(t, MAX_PATH - 1, L"%ls\\themes\\icons", g_dir); CreateDirectoryW(t, NULL);
    if (!ExtractExe()) { Log(L"ERROR: could not write %ls (error %lu)", g_exe, GetLastError()); return; }
    Log(L"Copied %ls", APP_EXE);
    GetModuleFileNameW(NULL, self, MAX_PATH);
    _snwprintf(setup, MAX_PATH - 1, L"%ls\\Setup.exe", g_dir); setup[MAX_PATH - 1] = 0;
    CopyFileW(self, setup, FALSE);

    RegStr(HKEY_CURRENT_USER, KEY_APP, L"InstallDir", g_dir);
    RegStr(HKEY_CURRENT_USER, KEY_APP, L"Version", L"2.0");
    RegDw(HKEY_CURRENT_USER, KEY_APP, L"AutoSkin", 1);
    RegDw(HKEY_CURRENT_USER, KEY_APP, L"ApplySystem", 1);
    _snwprintf(t, MAX_PATH * 2 - 1, L"\"%ls\" /U", setup); t[MAX_PATH * 2 - 1] = 0;
    RegStr(HKEY_CURRENT_USER, KEY_UNIN, L"DisplayName", L"Window Look Changer");
    RegStr(HKEY_CURRENT_USER, KEY_UNIN, L"DisplayVersion", L"2.0");
    RegStr(HKEY_CURRENT_USER, KEY_UNIN, L"Publisher", L"Window Look Changer");
    RegStr(HKEY_CURRENT_USER, KEY_UNIN, L"InstallLocation", g_dir);
    RegStr(HKEY_CURRENT_USER, KEY_UNIN, L"DisplayIcon", g_exe);
    RegStr(HKEY_CURRENT_USER, KEY_UNIN, L"UninstallString", t);
    Log(L"Registry: settings + uninstall entry written.");

    if (autostart) {
        _snwprintf(t, MAX_PATH * 2 - 1, L"\"%ls\" /bg", g_exe); t[MAX_PATH * 2 - 1] = 0;
        RegStr(HKEY_CURRENT_USER, KEY_RUN, L"WindowLookChanger", t);
        RegStr(HKEY_LOCAL_MACHINE, KEY_RUN, L"WindowLookChanger", t);   /* wineboot reads both; HKLM may be denied */
        Log(L"Autostart enabled (starts hidden with /bg; run the exe again to open it).");
    } else {
        HKEY k;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, KEY_RUN, 0, KEY_WRITE, &k) == ERROR_SUCCESS) { RegDeleteValueW(k, L"WindowLookChanger"); RegCloseKey(k); }
    }
    if (assoc) {
        _snwprintf(t, MAX_PATH * 2 - 1, L"\"%ls\" \"%%1\"", g_exe); t[MAX_PATH * 2 - 1] = 0;
        RegStr(HKEY_CURRENT_USER, L"Software\\Classes\\.wlct", NULL, L"WindowLookChanger.Theme");
        RegStr(HKEY_CURRENT_USER, L"Software\\Classes\\WindowLookChanger.Theme", NULL, L"Window Look Changer Theme");
        RegStr(HKEY_CURRENT_USER, L"Software\\Classes\\WindowLookChanger.Theme\\DefaultIcon", NULL, g_exe);
        RegStr(HKEY_CURRENT_USER, L"Software\\Classes\\WindowLookChanger.Theme\\shell\\open\\command", NULL, t);
        Log(L".wlct theme files associated.");
    }
    if (tweak) {
        RegDw(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"ForegroundLockTimeout", 0);
        Log(L"Tweak: ForegroundLockTimeout=0 (lets the title bars raise windows reliably).");
    }
    if (focus) {
        RegStr(HKEY_CURRENT_USER, L"Software\\Wine\\X11 Driver", L"UseTakeFocus", L"N");
        Log(L"Tweak: Wine X11 UseTakeFocus=N (experimental focus fix; restart the container).");
    }
    Log(L"Done.");
    if (launch) ShellExecuteW(NULL, L"open", g_exe, NULL, g_dir, SW_SHOWNORMAL);
}

static void DoUninstall(void) {
    STARTUPINFOW si; PROCESS_INFORMATION pi; wchar_t cmd[MAX_PATH + 16]; HKEY k;
    StopApp();
    if (GetFileAttributesW(g_exe) != INVALID_FILE_ATTRIBUTES) {
        _snwprintf(cmd, MAX_PATH + 15, L"\"%ls\" /restore", g_exe); cmd[MAX_PATH + 15] = 0;
        memset(&si, 0, sizeof si); si.cb = sizeof si;
        if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, g_dir, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, 8000); CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
            Log(L"Original system colours and fonts restored.");
        }
    }
    if (RegOpenKeyExW(HKEY_CURRENT_USER, KEY_RUN, 0, KEY_WRITE, &k) == ERROR_SUCCESS) { RegDeleteValueW(k, L"WindowLookChanger"); RegCloseKey(k); }
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, KEY_RUN, 0, KEY_WRITE, &k) == ERROR_SUCCESS) { RegDeleteValueW(k, L"WindowLookChanger"); RegCloseKey(k); }
    RegDeleteTreeW(HKEY_CURRENT_USER, KEY_APP);
    RegDeleteTreeW(HKEY_CURRENT_USER, KEY_UNIN);
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\.wlct");
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\WindowLookChanger.Theme");
    RemoveTree(g_dir);
    Log(L"Removed registry keys and files. Uninstall complete.");
}

static HWND MkCtl(HWND p, const wchar_t *cls, const wchar_t *tx, DWORD st, int x, int y, int w, int h, int id) {
    HWND c = CreateWindowExW(0, cls, tx, WS_CHILD | WS_VISIBLE | st, x, y, w, h, p, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    if (c) SendMessageW(c, WM_SETFONT, (WPARAM)g_font, TRUE);
    return c;
}
static LRESULT CALLBACK SetupProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        static const wchar_t *ck[5] = { L"Start automatically with Wine / Winlator", L"Associate .wlct theme files",
                                        L"Apply recommended tweaks (ForegroundLockTimeout)", L"Launch after install",
                                        L"Experimental: Wine UseTakeFocus=N (focus fix)" };
        int i, y = 78;
        MkCtl(h, L"STATIC", L"Window Look Changer 2.0\r\nSkins every Wine window and themes system colours, menus, scroll bars and fonts.", SS_LEFT, 12, 8, 380, 56, 0);
        for (i = 0; i < 5; i++) { g_hCk[i] = MkCtl(h, L"BUTTON", ck[i], BS_AUTOCHECKBOX, 16, y, 380, 40, ID_AUTOSTART + i); y += 42; }
        SendMessageW(g_hCk[0], BM_SETCHECK, BST_CHECKED, 0); SendMessageW(g_hCk[1], BM_SETCHECK, BST_CHECKED, 0);
        SendMessageW(g_hCk[2], BM_SETCHECK, BST_CHECKED, 0); SendMessageW(g_hCk[3], BM_SETCHECK, BST_CHECKED, 0);
        MkCtl(h, L"BUTTON", L"Install / Update", BS_PUSHBUTTON, 12, y + 4, 190, 48, ID_INSTALL);
        MkCtl(h, L"BUTTON", L"Uninstall", BS_PUSHBUTTON, 210, y + 4, 190, 48, ID_UNINST);
        g_hLog = MkCtl(h, L"EDIT", L"", ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL | WS_BORDER, 12, y + 58, 388, 110, ID_LOG);
        MkCtl(h, L"BUTTON", L"Close", BS_PUSHBUTTON, 12, y + 174, 388, 44, ID_CLOSE);
        return 0; }
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case ID_INSTALL:
            DoInstall(SendMessageW(g_hCk[0], BM_GETCHECK, 0, 0) == BST_CHECKED, SendMessageW(g_hCk[1], BM_GETCHECK, 0, 0) == BST_CHECKED,
                      SendMessageW(g_hCk[2], BM_GETCHECK, 0, 0) == BST_CHECKED, SendMessageW(g_hCk[3], BM_GETCHECK, 0, 0) == BST_CHECKED,
                      SendMessageW(g_hCk[4], BM_GETCHECK, 0, 0) == BST_CHECKED);
            break;
        case ID_UNINST: DoUninstall(); break;
        case ID_CLOSE: DestroyWindow(h); break;
        }
        return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cmd, int show) {
    WNDCLASSEXW wc; MSG msg; HWND h; int argc = 0, i; LPWSTR *argv; BOOL silentI = FALSE, silentU = FALSE;
    (void)hp; (void)cmd;
    InitDir();
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (i = 1; argv && i < argc; i++) { if (!_wcsicmp(argv[i], L"/S")) silentI = TRUE; else if (!_wcsicmp(argv[i], L"/U")) silentU = TRUE; }
    if (argv) LocalFree(argv);
    if (silentU) { DoUninstall(); return 0; }
    if (silentI) { DoInstall(TRUE, TRUE, TRUE, FALSE, FALSE); return 0; }
    g_font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    if (!g_font) g_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    memset(&wc, 0, sizeof wc); wc.cbSize = sizeof wc; wc.hInstance = hi; wc.lpfnWndProc = SetupProc;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1); wc.lpszClassName = L"WLCSetup";
    if (!RegisterClassExW(&wc)) return 1;
    h = CreateWindowExW(0, L"WLCSetup", L"Window Look Changer Setup", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                        CW_USEDEFAULT, CW_USEDEFAULT, 430, 560, NULL, NULL, hi, NULL);
    if (!h) return 1;
    ShowWindow(h, show); UpdateWindow(h);
    while (GetMessageW(&msg, NULL, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return 0;
}

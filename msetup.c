/*
 * Taskbar & Desktop by Monty - Setup (plain Win32 C, mingw-w64).
 * Embeds MontyShell.exe (resource 1) and WindowLookChanger.exe (resource 2).
 *  Install  : copies both exes to %APPDATA%\MontyShell, registry settings + uninstall entry, optional autostart,
 *             optional default-shell replacement (wfm.exe, backed up), optional Wine tweaks (every old value is saved
 *             and restored on uninstall), optional .wlct association.
 *  Uninstall: restores system look, wfm.exe and every tweaked registry value, removes keys and files.
 *  Command line: /S silent install   /U silent uninstall
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

#define KEY_APP  L"Software\\MontyShell"
#define KEY_UNIN L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MontyShell"
#define KEY_RUN  L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
enum { ID_O1 = 201, ID_O2, ID_O3, ID_O4, ID_O5, ID_INSTALL, ID_UNINST, ID_CLOSE, ID_LOG };
#define NOPT 5

static wchar_t g_dir[MAX_PATH], g_log[3000], g_bak[MAX_PATH];
static HWND g_main, g_hLog; static HFONT g_f, g_fb; static int g_opt[NOPT] = { 1, 0, 1, 1, 1 }, g_prog, g_busy;
static const wchar_t *OPTN[NOPT] = { L"Start automatically with the container", L"Use Monty as the startup shell (replaces wfm.exe, reversible)",
    L"Apply Wine stability / storage / power tweaks", L"Associate .wlct theme files", L"Launch after install" };
#define ACC RGB(0,120,215)

static void Pump(void) { MSG m; while (PeekMessageW(&m, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&m); DispatchMessageW(&m); } }
static void Log(const wchar_t *fmt, ...) {
    wchar_t b[300]; va_list ap; va_start(ap, fmt); _vsnwprintf(b, 299, fmt, ap); va_end(ap); b[299] = 0;
    if (wcslen(g_log) + wcslen(b) + 3 < 2990) { wcscat(g_log, b); wcscat(g_log, L"\r\n"); }
    if (g_hLog) { SetWindowTextW(g_hLog, g_log); SendMessageW(g_hLog, EM_LINESCROLL, 0, 1000); }
}
static void Progress(int p) { g_prog = p; if (g_main) { InvalidateRect(g_main, NULL, FALSE); UpdateWindow(g_main); } Pump(); }
static void InitDir(void) {
    wchar_t a[MAX_PATH]; DWORD n = GetEnvironmentVariableW(L"APPDATA", a, MAX_PATH);
    if (!n || n >= MAX_PATH) wcscpy(a, L"C:\\users\\Public");
    _snwprintf(g_dir, MAX_PATH - 1, L"%ls\\MontyShell", a); g_dir[MAX_PATH - 1] = 0;
    _snwprintf(g_bak, MAX_PATH - 1, L"%ls\\uninstall.ini", g_dir); g_bak[MAX_PATH - 1] = 0;
}
static BOOL RegStr(HKEY r, const wchar_t *sub, const wchar_t *name, const wchar_t *val) {
    HKEY k; LONG x = RegCreateKeyExW(r, sub, 0, NULL, 0, KEY_WRITE, NULL, &k, NULL);
    if (x != ERROR_SUCCESS) return FALSE;
    x = RegSetValueExW(k, name, 0, REG_SZ, (const BYTE *)val, (DWORD)((wcslen(val) + 1) * sizeof(wchar_t))); RegCloseKey(k); return x == ERROR_SUCCESS;
}
static BOOL RegDw(HKEY r, const wchar_t *sub, const wchar_t *name, DWORD v) {
    HKEY k; LONG x = RegCreateKeyExW(r, sub, 0, NULL, 0, KEY_WRITE, NULL, &k, NULL);
    if (x != ERROR_SUCCESS) return FALSE;
    x = RegSetValueExW(k, name, 0, REG_DWORD, (const BYTE *)&v, sizeof v); RegCloseKey(k); return x == ERROR_SUCCESS;
}
/* tweak with backup: the previous value (or "none") is stored in uninstall.ini */
static int g_nbak;
static void Tweak(const wchar_t *sub, const wchar_t *name, DWORD type, const wchar_t *sval, DWORD dval) {
    HKEY k; wchar_t key[16], rec[700], old[500]; DWORD ty = 0, sz = sizeof old; LONG x;
    old[0] = 0;
    x = RegOpenKeyExW(HKEY_CURRENT_USER, sub, 0, KEY_READ, &k);
    if (x == ERROR_SUCCESS) { x = RegQueryValueExW(k, name, NULL, &ty, (BYTE *)old, &sz); RegCloseKey(k); }
    if (x == ERROR_SUCCESS) {
        if (ty == REG_DWORD) _snwprintf(rec, 699, L"%ls|%ls|%d|%lu", sub, name, (int)REG_DWORD, (unsigned long)*(DWORD *)old);
        else _snwprintf(rec, 699, L"%ls|%ls|%d|%ls", sub, name, (int)REG_SZ, old);
    } else _snwprintf(rec, 699, L"%ls|%ls|0|", sub, name);
    rec[699] = 0; _snwprintf(key, 15, L"%d", g_nbak++); key[15] = 0;
    WritePrivateProfileStringW(L"bak", key, rec, g_bak);
    if (type == REG_DWORD) RegDw(HKEY_CURRENT_USER, sub, name, dval); else RegStr(HKEY_CURRENT_USER, sub, name, sval);
}
static void RevertTweaks(void) {
    int i; wchar_t key[16], rec[700];
    for (i = 0; i < 200; i++) {
        wchar_t *sub, *name, *ty, *val, *p; HKEY k;
        _snwprintf(key, 15, L"%d", i); key[15] = 0; GetPrivateProfileStringW(L"bak", key, L"", rec, 700, g_bak);
        if (!rec[0]) continue;
        sub = rec; p = wcschr(sub, L'|'); if (!p) continue; *p++ = 0; name = p; p = wcschr(name, L'|'); if (!p) continue; *p++ = 0;
        ty = p; p = wcschr(ty, L'|'); if (!p) continue; *p++ = 0; val = p;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, sub, 0, KEY_WRITE, &k) != ERROR_SUCCESS) continue;
        if (_wtoi(ty) == 0) RegDeleteValueW(k, name);
        else if (_wtoi(ty) == REG_DWORD) { DWORD d = (DWORD)_wtol(val); RegSetValueExW(k, name, 0, REG_DWORD, (const BYTE *)&d, sizeof d); }
        else RegSetValueExW(k, name, 0, REG_SZ, (const BYTE *)val, (DWORD)((wcslen(val) + 1) * sizeof(wchar_t)));
        RegCloseKey(k);
    }
}

static void StopApps(void) {
    HWND w; int i;
    w = FindWindowExW(HWND_MESSAGE, NULL, L"MontyMsg", NULL); if (w) PostMessageW(w, WM_APP + 6, 0, 0);
    w = FindWindowW(L"WLCMain", NULL); if (w) PostMessageW(w, WM_APP + 2, 0, 0);
    for (i = 0; i < 40; i++) { if (!FindWindowExW(HWND_MESSAGE, NULL, L"MontyMsg", NULL) && !FindWindowW(L"WLCMain", NULL)) break; Sleep(100); }
}
static BOOL Extract(int id, const wchar_t *dest) {
    HRSRC r = FindResourceW(NULL, MAKEINTRESOURCEW(id), RT_RCDATA); HGLOBAL h; DWORD sz, wr; HANDLE f; void *p;
    if (!r) return FALSE;
    h = LoadResource(NULL, r); sz = SizeofResource(NULL, r); p = h ? LockResource(h) : NULL; if (!p || !sz) return FALSE;
    f = CreateFileW(dest, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); if (f == INVALID_HANDLE_VALUE) return FALSE;
    WriteFile(f, p, sz, &wr, NULL); CloseHandle(f); return wr == sz;
}
static void RemoveTree(const wchar_t *dir) {
    WIN32_FIND_DATAW fd; wchar_t pat[MAX_PATH], p[MAX_PATH]; HANDLE h;
    _snwprintf(pat, MAX_PATH - 1, L"%ls\\*", dir); pat[MAX_PATH - 1] = 0; h = FindFirstFileW(pat, &fd);
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
/* wfm.exe is the program Winlator starts as the desktop shell; replacing it makes Monty start with the container */
static void WfmPath(int i, wchar_t *out) {
    wchar_t w[MAX_PATH]; static const wchar_t *sub[3] = { L"", L"\\system32", L"\\syswow64" };
    GetWindowsDirectoryW(w, MAX_PATH); _snwprintf(out, MAX_PATH - 1, L"%ls%ls\\wfm.exe", w, sub[i]); out[MAX_PATH - 1] = 0;
}
static void ReplaceWfm(const wchar_t *monty) {
    int i, done = 0; wchar_t p[MAX_PATH], b[MAX_PATH];
    for (i = 0; i < 3; i++) {
        WfmPath(i, p);
        if (GetFileAttributesW(p) == INVALID_FILE_ATTRIBUTES) continue;
        _snwprintf(b, MAX_PATH - 1, L"%ls.monty-bak", p); b[MAX_PATH - 1] = 0;
        if (GetFileAttributesW(b) == INVALID_FILE_ATTRIBUTES) CopyFileW(p, b, TRUE);
        if (CopyFileW(monty, p, FALSE)) { Log(L"Startup shell: replaced %ls (backup kept)", p); done++; } else Log(L"Could not replace %ls (error %lu)", p, GetLastError());
    }
    if (!done) Log(L"wfm.exe not found in this container - autostart entry is used instead.");
}
static void RestoreWfm(void) {
    int i; wchar_t p[MAX_PATH], b[MAX_PATH];
    for (i = 0; i < 3; i++) {
        WfmPath(i, p); _snwprintf(b, MAX_PATH - 1, L"%ls.monty-bak", p); b[MAX_PATH - 1] = 0;
        if (GetFileAttributesW(b) != INVALID_FILE_ATTRIBUTES) { if (CopyFileW(b, p, FALSE)) { DeleteFileW(b); Log(L"Restored original %ls", p); } }
    }
}

static void DoInstall(void) {
    wchar_t t[MAX_PATH * 2], exe[MAX_PATH], wlc[MAX_PATH], self[MAX_PATH], setup[MAX_PATH];
    if (g_busy) return; g_busy = 1; g_log[0] = 0; Log(L"Installing to %ls", g_dir); Progress(5);
    StopApps(); Progress(12);
    CreateDirectoryW(g_dir, NULL);
    _snwprintf(t, MAX_PATH - 1, L"%ls\\themes", g_dir); CreateDirectoryW(t, NULL);
    _snwprintf(t, MAX_PATH - 1, L"%ls\\themes\\icons", g_dir); CreateDirectoryW(t, NULL);
    _snwprintf(exe, MAX_PATH - 1, L"%ls\\MontyShell.exe", g_dir); _snwprintf(wlc, MAX_PATH - 1, L"%ls\\WindowLookChanger.exe", g_dir);
    if (!Extract(1, exe)) { Log(L"ERROR: cannot write %ls (error %lu)", exe, GetLastError()); g_busy = 0; Progress(0); return; }
    Log(L"Copied MontyShell.exe"); Progress(35);
    if (Extract(2, wlc)) Log(L"Copied WindowLookChanger.exe"); Progress(48);
    GetModuleFileNameW(NULL, self, MAX_PATH); _snwprintf(setup, MAX_PATH - 1, L"%ls\\Setup.exe", g_dir); CopyFileW(self, setup, FALSE);
    WritePrivateProfileStringW(L"bak", NULL, NULL, g_bak); g_nbak = 0;

    RegStr(HKEY_CURRENT_USER, KEY_APP, L"InstallDir", g_dir); RegStr(HKEY_CURRENT_USER, KEY_APP, L"Version", L"1.0");
    _snwprintf(t, MAX_PATH * 2 - 1, L"\"%ls\" /U", setup); t[MAX_PATH * 2 - 1] = 0;
    RegStr(HKEY_CURRENT_USER, KEY_UNIN, L"DisplayName", L"Taskbar & Desktop by Monty"); RegStr(HKEY_CURRENT_USER, KEY_UNIN, L"DisplayVersion", L"1.0");
    RegStr(HKEY_CURRENT_USER, KEY_UNIN, L"Publisher", L"monty786-dev"); RegStr(HKEY_CURRENT_USER, KEY_UNIN, L"InstallLocation", g_dir);
    RegStr(HKEY_CURRENT_USER, KEY_UNIN, L"DisplayIcon", exe); RegStr(HKEY_CURRENT_USER, KEY_UNIN, L"UninstallString", t);
    Log(L"Registry: settings + uninstall entry"); Progress(58);

    if (g_opt[0]) {
        _snwprintf(t, MAX_PATH * 2 - 1, L"\"%ls\" /bg", exe); t[MAX_PATH * 2 - 1] = 0;
        RegStr(HKEY_CURRENT_USER, KEY_RUN, L"MontyShell", t); RegStr(HKEY_LOCAL_MACHINE, KEY_RUN, L"MontyShell", t); Log(L"Autostart enabled (/bg)");
    }
    Progress(66);
    if (g_opt[1]) ReplaceWfm(exe);
    Progress(74);
    if (g_opt[2]) {
        Tweak(L"Control Panel\\Desktop", L"ForegroundLockTimeout", REG_DWORD, NULL, 0);
        Tweak(L"Control Panel\\Desktop", L"MenuShowDelay", REG_SZ, L"0", 0);
        Tweak(L"Software\\Wine\\WineDbg", L"ShowCrashDialog", REG_DWORD, NULL, 0);
        Tweak(L"Software\\Wine\\DllOverrides", L"winemenubuilder.exe", REG_SZ, L"", 0);   /* no .desktop / menu files: saves storage */
        Tweak(L"Software\\Wine\\DllOverrides", L"mscoree", REG_SZ, L"", 0);              /* no Mono install prompt */
        Tweak(L"Software\\Wine\\DllOverrides", L"mshtml", REG_SZ, L"", 0);               /* no Gecko install prompt */
        Log(L"Wine tweaks applied (%d values saved for uninstall)", g_nbak);
    }
    Progress(86);
    if (g_opt[3]) {
        _snwprintf(t, MAX_PATH * 2 - 1, L"\"%ls\" \"%%1\"", wlc); t[MAX_PATH * 2 - 1] = 0;
        RegStr(HKEY_CURRENT_USER, L"Software\\Classes\\.wlct", NULL, L"WindowLookChanger.Theme");
        RegStr(HKEY_CURRENT_USER, L"Software\\Classes\\WindowLookChanger.Theme", NULL, L"Window Look Changer Theme");
        RegStr(HKEY_CURRENT_USER, L"Software\\Classes\\WindowLookChanger.Theme\\shell\\open\\command", NULL, t); Log(L".wlct association set");
    }
    Progress(100); Log(L"Done. Open MontyShell.exe any time to change settings.");
    if (g_opt[4]) ShellExecuteW(NULL, L"open", exe, NULL, g_dir, SW_SHOWNORMAL);
    g_busy = 0;
}
static void DoUninstall(void) {
    STARTUPINFOW si; PROCESS_INFORMATION pi; wchar_t cmd[MAX_PATH + 24], wlc[MAX_PATH]; HKEY k;
    if (g_busy) return; g_busy = 1; g_log[0] = 0; Progress(8); StopApps();
    _snwprintf(wlc, MAX_PATH - 1, L"%ls\\WindowLookChanger.exe", g_dir); wlc[MAX_PATH - 1] = 0;
    if (GetFileAttributesW(wlc) != INVALID_FILE_ATTRIBUTES) {
        _snwprintf(cmd, MAX_PATH + 23, L"\"%ls\" /restore", wlc); cmd[MAX_PATH + 23] = 0; memset(&si, 0, sizeof si); si.cb = sizeof si;
        if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, g_dir, &si, &pi)) { WaitForSingleObject(pi.hProcess, 8000); CloseHandle(pi.hProcess); CloseHandle(pi.hThread); Log(L"System colours restored"); }
    }
    Progress(30); RestoreWfm(); Progress(45); RevertTweaks(); Log(L"Wine tweaks reverted"); Progress(65);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, KEY_RUN, 0, KEY_WRITE, &k) == ERROR_SUCCESS) { RegDeleteValueW(k, L"MontyShell"); RegCloseKey(k); }
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, KEY_RUN, 0, KEY_WRITE, &k) == ERROR_SUCCESS) { RegDeleteValueW(k, L"MontyShell"); RegCloseKey(k); }
    RegDeleteTreeW(HKEY_CURRENT_USER, KEY_APP); RegDeleteTreeW(HKEY_CURRENT_USER, KEY_UNIN);
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\.wlct"); RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\WindowLookChanger.Theme");
    Progress(85); RemoveTree(g_dir); Progress(100); Log(L"Uninstall complete."); g_busy = 0;
}

/* ---- themed UI ---- */
static void Fill(HDC dc, int l, int t, int r, int b, COLORREF c) { RECT q; HBRUSH br = CreateSolidBrush(c); q.left = l; q.top = t; q.right = r; q.bottom = b; FillRect(dc, &q, br); DeleteObject(br); }
static void RR(HDC dc, const RECT *r, int rad, COLORREF f) {
    HPEN p = CreatePen(PS_SOLID, 1, f); HBRUSH b = CreateSolidBrush(f); HPEN op = (HPEN)SelectObject(dc, p); HBRUSH ob = (HBRUSH)SelectObject(dc, b);
    RoundRect(dc, r->left, r->top, r->right, r->bottom, rad, rad); SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b); DeleteObject(p);
}
static void DrawBtn(const DRAWITEMSTRUCT *d) {
    int id = (int)d->CtlID; RECT r = d->rcItem, q = r; wchar_t tx[100]; HDC dc = d->hDC; BOOL pr = (d->itemState & ODS_SELECTED) != 0;
    GetWindowTextW(d->hwndItem, tx, 100); Fill(dc, r.left, r.top, r.right, r.bottom, RGB(24,26,32)); InflateRect(&q, -2, -2);
    SelectObject(dc, g_f); SetBkMode(dc, TRANSPARENT);
    if (id >= ID_O1 && id < ID_O1 + NOPT) {
        RECT pill, lab = q; int on = g_opt[id - ID_O1];
        RR(dc, &q, 12, RGB(40,43,52)); lab.left += 12; lab.right -= 64; SetTextColor(dc, RGB(235,235,240));
        { RECT c = lab; DrawTextW(dc, tx, -1, &c, DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT); lab.top += (lab.bottom - lab.top - (c.bottom - c.top)) / 2; DrawTextW(dc, tx, -1, &lab, DT_WORDBREAK | DT_NOPREFIX); }
        pill.right = q.right - 12; pill.left = pill.right - 44; pill.top = (q.top + q.bottom) / 2 - 11; pill.bottom = pill.top + 22; RR(dc, &pill, 22, on ? ACC : RGB(90,94,106));
        { int kx = on ? pill.right - 20 : pill.left + 2; RECT k; k.left = kx; k.top = pill.top + 2; k.right = kx + 18; k.bottom = pill.top + 20; RR(dc, &k, 18, RGB(255,255,255)); }
        return;
    }
    RR(dc, &q, 12, id == ID_INSTALL ? (pr ? RGB(0,95,175) : ACC) : (pr ? RGB(70,74,86) : RGB(52,56,68)));
    SetTextColor(dc, RGB(255,255,255)); DrawTextW(dc, tx, -1, &q, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
}
static HWND Mk(const wchar_t *cls, const wchar_t *tx, DWORD st, DWORD ex, int x, int y, int w, int h, int id) {
    HWND c = CreateWindowExW(ex, cls, tx, WS_CHILD | WS_VISIBLE | st, x, y, w, h, g_main, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    if (c) SendMessageW(c, WM_SETFONT, (WPARAM)g_f, TRUE); return c;
}
static LRESULT CALLBACK SetupProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        int i, y = 108; g_main = h;
        for (i = 0; i < NOPT; i++) { Mk(L"BUTTON", OPTN[i], BS_OWNERDRAW, 0, 12, y, 416, 50, ID_O1 + i); y += 54; }
        Mk(L"BUTTON", L"Install / Update", BS_OWNERDRAW, 0, 12, y + 30, 200, 48, ID_INSTALL); Mk(L"BUTTON", L"Uninstall", BS_OWNERDRAW, 0, 228, y + 30, 200, 48, ID_UNINST);
        g_hLog = Mk(L"EDIT", L"", ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, WS_EX_CLIENTEDGE, 12, y + 88, 416, 92, ID_LOG);
        Mk(L"BUTTON", L"Close", BS_OWNERDRAW, 0, 12, y + 188, 416, 44, ID_CLOSE);
        return 0; }
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps; RECT rc; HDC dc = BeginPaint(h, &ps); int i, y; GetClientRect(h, &rc);
        Fill(dc, 0, 0, rc.right, rc.bottom, RGB(24,26,32));
        for (i = 0; i < 92; i++) { COLORREF a = RGB(0,100 + i / 3, 200 - i / 4), b = RGB(20,20,40); (void)b; Fill(dc, 0, i, rc.right, i + 1, a); }
        SelectObject(dc, g_fb); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(255,255,255));
        { RECT t = { 16, 12, rc.right - 16, 52 }; DrawTextW(dc, L"Taskbar & Desktop by Monty", -1, &t, DT_SINGLELINE | DT_VCENTER); }
        SelectObject(dc, g_f);
        { RECT t = { 16, 52, rc.right - 16, 88 }; DrawTextW(dc, L"Taskbar, Start panel, desktop and window looks for Winlator", -1, &t, DT_WORDBREAK); }
        y = 108 + NOPT * 54 + 6; { RECT bar = { 12, y, rc.right - 12, y + 18 }, fill; RR(dc, &bar, 18, RGB(52,56,68)); fill = bar; fill.right = bar.left + (bar.right - bar.left) * g_prog / 100; if (g_prog > 0) RR(dc, &fill, 18, ACC); }
        EndPaint(h, &ps); return 0; }
    case WM_DRAWITEM: DrawBtn((DRAWITEMSTRUCT *)l); return TRUE;
    case WM_CTLCOLOREDIT: SetTextColor((HDC)w, RGB(220,220,226)); SetBkColor((HDC)w, RGB(36,39,48)); { static HBRUSH b; if (!b) b = CreateSolidBrush(RGB(36,39,48)); return (LRESULT)b; }
    case WM_COMMAND: {
        int id = LOWORD(w);
        if (id >= ID_O1 && id < ID_O1 + NOPT) { g_opt[id - ID_O1] = !g_opt[id - ID_O1]; InvalidateRect((HWND)l, NULL, FALSE); }
        else if (id == ID_INSTALL) DoInstall(); else if (id == ID_UNINST) DoUninstall(); else if (id == ID_CLOSE) DestroyWindow(h);
        return 0; }
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, m, w, l);
}
int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cmd, int show) {
    WNDCLASSEXW wc; MSG msg; HWND h; int argc = 0, i; LPWSTR *argv; BOOL sI = FALSE, sU = FALSE;
    (void)hp; (void)cmd; InitDir();
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (i = 1; argv && i < argc; i++) { if (!_wcsicmp(argv[i], L"/S")) sI = TRUE; else if (!_wcsicmp(argv[i], L"/U")) sU = TRUE; }
    if (argv) LocalFree(argv);
    if (sU) { DoUninstall(); return 0; }
    if (sI) { g_opt[4] = 0; DoInstall(); return 0; }
    g_f = CreateFontW(-16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    g_fb = CreateFontW(-26, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    if (!g_f) g_f = (HFONT)GetStockObject(DEFAULT_GUI_FONT); if (!g_fb) g_fb = g_f;
    memset(&wc, 0, sizeof wc); wc.cbSize = sizeof wc; wc.hInstance = hi; wc.lpfnWndProc = SetupProc; wc.hCursor = LoadCursorW(NULL, IDC_ARROW); wc.lpszClassName = L"MontySetup";
    if (!RegisterClassExW(&wc)) return 1;
    h = CreateWindowExW(0, L"MontySetup", L"Taskbar & Desktop by Monty - Setup", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 446, 700, NULL, NULL, hi, NULL);
    if (!h) return 1;
    ShowWindow(h, show); UpdateWindow(h);
    while (GetMessageW(&msg, NULL, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return 0;
}

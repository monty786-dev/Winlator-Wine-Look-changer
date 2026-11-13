/*
 * Taskbar & Desktop by Monty  -  plain Win32 C for Wine / Winlator (mingw-w64)
 * https://github.com/monty786-dev/Winlator-Wine-Look-changer   (star / support the project)
 *
 * One exe containing: taskbar (+reveal bar, previews), start panel, desktop mode, settings,
 * custom cursor, keyboard-assist signal, and a bridge to WindowLookChanger.exe (title-bar skinner).
 * Single translation unit: the headers below are code, included in this order.
 */
#include "common.h"
#include "theme.h"
#include "img.h"
#include "draw.h"
#include "mcfg.h"
#include "mpal.h"
#include "mtb1.h"
#include "mtb2.h"
#include "mst1.h"
#include "mst2.h"
#include "mdesk.h"
#include "mmisc.h"
#include "mset.h"

static int g_pvTick;
/* when started as wfm.exe (startup-shell replacement) the real install folder comes from the registry */
static void RedirectDirs(void) {
    wchar_t me[MAX_PATH], d[MAX_PATH], *b; HKEY k; DWORD sz = sizeof d, ty = 0;
    GetModuleFileNameW(NULL, me, MAX_PATH); b = wcsrchr(me, L'\\'); b = b ? b + 1 : me;
    if (_wcsicmp(b, L"wfm.exe") != 0) return;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\MontyShell", 0, KEY_READ, &k) == ERROR_SUCCESS) {
        if (RegQueryValueExW(k, L"InstallDir", NULL, &ty, (BYTE *)d, &sz) == ERROR_SUCCESS && ty == REG_SZ) {
            wcscpy(g_dir, d); wcscat(g_dir, L"\\"); wcscpy(g_themeDir, g_dir); wcscat(g_themeDir, L"themes\\"); wcscpy(g_ini, g_dir); wcscat(g_ini, L"wlc.ini");
        }
        RegCloseKey(k);
    }
}
static void TbReflow(void) {
    RECT o = g_tbRect; int ow = o.right - o.left, oh = o.bottom - o.top;
    TbLayout(); g_tbHot = -1;
    if (g_tbShown && !g_tbAnimOn) TbPutRect(&g_tbRect);
    if ((g_tbRect.right - g_tbRect.left) != ow || (g_tbRect.bottom - g_tbRect.top) != oh) { if (g.tbRound) RoundRgn(g_tb, g_tbRect.right - g_tbRect.left, g_tbRect.bottom - g_tbRect.top, ScaleI(10)); }
    InvalidateRect(g_tb, NULL, FALSE);
}
static void QuitShell(void) {
    HWND w;
    RestoreWorkArea();
    if (g.curOn || g.curHide) SystemParametersInfoW(SPI_SETCURSORS, 0, NULL, SPIF_SENDCHANGE);
    w = FindWindowW(L"Shell_TrayWnd", NULL); if (w && g.hideWine) ShowWindow(w, SW_SHOW);
    SaveCfg();
    if (g_msg) DestroyWindow(g_msg);
    PostQuitMessage(0);
}

static LRESULT CALLBACK MsgProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_TIMER:
        switch (w) {
        case MT_RUN: if (RefreshRun()) { TbReflow(); } else if (g_tb) InvalidateRect(g_tb, NULL, FALSE); break;
        case MT_CLOCK: if (g_tb) InvalidateRect(g_tb, NULL, FALSE); if (g_dk && g.dkOn && g.dkClock) InvalidateRect(g_dk, NULL, FALSE); break;
        case MT_LOGIC: TbLogic(); SetTimer(h, MT_LOGIC, g_wantHide ? 60 : 250, NULL); break;
        case MT_KBD: KbdTick(); break;
        case MT_HIDEW: WineHide(); if (g_tbShown && !g_tbAnimOn && g_tb) SetWindowPos(g_tb, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                       if (g_dk && g.dkOn) SetWindowPos(g_dk, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); break;
        case MT_ANIM: TbAnimStep(); break;
        case MT_PV: KillTimer(h, MT_PV); if (g_tbHot >= 0 && g_it[g_tbHot].type == IT_RUN && g.tbPreview) { PvShow(g_tbHot); SetTimer(h, MT_PVREF, 250, NULL); g_pvTick = 0; } break;
        case MT_PVREF: {
            POINT pt; RECT r; int sub, i; GetCursorPos(&pt);
            if (g_pvItem < 0) { KillTimer(h, MT_PVREF); break; }
            GetWindowRect(g_pv, &r); if (PtInRect(&r, pt)) break;
            GetWindowRect(g_tb, &r);
            if (PtInRect(&r, pt)) {
                i = TbHit(pt.x - r.left, pt.y - r.top, &sub);
                if (i >= 0 && g_it[i].type == IT_RUN) { if (i != g_pvItem) { PvShow(i); g_pvTick = 0; } else if (++g_pvTick >= 6) { PvCapture(g_pvTarget); InvalidateRect(g_pv, NULL, FALSE); g_pvTick = 0; } break; }
            }
            PvHide(); KillTimer(h, MT_PVREF); break; }
        }
        return 0;
    case WM_APP + 5: OpenSettings(); return 0;
    case WM_APP + 6: QuitShell(); return 0;          /* sent by the installer */
    }
    return DefWindowProcW(h, m, w, l);
}

static HWND MkWin(const wchar_t *cls, DWORD ex, DWORD st, int w, int h) {
    return CreateWindowExW(ex, cls, L"", st, 0, 0, w, h, NULL, NULL, g_inst, NULL);
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cmd, int show) {
    int argc = 0, i, bg = 0, openSet = 0, restart = 0, sw, sh; LPWSTR *argv; WNDCLASSEXW wc; MSG msg; HANDLE mtx; int ti;
    (void)hp; (void)cmd; (void)show; g_inst = hi;
    InitPaths(); RedirectDirs(); wcscpy(g_mini, g_dir); wcscat(g_mini, L"monty.ini");
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (i = 1; argv && i < argc; i++) {
        if (!_wcsicmp(argv[i], L"/bg")) bg = 1; else if (!_wcsicmp(argv[i], L"/settings")) openSet = 1; else if (!_wcsicmp(argv[i], L"/restart")) restart = 1;
    }
    if (argv) LocalFree(argv);
    if (restart) Sleep(900);
    mtx = CreateMutexW(NULL, FALSE, L"MontyShell_single");
    if (mtx && GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND o = FindWindowExW(HWND_MESSAGE, NULL, L"MontyMsg", NULL); if (o && !bg) PostMessageW(o, WM_APP + 5, 0, 0);
        return 0;
    }
    InitThemes(); LoadCustomThemes(); LoadCfg();
    g_fUi = MakeFont(L"Segoe UI", 15, 0); g_fSmall = MakeFont(L"Segoe UI", 12, 0);
    if (!g_fUi) g_fUi = (HFONT)GetStockObject(DEFAULT_GUI_FONT); if (!g_fSmall) g_fSmall = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    ti = FindTheme(S_theme); g_themeIdx = ti >= 0 ? ti : 0; g_hasCol = 0; RebuildEff(); BuildPal(); MakeFonts(); RecalcHide();
    if (bg && g.startDelay > 0) Sleep((DWORD)g.startDelay * 1000);

    memset(&wc, 0, sizeof wc); wc.cbSize = sizeof wc; wc.hInstance = hi; wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.lpfnWndProc = MsgProc; wc.lpszClassName = L"MontyMsg"; RegisterClassExW(&wc);
    wc.lpfnWndProc = TbProc; wc.lpszClassName = L"MontyTb"; RegisterClassExW(&wc);
    wc.lpfnWndProc = PvProc; wc.lpszClassName = L"MontyPv"; RegisterClassExW(&wc);
    wc.lpfnWndProc = RvProc; wc.lpszClassName = L"MontyRv"; RegisterClassExW(&wc);
    wc.lpfnWndProc = FlProc; wc.lpszClassName = L"MontyFl"; RegisterClassExW(&wc);
    wc.lpfnWndProc = StProc; wc.lpszClassName = L"MontySt"; RegisterClassExW(&wc);
    wc.lpfnWndProc = InProc; wc.lpszClassName = L"MontyIn"; wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1); RegisterClassExW(&wc); wc.hbrBackground = NULL;
    wc.lpfnWndProc = DkProc; wc.lpszClassName = L"MontyDk"; wc.style = CS_DBLCLKS; RegisterClassExW(&wc);
    wc.lpfnWndProc = SetProc; wc.lpszClassName = L"MontySet"; wc.style = 0; wc.hIcon = LoadIconW(NULL, IDI_APPLICATION); RegisterClassExW(&wc);

    sw = GetSystemMetrics(SM_CXSCREEN); sh = GetSystemMetrics(SM_CYSCREEN);
    g_msg = CreateWindowExW(0, L"MontyMsg", L"", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hi, NULL);
    g_tb = MkWin(L"MontyTb", WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_LAYERED, WS_POPUP, 100, 40);
    g_pv = MkWin(L"MontyPv", WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE, WS_POPUP, 100, 100);
    g_rv = MkWin(L"MontyRv", WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_LAYERED, WS_POPUP, 100, 10);
    g_fl = MkWin(L"MontyFl", WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE, WS_POPUP, 44, 44);
    g_st = MkWin(L"MontySt", WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED, WS_POPUP, 380, 520);
    g_dk = MkWin(L"MontyDk", WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, WS_POPUP, sw, sh);
    g_set = CreateWindowExW(0, L"MontySet", L"Taskbar & Desktop by Monty - Settings", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, ScaleI(480), ScaleI(660), NULL, NULL, hi, NULL);
    if (!g_tb || !g_st || !g_set || !g_msg) { MessageBoxW(NULL, L"Could not create the shell windows.", APP_TITLE, MB_ICONERROR); return 1; }
    g_stEdit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 10, 10, g_st, (HMENU)(INT_PTR)1, hi, NULL);
    SendMessageW(g_stEdit, WM_SETFONT, (WPARAM)g_fTb, TRUE);
    SetLayeredWindowAttributes(g_tb, 0, 255, LWA_ALPHA); SetLayeredWindowAttributes(g_st, 0, 255, LWA_ALPHA);
    if (g_rv) SetLayeredWindowAttributes(g_rv, 0, 255, LWA_ALPHA);

    RefreshRun(); TbRelayout(); ShowWindow(g_tb, SW_SHOWNOACTIVATE); g_tbShown = 1;
    WineHide(); UpdateWorkArea();
    if (g.curOn || g.curHide) ApplyCursor();
    if (g.wlOn) SkinBridge();
    if (g.dkOn) DesktopToggle();
    SetTimer(g_msg, MT_RUN, 400, NULL); SetTimer(g_msg, MT_CLOCK, 10000, NULL); SetTimer(g_msg, MT_LOGIC, 250, NULL);
    SetTimer(g_msg, MT_KBD, 250, NULL); SetTimer(g_msg, MT_HIDEW, 2000, NULL);
    if (openSet || !bg) { if (!bg || openSet) OpenSettings(); }

    while (GetMessageW(&msg, NULL, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return (int)msg.wParam;
}

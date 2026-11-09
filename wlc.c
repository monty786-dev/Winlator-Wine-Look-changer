/*
 * Window Look Changer v2 - plain Win32 C for Wine / Winlator (mingw-w64).
 * Skins every window with a themed title bar and restyles Windows system colours,
 * fonts and metrics (menu bar, toolbar, status bar, scroll bars, workspace, borders)
 * of classic Win32 controls. No .NET / WPF / UWP / DWM.
 * Single translation unit: the headers below are code, included in order.
 */
#include "common.h"
#include "theme.h"
#include "img.h"
#include "draw.h"
#include "skin.h"
#include "app.h"
#include "ui1.h"
#include "ui2.h"

static int g_winX = CW_USEDEFAULT, g_winY = CW_USEDEFAULT, g_winW = 460, g_winH = 600;

static void LoadConfig(void) {
    wchar_t nm[48]; int i;
    g_hasCol = RdI(L"hascolor", 0); g_col = (COLORREF)RdI(L"color", (int)RGB(0,120,215));
    g_dark = RdI(L"dark", 1); g_mgrTop = RdI(L"top", 0);
    g_iconOver = RdI(L"icons", -1); if (g_iconOver < -1 || g_iconOver > 4) g_iconOver = -1;
    g_auto = RdI(L"auto", RegDw(L"AutoSkin", 1)); g_sysApply = RdI(L"sys", RegDw(L"ApplySystem", 1)); g_bg = RdI(L"bg", 0);
    GetPrivateProfileStringW(L"cfg", L"exclude", L";explorer.exe;", g_exclude, 500, g_ini);
    if (g_exclude[0] != L';') wcscpy(g_exclude, L";explorer.exe;");
    g_winW = RdI(L"w", 460); g_winH = RdI(L"h", 600); g_winX = RdI(L"x", CW_USEDEFAULT); g_winY = RdI(L"y", CW_USEDEFAULT);
    if (g_winW < 360) g_winW = 460; if (g_winH < 520) g_winH = 600;
    GetPrivateProfileStringW(L"cfg", L"theme", L"Windows 10", nm, 48, g_ini);
    i = FindTheme(nm); g_themeIdx = i >= 0 ? i : 0;
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cmd, int show) {
    int argc = 0, i, sw, sh; LPWSTR *argv; BOOL bg = FALSE, restore = FALSE; wchar_t imp[MAX_PATH] = L"";
    WNDCLASSEXW wc; MSG msg; HWND h; HANDLE mtx;
    (void)hp; (void)cmd; g_inst = hi;
    InitPaths();
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (i = 1; argv && i < argc; i++) {
        size_t n = wcslen(argv[i]);
        if (!_wcsicmp(argv[i], L"/restore")) restore = TRUE;
        else if (!_wcsicmp(argv[i], L"/bg") || !_wcsicmp(argv[i], L"/min")) bg = TRUE;
        else if (n > 5 && !_wcsicmp(argv[i] + n - 5, L".wlct")) wcsncpy(imp, argv[i], MAX_PATH - 1);
    }
    if (argv) LocalFree(argv);
    if (restore) { RestoreSystemLook(); return 0; }

    mtx = CreateMutexW(NULL, FALSE, L"WindowLookChanger_v2_single");
    if (mtx && GetLastError() == ERROR_ALREADY_EXISTS) {          /* already running: just bring it up */
        HWND o = FindWindowW(L"WLCMain", NULL); if (o && !bg) PostMessageW(o, WM_SHOWAPP, 0, 0);
        return 0;
    }
    InitThemes(); LoadCustomThemes(); LoadConfig(); RebuildEff();
    g_fUi = MakeFont(L"Segoe UI", 15, 0); g_fSmall = MakeFont(L"Segoe UI", 12, 0);
    if (!g_fUi) g_fUi = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (!g_fSmall) g_fSmall = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    g_ovFont = MakeTitleFont(CurTheme());

    memset(&wc, 0, sizeof wc); wc.cbSize = sizeof wc; wc.hInstance = hi; wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.lpfnWndProc = MainProc; wc.lpszClassName = L"WLCMain"; wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    if (!RegisterClassExW(&wc)) { MessageBoxW(NULL, L"RegisterClass failed", APP_TITLE, MB_ICONERROR); return 1; }
    wc.lpfnWndProc = SkinProc; wc.lpszClassName = L"WLCSkin"; wc.style = CS_DBLCLKS; wc.hIcon = NULL;
    if (!RegisterClassExW(&wc)) { MessageBoxW(NULL, L"RegisterClass failed", APP_TITLE, MB_ICONERROR); return 1; }
    wc.lpfnWndProc = TipProc; wc.lpszClassName = L"WLCTip"; wc.style = 0;
    RegisterClassExW(&wc);
    g_tip = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE, L"WLCTip", L"", WS_POPUP | WS_BORDER, 0, 0, 10, 10, NULL, NULL, hi, NULL);

    sw = GetSystemMetrics(SM_CXSCREEN); sh = GetSystemMetrics(SM_CYSCREEN);
    if (g_winW > sw) g_winW = sw; if (g_winH > sh) g_winH = sh;
    if (g_winX != CW_USEDEFAULT && (g_winX < -50 || g_winX > sw - 80)) g_winX = CW_USEDEFAULT;
    if (g_winY != CW_USEDEFAULT && (g_winY < -10 || g_winY > sh - 80)) g_winY = CW_USEDEFAULT;
    h = CreateWindowExW(0, L"WLCMain", APP_TITLE, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, g_winX, g_winY, g_winW, g_winH, NULL, NULL, hi, NULL);
    if (!h) { MessageBoxW(NULL, L"Could not create window", APP_TITLE, MB_ICONERROR); return 1; }
    if (bg) g_bg = 1;
    if (g_mgrTop) SetMgrTop();
    if (g_sysApply) ApplySystemLook(CurTheme());
    if (imp[0]) ImportThemeFile(imp);
    ShowWindow(h, bg ? SW_HIDE : show); if (!bg) UpdateWindow(h);

    while (GetMessageW(&msg, NULL, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return (int)msg.wParam;
}

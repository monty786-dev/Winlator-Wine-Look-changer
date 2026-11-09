/* common.h - shared definitions (single translation unit, included by wlc.c) */
#define UNICODE
#define _UNICODE
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <stddef.h>

#define APP_TITLE L"Window Look Changer"
#define APP_VER   L"2.0"
#define MAXSKIN   96
#define MAXWIN    200
#define MAXTHEME  64
#define MAXIMG    48
#define WM_SHOWAPP (WM_APP + 1)
#define WM_QUITAPP (WM_APP + 2)

enum { TM_REFRESH = 1, TM_SYNC, TM_SCAN, TM_TIP };
enum { BT_CLOSE, BT_MAX, BT_MIN, BT_SNAPR, BT_SNAPL, BT_PIN, BT_SHADE, BT_COUNT };
#define ICN_RESTORE BT_COUNT
#define NICON (BT_COUNT + 1)
enum { ACT_SKIN = 1, ACT_UNSKIN, ACT_SHADE, ACT_TOP, ACT_CLOSE, ACT_LEFT, ACT_RIGHT,
       ACT_MIN, ACT_MAX, ACT_KILL, ACT_NEVER, ACT_SKINALL, ACT_UNSKINALL };
enum { IDC_TAB0 = 101, IDC_TAB1, IDC_TAB2, IDC_TAB3, IDC_SEARCH, IDC_REFRESH, IDC_LIST,
       IDC_RUN, IDC_BROWSE, IDC_GO, IDC_GAL, IDC_ISTYLE, IDC_COLOR, IDC_RESET,
       IDC_LABNAME, IDC_LABFROM, IDC_PROPS, IDC_LSAVE, IDC_LUPD, IDC_LDEL, IDC_LREV,
       IDC_O_AUTO, IDC_O_SYS, IDC_O_BG, IDC_O_TOP, IDC_O_DARK,
       IDC_O_RESTORE, IDC_O_UNSKIN, IDC_O_CLEAR, IDC_O_REAPPLY, IDC_STATUS, IDC_ACT = 300 };
#define CM_BASE 500

#define W_ RGB(255,255,255)
#define K_ RGB(0,0,0)

/* ---- Theme: pure data. Built-ins are in theme.h, custom ones are .wlct files ---- */
#define THEME_FIELDS(X) \
 X(shape) X(gstyle) X(height) X(radius) X(bw) X(bh) X(bt) X(closeW) X(gap) X(rmargin) \
 X(left) X(center) X(bold) X(fsize) X(dbl) X(shadow) \
 X(tTop) X(tBot) X(tTopI) X(tBotI) X(txtA) X(txtI) X(border) X(borderI) \
 X(bFace) X(bHover) X(bPress) X(bBorder) X(glyph) X(closeFace) X(closeHover) X(closeGlyph) X(pin) \
 X(sWindow) X(sFace) X(sText) X(sHilite) X(sHiliteText) X(sShadow) X(sLight) X(sScroll) X(sWork) \
 X(scrollW) X(menuH) X(borderW)

typedef struct {
    wchar_t name[48], file[64], font[32];
    int custom;
    int shape;      /* 0 flat, 1 rounded, 2 circle, 3 glass, 4 classic 3D bevel */
    int gstyle;     /* 0 thin, 1 bold, 2 colour dots, 3 boxed, 4 filled          */
    int height, radius, bw, bh, bt, closeW, gap, rmargin;
    int left, center, bold, fsize, dbl, shadow;
    COLORREF tTop, tBot, tTopI, tBotI, txtA, txtI, border, borderI;
    COLORREF bFace, bHover, bPress, bBorder, glyph, closeFace, closeHover, closeGlyph, pin;
    COLORREF sWindow, sFace, sText, sHilite, sHiliteText, sShadow, sLight, sScroll, sWork;
    int scrollW, menuH, borderW;
    wchar_t icon[NICON][MAX_PATH];
} Theme;

/* ---- globals ---- */
static HINSTANCE g_inst;
static HWND g_main, g_status;
static HFONT g_fUi, g_fSmall, g_ovFont;
static int  g_dark = 1, g_mgrTop = 0, g_auto = 1, g_sysApply = 1, g_bg = 0;
static int  g_themeIdx = 0, g_iconOver = -1, g_hasCol = 0;
static COLORREF g_col = RGB(0,120,215);
static Theme g_themes[MAXTHEME];
static int   g_nthemes, g_nbuiltin;
static Theme g_lab;          /* theme being edited in the Lab */
static int   g_labIdx;
static Theme g_eff;          /* effective theme (accent / icon overrides applied) */
static wchar_t g_dir[MAX_PATH], g_themeDir[MAX_PATH], g_ini[MAX_PATH];
static wchar_t g_exclude[512] = L";explorer.exe;";

static void SetStatus(const wchar_t *fmt, ...);
static void ApplyTheme(int idx);
static void ApplyLookAll(void);
static void Refresh(BOOL force);
static const Theme *CurTheme(void) { return &g_eff; }

/* ---- helpers ---- */
static COLORREF Blend(COLORREF a, COLORREF b, int p) {
    return RGB((GetRValue(a)*(100-p)+GetRValue(b)*p)/100,
               (GetGValue(a)*(100-p)+GetGValue(b)*p)/100,
               (GetBValue(a)*(100-p)+GetBValue(b)*p)/100);
}
static int Lum(COLORREF c) { return (GetRValue(c)*30 + GetGValue(c)*59 + GetBValue(c)*11) / 100; }
static void FillRectC(HDC dc, int l, int t, int r, int b, COLORREF c) {
    RECT rc; HBRUSH br; rc.left = l; rc.top = t; rc.right = r; rc.bottom = b;
    br = CreateSolidBrush(c);
    if (br) { FillRect(dc, &rc, br); DeleteObject(br); }
}
static void GetWork(RECT *wa) {
    if (!SystemParametersInfoW(SPI_GETWORKAREA, 0, wa, 0))
        SetRect(wa, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
}
static void SetStatus(const wchar_t *fmt, ...) {
    wchar_t buf[300]; va_list ap;
    va_start(ap, fmt); _vsnwprintf(buf, 299, fmt, ap); va_end(ap); buf[299] = 0;
    if (g_status) SetWindowTextW(g_status, buf);
}
static HFONT MakeFont(const wchar_t *face, int px, int bold) {
    return CreateFontW(-px, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0,
                       CLEARTYPE_QUALITY, 0, face);
}
static HFONT MakeTitleFont(const Theme *t) {
    HFONT f = MakeFont(t->font[0] ? t->font : L"Segoe UI", MulDiv(t->fsize, 96, 72), t->bold);
    return f ? f : (HFONT)GetStockObject(DEFAULT_GUI_FONT);
}

/* ---- paths + INI (settings live next to the exe) ---- */
static void InitPaths(void) {
    wchar_t *sl, t[MAX_PATH];
    if (!GetModuleFileNameW(NULL, g_dir, MAX_PATH)) wcscpy(g_dir, L".\\");
    sl = wcsrchr(g_dir, L'\\'); if (sl) sl[1] = 0; else wcscpy(g_dir, L".\\");
    wcscpy(g_themeDir, g_dir); wcscat(g_themeDir, L"themes\\");
    CreateDirectoryW(g_themeDir, NULL);
    wcscpy(t, g_themeDir); wcscat(t, L"icons"); CreateDirectoryW(t, NULL);
    wcscpy(g_ini, g_dir); wcscat(g_ini, L"wlc.ini");
}
static int  RdI(const wchar_t *k, int d) { return (int)GetPrivateProfileIntW(L"cfg", k, d, g_ini); }
static void WrI(const wchar_t *k, int v) {
    wchar_t b[32]; _snwprintf(b, 31, L"%d", v); b[31] = 0; WritePrivateProfileStringW(L"cfg", k, b, g_ini);
}
static int RegDw(const wchar_t *name, int def) {
    HKEY k; DWORD v = 0, sz = sizeof v, ty = 0; int r = def;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\WindowLookChanger", 0, KEY_READ, &k) == ERROR_SUCCESS) {
        if (RegQueryValueExW(k, name, NULL, &ty, (BYTE *)&v, &sz) == ERROR_SUCCESS && ty == REG_DWORD) r = (int)v;
        RegCloseKey(k);
    }
    return r;
}

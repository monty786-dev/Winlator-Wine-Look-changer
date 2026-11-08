/*
 * Window Look Changer  -  plain Win32 (C), mingw-w64, for Winlator / Wine
 *
 * Re-skins the title bar of OTHER Wine application windows so they look like
 * Windows 10 / 7 / 8 / XP / Ubuntu XFCE. For each skinned window it:
 *   1. removes the native caption (WS_CAPTION) of the target,
 *   2. puts a themed, owner-linked overlay title bar directly above it with
 *      icon, title, Minimize / Maximize-Restore / Close, Snap-Left, Snap-Right
 *      and Always-on-top (pin) buttons,
 *   3. keeps overlay and target in sync (move / resize / minimize / focus).
 * Only user32 / gdi32 / comdlg32 / shell32 / kernel32. No .NET, DWM, WPF, UWP.
 * Every Win32 call that can fail is checked; failures go to the status bar.
 */
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

#define APP_TITLE L"Window Look Changer"
#define MAXSKIN 64
#define MAXWIN  200
#define TM_REFRESH 1
#define TM_SYNC    2
#define TM_TIP     3

enum { BT_CLOSE, BT_MAX, BT_MIN, BT_SNAPR, BT_SNAPL, BT_PIN, BT_COUNT };
enum { ACT_SKIN = 1, ACT_UNSKIN, ACT_MIN, ACT_MAX, ACT_LEFT, ACT_RIGHT,
       ACT_TOP, ACT_CLOSE, ACT_KILL, ACT_SKINALL, ACT_UNSKINALL };
enum { IDC_SEARCH = 101, IDC_THEME, IDC_ICONS, IDC_COLOR, IDC_RESET, IDC_REFRESH,
       IDC_MPIN, IDC_MODE, IDC_LIST, IDC_RUN, IDC_BROWSE, IDC_GO, IDC_STATUS,
       IDC_ACT = 120 };           /* IDC_ACT + ACT_x  (buttons)  */
#define CM_BASE 200               /* CM_BASE + ACT_x  (context menu) */

/* ------------------------------------------------------------------ */
/* Themes                                                              */
/* ------------------------------------------------------------------ */
typedef struct {
    const wchar_t *name;
    int kind;                 /* 0 Win10, 1 Win7, 2 Win8, 3 XP, 4 XFCE */
    int height;               /* title bar height                      */
    int bw, bh, bt;           /* button width/height/top offset        */
    int closeW;               /* close button width                    */
    int gap, rmargin, radius;
    int center, bold;
    COLORREF top, bot, textA, textI, border, topI, botI;
} Theme;

#define C_W RGB(255,255,255)
#define C_K RGB(0,0,0)
static const Theme THEMES[5] = {
 { L"Windows 10",  0, 31, 46,31,0, 46, 0,0, 0, 0,0, C_W,C_W, C_K,RGB(128,128,128), RGB(0,120,215), C_W,C_W },
 { L"Windows 7",   1, 30, 28,18,1, 47, 0,6, 7, 0,0, RGB(185,209,234),RGB(125,160,205), C_K,RGB(90,90,90), RGB(40,60,90), RGB(215,228,242),RGB(185,205,225) },
 { L"Windows 8",   2, 31, 34,22,0, 47, 0,0, 0, 0,0, RGB(70,134,198),RGB(70,134,198), C_W,RGB(220,230,240), RGB(70,134,198), RGB(190,200,215),RGB(190,200,215) },
 { L"Windows XP",  3, 30, 21,21,5, 21, 2,5, 8, 0,1, RGB(40,120,250),RGB(0,70,215), C_W,RGB(216,228,248), RGB(0,50,170), RGB(150,178,238),RGB(122,150,223) },
 { L"Ubuntu XFCE", 4, 30, 24,24,3, 24, 4,8, 4, 1,1, RGB(238,237,236),RGB(205,203,200), RGB(60,60,60),RGB(140,140,140), RGB(150,148,144), RGB(245,244,243),RGB(228,227,225) },
};
static const wchar_t *ICON_NAMES[5] = { L"Thin lines", L"Bold lines", L"Color dots", L"Boxed", L"Filled shapes" };
static const wchar_t *BTN_TIPS[BT_COUNT] = { L"Close", L"Maximize / Restore", L"Minimize",
                                             L"Snap to right half", L"Snap to left half", L"Always on top" };
static const COLORREF PALETTE[8] = { RGB(0,120,215), RGB(232,17,35), RGB(16,137,62), RGB(255,140,0),
                                     RGB(136,23,152), RGB(0,153,188), RGB(60,60,60), RGB(233,30,99) };

/* ------------------------------------------------------------------ */
/* Globals                                                             */
/* ------------------------------------------------------------------ */
static HINSTANCE g_inst;
static HWND g_main, g_edSearch, g_cbTheme, g_cbIcon, g_btnColor, g_btnReset, g_btnRef,
            g_btnPin, g_btnMode, g_list, g_edRun, g_btnBrowse, g_btnGo, g_status, g_btnAct[8];
static HFONT g_fUi, g_fSmall, g_fTitle[2], g_fTip;
static HBRUSH g_brDarkBg, g_brDarkEd, g_brLightBg, g_brLightEd;
static HWND g_tip;
static int  g_themeIdx = 0, g_iconStyle = 0, g_hasCol = 0, g_dark = 1, g_mgrTop = 0;
static COLORREF g_col = RGB(0,120,215);
static int  g_winX = CW_USEDEFAULT, g_winY = CW_USEDEFAULT, g_winW = 420, g_winH = 640;
static wchar_t g_ini[MAX_PATH];
static RECT g_prev;                  /* preview rectangle (main client coords) */

static const Theme *CurTheme(void) { return &THEMES[g_themeIdx]; }

/* ------------------------------------------------------------------ */
/* Small helpers                                                       */
/* ------------------------------------------------------------------ */
static COLORREF Blend(COLORREF a, COLORREF b, int p) {
    return RGB((GetRValue(a)*(100-p)+GetRValue(b)*p)/100,
               (GetGValue(a)*(100-p)+GetGValue(b)*p)/100,
               (GetBValue(a)*(100-p)+GetBValue(b)*p)/100);
}
static int Lum(COLORREF c) { return (GetRValue(c)*30 + GetGValue(c)*59 + GetBValue(c)*11) / 100; }

static void FillRectC(HDC dc, int l, int t, int r, int b, COLORREF c) {
    RECT rc = { l, t, r, b };
    HBRUSH br = CreateSolidBrush(c);
    if (br) { FillRect(dc, &rc, br); DeleteObject(br); }
}

static void SetStatus(const wchar_t *fmt, ...) {
    wchar_t buf[300]; va_list ap;
    va_start(ap, fmt); _vsnwprintf(buf, 299, fmt, ap); va_end(ap); buf[299] = 0;
    if (g_status) SetWindowTextW(g_status, buf);
}

static void GetWork(RECT *wa) {
    if (!SystemParametersInfoW(SPI_GETWORKAREA, 0, wa, 0)) {
        SetRect(wa, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    }
}

/* ------------------------------------------------------------------ */
/* Config (INI next to the exe)                                        */
/* ------------------------------------------------------------------ */
static void InitIniPath(void) {
    wchar_t *dot;
    if (!GetModuleFileNameW(NULL, g_ini, MAX_PATH)) { wcscpy(g_ini, L"wlc.ini"); return; }
    dot = wcsrchr(g_ini, L'.');
    if (dot) wcscpy(dot, L".ini"); else wcscat(g_ini, L".ini");
}
static int  RdI(const wchar_t *k, int d) { return (int)GetPrivateProfileIntW(L"cfg", k, d, g_ini); }
static void WrI(const wchar_t *k, int v) { wchar_t b[32]; _snwprintf(b, 31, L"%d", v); b[31] = 0;
                                           WritePrivateProfileStringW(L"cfg", k, b, g_ini); }
static void LoadConfig(void) {
    g_themeIdx  = RdI(L"theme", 0);  if (g_themeIdx < 0 || g_themeIdx > 4) g_themeIdx = 0;
    g_iconStyle = RdI(L"icons", 0);  if (g_iconStyle < 0 || g_iconStyle > 4) g_iconStyle = 0;
    g_hasCol    = RdI(L"hascolor", 0);
    g_col       = (COLORREF)RdI(L"color", (int)RGB(0,120,215));
    g_dark      = RdI(L"dark", 1);
    g_mgrTop    = RdI(L"top", 0);
    g_winW      = RdI(L"w", 420);  g_winH = RdI(L"h", 640);
    g_winX      = RdI(L"x", CW_USEDEFAULT); g_winY = RdI(L"y", CW_USEDEFAULT);
    if (g_winW < 360) g_winW = 420;  if (g_winH < 520) g_winH = 640;
}
static void SaveConfig(void) {
    RECT r;
    WrI(L"theme", g_themeIdx); WrI(L"icons", g_iconStyle); WrI(L"hascolor", g_hasCol);
    WrI(L"color", (int)g_col); WrI(L"dark", g_dark); WrI(L"top", g_mgrTop);
    if (g_main && !IsIconic(g_main) && GetWindowRect(g_main, &r)) {
        WrI(L"x", r.left); WrI(L"y", r.top);
        WrI(L"w", r.right - r.left); WrI(L"h", r.bottom - r.top);
    }
}

/* ------------------------------------------------------------------ */
/* Skin drawing (shared by overlay windows and the preview)            */
/* ------------------------------------------------------------------ */
typedef struct { COLORREF top, bot, fg, border, hover, press; } Cols;
typedef struct { const wchar_t *title; HICON icon; BOOL active; int hot, down; BOOL maxed, pinned; } PState;

static Cols GetCols(const Theme *t, BOOL active) {
    Cols c;
    if (g_hasCol) {
        COLORREF b = g_col;
        if (t->kind == 0 || t->kind == 2) { c.top = c.bot = b; }
        else { c.top = Blend(b, C_W, 30); c.bot = Blend(b, C_K, 12); }
        if (!active) { c.top = Blend(c.top, RGB(205,205,205), 60); c.bot = Blend(c.bot, RGB(205,205,205), 60); }
        c.fg = Lum(c.bot) < 150 ? C_W : C_K;
        c.border = Blend(c.bot, C_K, 30);
    } else {
        c.top = active ? t->top : t->topI;  c.bot = active ? t->bot : t->botI;
        c.fg  = active ? t->textA : t->textI;
        c.border = active ? t->border : Blend(t->border, RGB(190,190,190), 55);
    }
    if (Lum(c.bot) < 150) { c.hover = Blend(c.bot, C_W, 22); c.press = Blend(c.bot, C_W, 38); }
    else                  { c.hover = Blend(c.bot, C_K, 10); c.press = Blend(c.bot, C_K, 20); }
    return c;
}

/* Right-to-left: close, max, min, gap, pin, snapR, snapL. Returns leftmost x. */
static int LayoutBtns(const Theme *t, int w, RECT *r, int *vis) {
    int x = w - t->rmargin, left = w, i;
    int ord[3] = { BT_CLOSE, BT_MAX, BT_MIN };
    int ex[3]  = { BT_PIN, BT_SNAPR, BT_SNAPL };
    for (i = 0; i < BT_COUNT; i++) { vis[i] = 0; SetRectEmpty(&r[i]); }
    for (i = 0; i < 3; i++) {
        int id = ord[i], bw = (id == BT_CLOSE) ? t->closeW : t->bw;
        if (x - bw < 60) break;
        x -= bw; SetRect(&r[id], x, t->bt, x + bw, t->bt + t->bh); vis[id] = 1; left = x; x -= t->gap;
    }
    x -= 6;
    for (i = 0; i < 3; i++) {
        int id = ex[i];
        if (x - t->bw < 110) break;
        x -= t->bw; SetRect(&r[id], x, t->bt, x + t->bw, t->bt + t->bh); vis[id] = 1; left = x; x -= t->gap;
    }
    return left;
}

static void RoundBtn(HDC dc, const RECT *rc, int rad, COLORREF top, COLORREF bot, COLORREF bord) {
    HPEN p = CreatePen(PS_SOLID, 1, bord);
    HBRUSH b = CreateSolidBrush(bot);
    HPEN op = (HPEN)SelectObject(dc, p);
    HBRUSH ob = (HBRUSH)SelectObject(dc, b);
    RoundRect(dc, rc->left, rc->top, rc->right, rc->bottom, rad, rad);
    SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b); DeleteObject(p);
    if (top != bot) FillRectC(dc, rc->left + 1, rc->top + 1, rc->right - 1, (rc->top + rc->bottom) / 2, top);
}

static void DrawBtnFace(HDC dc, const Theme *t, int id, const RECT *rc, int st, const Cols *c, BOOL active) {
    BOOL cl = (id == BT_CLOSE);
    switch (t->kind) {
    case 0:   /* Windows 10 flat */
        if (st) {
            COLORREF f = cl ? (st == 2 ? RGB(241,112,122) : RGB(232,17,35)) : (st == 2 ? c->press : c->hover);
            FillRectC(dc, rc->left, rc->top, rc->right, rc->bottom, f);
        }
        break;
    case 2: { /* Windows 8: red close always, flat others */
        COLORREF f;
        if (cl) f = st == 2 ? RGB(171,50,50) : (st == 1 ? RGB(224,67,67) : RGB(199,80,80));
        else if (st) f = st == 2 ? c->press : c->hover;
        else break;
        if (!active && cl) f = Blend(f, RGB(200,200,200), 40);
        FillRectC(dc, rc->left, rc->top, rc->right, rc->bottom, f);
        break; }
    case 1: { /* Windows 7 glass buttons */
        COLORREF tp = cl ? RGB(230,160,140) : RGB(222,233,246), bt = cl ? RGB(196,64,38) : RGB(168,193,224);
        if (st == 1) { tp = Blend(tp, C_W, 30); bt = Blend(bt, C_W, 30); }
        if (st == 2) { tp = Blend(tp, C_K, 20); bt = Blend(bt, C_K, 20); }
        if (!active) { tp = Blend(tp, RGB(220,220,220), 50); bt = Blend(bt, RGB(220,220,220), 50); }
        RoundBtn(dc, rc, 5, tp, bt, RGB(50,70,100));
        break; }
    case 3: { /* Windows XP rounded square */
        COLORREF f = cl ? RGB(206,84,52) : RGB(36,102,238);
        if (st == 1) f = Blend(f, C_W, 28);
        if (st == 2) f = Blend(f, C_K, 20);
        if (!active) f = Blend(f, RGB(180,195,235), 45);
        RoundBtn(dc, rc, 7, f, f, C_W);
        break; }
    default: { /* XFCE round */
        COLORREF f = st == 2 ? RGB(210,208,205) : (st == 1 ? RGB(252,252,252) : RGB(236,235,233));
        HPEN p; HBRUSH b; HPEN op; HBRUSH ob;
        if (cl && st) f = RGB(222,90,70);
        p = CreatePen(PS_SOLID, 1, RGB(150,148,144)); b = CreateSolidBrush(f);
        op = (HPEN)SelectObject(dc, p); ob = (HBRUSH)SelectObject(dc, b);
        Ellipse(dc, rc->left, rc->top, rc->right, rc->bottom);
        SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b); DeleteObject(p);
        break; }
    }
}

/* Icon sets: 0 thin, 1 bold, 2 colour dots, 3 boxed, 4 filled shapes. All drawn with GDI
   primitives - no icon fonts needed (Marlett / Segoe MDL2 are not reliable on Wine). */
static void DrawGlyph(HDC dc, int id, const RECT *rc, int style, COLORREF c, BOOL maxed, BOOL pinned, BOOL hot) {
    int cx = (rc->left + rc->right) / 2, cy = (rc->top + rc->bottom) / 2;
    const COLORREF AMBER = RGB(255,176,0);
    HPEN pen, op; HBRUSH hb, ob; int pw; BOOL fill; COLORREF gc;

    if (style == 2) {
        COLORREF d; HPEN p; HBRUSH b;
        switch (id) { case BT_CLOSE: d = RGB(255,95,86); break; case BT_MAX: d = RGB(39,201,63); break;
                      case BT_MIN: d = RGB(255,189,46); break;
                      case BT_PIN: d = pinned ? AMBER : RGB(160,160,160); break;
                      default: d = RGB(90,140,255); }
        if (hot) d = Blend(d, C_W, 30);
        p = CreatePen(PS_SOLID, 1, Blend(d, C_K, 30)); b = CreateSolidBrush(d);
        op = (HPEN)SelectObject(dc, p); ob = (HBRUSH)SelectObject(dc, b);
        Ellipse(dc, cx - 7, cy - 7, cx + 8, cy + 8);
        SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b); DeleteObject(p);
        return;
    }
    if (style == 3) {
        int hh = (rc->bottom - rc->top) / 2 - 1; HPEN p; 
        if (hh > 9) hh = 9;
        p = CreatePen(PS_SOLID, 1, c);
        op = (HPEN)SelectObject(dc, p); ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
        RoundRect(dc, cx - hh, cy - hh, cx + hh + 1, cy + hh + 1, 4, 4);
        SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(p);
    }
    pw = (style == 1 || style == 4) ? 2 : 1;
    fill = (style == 4);
    gc = (id == BT_PIN && pinned) ? AMBER : c;
    pen = CreatePen(PS_SOLID, pw, gc);
    hb = fill ? CreateSolidBrush(gc) : (HBRUSH)GetStockObject(NULL_BRUSH);
    op = (HPEN)SelectObject(dc, pen); ob = (HBRUSH)SelectObject(dc, hb);

    switch (id) {
    case BT_MIN:
        if (fill) Rectangle(dc, cx - 5, cy + 1, cx + 6, cy + 4);
        else { MoveToEx(dc, cx - 5, cy + 2, NULL); LineTo(dc, cx + 6, cy + 2); }
        break;
    case BT_MAX:
        if (!maxed) Rectangle(dc, cx - 5, cy - 5, cx + 6, cy + 6);
        else {
            MoveToEx(dc, cx - 3, cy - 3, NULL); LineTo(dc, cx - 3, cy - 6); LineTo(dc, cx + 6, cy - 6);
            LineTo(dc, cx + 6, cy + 3); LineTo(dc, cx + 3, cy + 3);
            Rectangle(dc, cx - 6, cy - 3, cx + 4, cy + 6);
        }
        break;
    case BT_CLOSE: {
        HPEN cp = CreatePen(PS_SOLID, fill ? 3 : pw, gc);
        HPEN old = (HPEN)SelectObject(dc, cp);
        MoveToEx(dc, cx - 5, cy - 5, NULL); LineTo(dc, cx + 6, cy + 6);
        MoveToEx(dc, cx + 5, cy - 5, NULL); LineTo(dc, cx - 6, cy + 6);
        SelectObject(dc, old); DeleteObject(cp);
        break; }
    case BT_SNAPL:
        Rectangle(dc, cx - 6, cy - 5, cx + 7, cy + 7);
        FillRectC(dc, cx - 5, cy - 4, cx, cy + 6, gc);
        break;
    case BT_SNAPR:
        Rectangle(dc, cx - 6, cy - 5, cx + 7, cy + 7);
        FillRectC(dc, cx + 1, cy - 4, cx + 6, cy + 6, gc);
        break;
    case BT_PIN: {
        HBRUSH pb = pinned ? CreateSolidBrush(AMBER) : NULL;
        if (pb) SelectObject(dc, pb);
        Ellipse(dc, cx - 4, cy - 7, cx + 5, cy + 2);
        MoveToEx(dc, cx - 5, cy + 2, NULL); LineTo(dc, cx + 6, cy + 2);
        MoveToEx(dc, cx, cy + 2, NULL); LineTo(dc, cx, cy + 8);
        if (pb) { SelectObject(dc, hb); DeleteObject(pb); }
        break; }
    }
    SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(pen);
    if (fill) DeleteObject(hb);
}

static void PaintSkin(HDC dc, int w, int h, const Theme *t, const PState *s) {
    Cols c = GetCols(t, s->active);
    RECT br[BT_COUNT]; int vis[BT_COUNT]; int left, y, id, tx = 8;
    RECT fr = { 0, 0, w, h }; HBRUSH bb; HFONT of; UINT fl; RECT tr;

    for (y = 0; y < h; y++) {
        COLORREF col = (c.top == c.bot) ? c.top : Blend(c.top, c.bot, h > 1 ? y * 100 / (h - 1) : 0);
        FillRectC(dc, 0, y, w, y + 1, col);
    }
    bb = CreateSolidBrush(c.border);
    if (bb) { FrameRect(dc, &fr, bb); DeleteObject(bb); }
    if (t->kind == 0 || t->kind == 2) FillRectC(dc, 1, h - 1, w - 1, h, c.bot);

    left = LayoutBtns(t, w, br, vis);
    if (s->icon) { DrawIconEx(dc, 8, (h - 16) / 2, s->icon, 16, 16, 0, NULL, DI_NORMAL); tx = 30; }

    of = (HFONT)SelectObject(dc, g_fTitle[t->bold ? 1 : 0]);
    SetBkMode(dc, TRANSPARENT);
    SetRect(&tr, tx, 0, left - 6, h);
    fl = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX | (t->center ? DT_CENTER : DT_LEFT);
    if (tr.right > tr.left && s->title) {
        if (t->kind == 3) { RECT sr = tr; OffsetRect(&sr, 1, 1); SetTextColor(dc, RGB(0,0,90)); DrawTextW(dc, s->title, -1, &sr, fl); }
        SetTextColor(dc, c.fg);
        DrawTextW(dc, s->title, -1, &tr, fl);
    }
    SelectObject(dc, of);

    for (id = 0; id < BT_COUNT; id++) {
        int st; COLORREF gc;
        if (!vis[id]) continue;
        st = (s->hot == id) ? ((s->down == id) ? 2 : 1) : 0;
        if (g_iconStyle != 2) DrawBtnFace(dc, t, id, &br[id], st, &c, s->active);
        switch (t->kind) {
            case 0: gc = (id == BT_CLOSE && st) ? C_W : c.fg; break;
            case 1: gc = (id == BT_CLOSE) ? C_W : RGB(30,40,60); break;
            case 2: gc = (id == BT_CLOSE) ? C_W : c.fg; break;
            case 3: gc = C_W; break;
            default: gc = (id == BT_CLOSE && st) ? C_W : RGB(70,70,70); break;
        }
        DrawGlyph(dc, id, &br[id], g_iconStyle, gc, s->maxed, s->pinned, st != 0);
    }
}

/* ------------------------------------------------------------------ */
/* Tooltip popup                                                       */
/* ------------------------------------------------------------------ */
static void TipHide(void) { if (g_tip) ShowWindow(g_tip, SW_HIDE); }
static void TipShow(const wchar_t *text, int sx, int sy) {
    SIZE sz; HDC dc; HFONT of;
    if (!g_tip) return;
    dc = GetDC(g_tip); if (!dc) return;
    of = (HFONT)SelectObject(dc, g_fTip);
    if (!GetTextExtentPoint32W(dc, text, (int)wcslen(text), &sz)) { sz.cx = 100; sz.cy = 16; }
    SelectObject(dc, of); ReleaseDC(g_tip, dc);
    SetWindowTextW(g_tip, text);
    SetWindowPos(g_tip, HWND_TOPMOST, sx, sy, sz.cx + 12, sz.cy + 6, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(g_tip, NULL, TRUE);
}
static LRESULT CALLBACK TipProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) {
        PAINTSTRUCT ps; wchar_t b[128]; RECT rc; HDC dc = BeginPaint(h, &ps);
        GetClientRect(h, &rc); FillRectC(dc, 0, 0, rc.right, rc.bottom, RGB(255,255,225));
        GetWindowTextW(h, b, 128); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, C_K);
        SelectObject(dc, g_fTip); rc.left += 5; rc.top += 2;
        DrawTextW(dc, b, -1, &rc, DT_SINGLELINE | DT_NOPREFIX);
        EndPaint(h, &ps); return 0;
    }
    if (m == WM_NCHITTEST) return HTTRANSPARENT;
    return DefWindowProcW(h, m, w, l);
}

/* ------------------------------------------------------------------ */
/* Skinned windows                                                     */
/* ------------------------------------------------------------------ */
typedef struct Skin {
    HWND target, over;
    LONG origStyle;
    int  H;                 /* title height currently applied */
    int  state;             /* 0 normal, 1 maximised, 2 snapped */
    RECT restore;
    int  hot, down;
    BOOL pinned, active, owned, dragging;
    POINT dragStart; RECT dragRect;
    wchar_t title[256];
    HICON icon; DWORD iconTick;
    int  lastW, lastH;
} Skin;

static Skin *g_skins[MAXSKIN];
static int   g_nskins;

static Skin *SkinFind(HWND t) {
    int i; for (i = 0; i < g_nskins; i++) if (g_skins[i]->target == t) return g_skins[i];
    return NULL;
}
static Skin *SkinFindOver(HWND o) {
    int i; for (i = 0; i < g_nskins; i++) if (g_skins[i]->over == o) return g_skins[i];
    return NULL;
}

static HICON GetWinIcon(HWND t) {
    DWORD_PTR r = 0;
    if (SendMessageTimeoutW(t, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 40, &r) && r) return (HICON)r;
    r = GetClassLongPtrW(t, GCLP_HICONSM);  if (r) return (HICON)r;
    r = GetClassLongPtrW(t, GCLP_HICON);    return (HICON)r;
}

static void SkinRegion(Skin *e, int w) {
    const Theme *t = CurTheme();
    if (t->radius > 0) {
        HRGN rg = CreateRoundRectRgn(0, 0, w + 1, e->H + t->radius * 2 + 1, t->radius * 2, t->radius * 2);
        if (rg && !SetWindowRgn(e->over, rg, TRUE)) DeleteObject(rg);
    } else SetWindowRgn(e->over, NULL, TRUE);
}

static void SkinMove(Skin *e, int x, int y, int w, int h) {
    SetWindowPos(e->target, NULL, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
}

static void SkinToggleMax(Skin *e) {
    RECT wa; GetWork(&wa);
    if (e->state == 1) {
        e->state = 0;
        SkinMove(e, e->restore.left, e->restore.top, e->restore.right - e->restore.left, e->restore.bottom - e->restore.top);
    } else {
        if (e->state == 0) GetWindowRect(e->target, &e->restore);
        e->state = 1;
        SkinMove(e, wa.left, wa.top + e->H, wa.right - wa.left, wa.bottom - wa.top - e->H);
    }
    if (e->over) InvalidateRect(e->over, NULL, FALSE);
}

static void SkinSnap(Skin *e, int right) {
    RECT wa; int hw, x, w; GetWork(&wa);
    if (e->state == 0) GetWindowRect(e->target, &e->restore);
    e->state = 2;
    hw = (wa.right - wa.left) / 2;
    x = right ? wa.left + hw : wa.left;
    w = right ? (wa.right - wa.left - hw) : hw;
    SkinMove(e, x, wa.top + e->H, w, wa.bottom - wa.top - e->H);
    if (e->over) InvalidateRect(e->over, NULL, FALSE);
}

static void TogglePin(HWND t) {
    LONG ex = GetWindowLongW(t, GWL_EXSTYLE);
    SetWindowPos(t, (ex & WS_EX_TOPMOST) ? HWND_NOTOPMOST : HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

static void SkinPress(Skin *e, int id) {
    switch (id) {
        case BT_MIN:   ShowWindow(e->target, SW_MINIMIZE); break;
        case BT_MAX:   SkinToggleMax(e); break;
        case BT_CLOSE: PostMessageW(e->target, WM_CLOSE, 0, 0); break;
        case BT_SNAPL: SkinSnap(e, 0); break;
        case BT_SNAPR: SkinSnap(e, 1); break;
        case BT_PIN:   TogglePin(e->target); break;
    }
}

static int ButtonAt(const Theme *t, int w, int x, int y) {
    RECT br[BT_COUNT]; int vis[BT_COUNT], i; POINT p = { x, y };
    LayoutBtns(t, w, br, vis);
    for (i = 0; i < BT_COUNT; i++) if (vis[i] && PtInRect(&br[i], p)) return i;
    return -1;
}

static void SkinPaintDC(Skin *e, HDC dc, int w, int h) {
    PState s; s.title = e->title; s.icon = e->icon; s.active = e->active; s.hot = e->hot;
    s.down = e->down; s.maxed = (e->state == 1); s.pinned = e->pinned;
    PaintSkin(dc, w, h, CurTheme(), &s);
}

static LRESULT CALLBACK SkinProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    Skin *e = (Skin *)GetWindowLongPtrW(h, GWLP_USERDATA);
    if (m == WM_NCCREATE) {
        e = (Skin *)((CREATESTRUCTW *)l)->lpCreateParams;
        SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)e);
        return DefWindowProcW(h, m, w, l);
    }
    if (!e) return DefWindowProcW(h, m, w, l);
    switch (m) {
    case WM_ERASEBKGND: return 1;
    case WM_MOUSEACTIVATE: return MA_NOACTIVATE;
    case WM_PAINT: {
        PAINTSTRUCT ps; RECT rc; HDC dc = BeginPaint(h, &ps);
        GetClientRect(h, &rc);
        if (rc.right > 0 && rc.bottom > 0) {
            HDC mem = CreateCompatibleDC(dc);
            HBITMAP bm = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
            if (mem && bm) {
                HBITMAP ob = (HBITMAP)SelectObject(mem, bm);
                SkinPaintDC(e, mem, rc.right, rc.bottom);
                BitBlt(dc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
                SelectObject(mem, ob);
            } else SkinPaintDC(e, dc, rc.right, rc.bottom);
            if (bm) DeleteObject(bm);
            if (mem) DeleteDC(mem);
        }
        EndPaint(h, &ps); return 0; }
    case WM_MOUSEMOVE: {
        int x = (short)LOWORD(l), y = (short)HIWORD(l); RECT rc; int b;
        GetClientRect(h, &rc);
        if (e->dragging && (w & MK_LBUTTON)) {
            POINT p; GetCursorPos(&p);
            SetWindowPos(e->target, NULL, e->dragRect.left + p.x - e->dragStart.x,
                         e->dragRect.top + p.y - e->dragStart.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        }
        b = ButtonAt(CurTheme(), rc.right, x, y);
        if (b != e->hot) {
            TRACKMOUSEEVENT te; te.cbSize = sizeof te; te.dwFlags = TME_LEAVE; te.hwndTrack = h; te.dwHoverTime = 0;
            TrackMouseEvent(&te);
            e->hot = b; TipHide(); KillTimer(h, TM_TIP);
            if (b >= 0) SetTimer(h, TM_TIP, 500, NULL);
            InvalidateRect(h, NULL, FALSE);
        }
        return 0; }
    case WM_MOUSELEAVE:
        e->hot = -1; TipHide(); KillTimer(h, TM_TIP); InvalidateRect(h, NULL, FALSE); return 0;
    case WM_TIMER:
        if (w == TM_TIP) {
            KillTimer(h, TM_TIP);
            if (e->hot >= 0) {
                RECT br[BT_COUNT], rc; int vis[BT_COUNT]; POINT p;
                GetClientRect(h, &rc); LayoutBtns(CurTheme(), rc.right, br, vis);
                p.x = br[e->hot].left; p.y = rc.bottom + 4; ClientToScreen(h, &p);
                TipShow(BTN_TIPS[e->hot], p.x, p.y);
            }
        }
        return 0;
    case WM_LBUTTONDOWN: {
        int x = (short)LOWORD(l), y = (short)HIWORD(l); RECT rc; int b;
        GetClientRect(h, &rc); TipHide();
        SetForegroundWindow(e->target);
        b = ButtonAt(CurTheme(), rc.right, x, y);
        SetCapture(h);
        if (b >= 0) { e->down = b; InvalidateRect(h, NULL, FALSE); }
        else if (e->state != 1) {
            e->dragging = TRUE; GetCursorPos(&e->dragStart); GetWindowRect(e->target, &e->dragRect);
            if (e->state == 2) e->state = 0;
        }
        return 0; }
    case WM_LBUTTONUP: {
        int x = (short)LOWORD(l), y = (short)HIWORD(l); RECT rc; int b, d = e->down;
        GetClientRect(h, &rc);
        e->dragging = FALSE; e->down = -1;
        if (GetCapture() == h) ReleaseCapture();
        b = ButtonAt(CurTheme(), rc.right, x, y);
        InvalidateRect(h, NULL, FALSE);
        if (d >= 0 && d == b) SkinPress(e, d);
        return 0; }
    case WM_LBUTTONDBLCLK: {
        int x = (short)LOWORD(l), y = (short)HIWORD(l); RECT rc; GetClientRect(h, &rc);
        if (ButtonAt(CurTheme(), rc.right, x, y) < 0) SkinToggleMax(e);
        return 0; }
    case WM_CAPTURECHANGED: e->dragging = FALSE; e->down = -1; return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static void SkinDestroy(Skin *e, BOOL restoreWin) {
    int i;
    if (restoreWin && IsWindow(e->target)) {
        RECT r; LONG cur;
        if (e->state != 0)
            SkinMove(e, e->restore.left, e->restore.top, e->restore.right - e->restore.left, e->restore.bottom - e->restore.top);
        cur = GetWindowLongW(e->target, GWL_STYLE);
        SetWindowLongW(e->target, GWL_STYLE, (cur & ~WS_CAPTION) | (e->origStyle & WS_CAPTION));
        SetWindowPos(e->target, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        if (GetWindowRect(e->target, &r))
            SetWindowPos(e->target, NULL, r.left, r.top - e->H, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (e->over) { TipHide(); DestroyWindow(e->over); }
    for (i = 0; i < g_nskins; i++) if (g_skins[i] == e) { g_skins[i] = g_skins[--g_nskins]; break; }
    free(e);
}

/* returns 0 ok, 1 no title bar, 2 limit, 3 already skinned, 4 create failed */
static int SkinApply(HWND t) {
    Skin *e; LONG st; RECT r0, r1, wa; int H = CurTheme()->height, x, y, w;
    if (SkinFind(t)) return 3;
    if (g_nskins >= MAXSKIN) return 2;
    st = GetWindowLongW(t, GWL_STYLE);
    if ((st & WS_CAPTION) != WS_CAPTION) return 1;
    if (IsIconic(t) || IsZoomed(t)) ShowWindow(t, SW_RESTORE);
    if (!GetWindowRect(t, &r0)) return 1;
    (void)wa;
    e = (Skin *)calloc(1, sizeof(Skin));
    if (!e) return 4;
    e->target = t; e->origStyle = st; e->H = H; e->hot = -1; e->down = -1; e->lastW = -1; e->lastH = -1;
    GetWindowTextW(t, e->title, 256);

    SetWindowLongW(t, GWL_STYLE, st & ~WS_CAPTION);
    SetWindowPos(t, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    if (!GetWindowRect(t, &r1)) r1 = r0;
    SetWindowPos(t, NULL, r0.left, r0.top + H, r1.right - r1.left, r1.bottom - r1.top, SWP_NOZORDER | SWP_NOACTIVATE);
    if (!GetWindowRect(t, &r1)) r1 = r0;
    x = r1.left; y = r1.top - H; w = r1.right - r1.left;

    e->over = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, L"WLCSkin", L"", WS_POPUP,
                              x, y, w, H, t, NULL, g_inst, e);
    e->owned = (e->over != NULL);
    if (!e->over)
        e->over = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, L"WLCSkin", L"", WS_POPUP,
                                  x, y, w, H, NULL, NULL, g_inst, e);
    if (!e->over) {
        SetWindowLongW(t, GWL_STYLE, st);
        SetWindowPos(t, NULL, r0.left, r0.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        free(e); return 4;
    }
    g_skins[g_nskins++] = e;
    SkinRegion(e, w);
    ShowWindow(e->over, SW_SHOWNOACTIVATE);
    return 0;
}

static void SkinSync(Skin *e) {
    RECT tr, orc; HWND fg; BOOL act, pin; int w, x, y;
    if (!IsWindow(e->target)) { SkinDestroy(e, FALSE); return; }
    if (IsIconic(e->target) || !IsWindowVisible(e->target)) {
        if (IsWindowVisible(e->over)) ShowWindow(e->over, SW_HIDE);
        return;
    }
    if (!GetWindowRect(e->target, &tr)) return;
    w = tr.right - tr.left; x = tr.left; y = tr.top - e->H;
    if (!IsWindowVisible(e->over)) { ShowWindow(e->over, SW_SHOWNOACTIVATE); InvalidateRect(e->over, NULL, FALSE); }
    if (!GetWindowRect(e->over, &orc) || orc.left != x || orc.top != y ||
        orc.right - orc.left != w || orc.bottom - orc.top != e->H) {
        SetWindowPos(e->over, NULL, x, y, w, e->H, SWP_NOACTIVATE | SWP_NOZORDER);
        InvalidateRect(e->over, NULL, FALSE);
    }
    if (e->lastW != w || e->lastH != e->H) { SkinRegion(e, w); e->lastW = w; e->lastH = e->H; }
    if (!e->owned) {                       /* fallback z-order: keep overlay directly above target */
        HWND prev = GetWindow(e->target, GW_HWNDPREV);
        if (prev != e->over)
            SetWindowPos(e->over, prev ? prev : HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    fg = GetForegroundWindow();
    act = (fg == e->target || fg == e->over);
    pin = (GetWindowLongW(e->target, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
    if (act != e->active || pin != e->pinned) { e->active = act; e->pinned = pin; InvalidateRect(e->over, NULL, FALSE); }
    {
        wchar_t t[256]; DWORD now = GetTickCount();
        if (now - e->iconTick > 2000 || e->iconTick == 0) { e->icon = GetWinIcon(e->target); e->iconTick = now; InvalidateRect(e->over, NULL, FALSE); }
        t[0] = 0; GetWindowTextW(e->target, t, 256);
        if (wcscmp(t, e->title) != 0) { wcscpy(e->title, t); InvalidateRect(e->over, NULL, FALSE); }
    }
}

static void SyncAll(void) { int i; for (i = g_nskins - 1; i >= 0; i--) if (i < g_nskins) SkinSync(g_skins[i]); }

/* Theme / icon / colour changed: re-lay out every skinned window */
static void ApplyLookAll(void) {
    int i, H = CurTheme()->height;
    for (i = 0; i < g_nskins; i++) {
        Skin *e = g_skins[i];
        if (!IsWindow(e->target)) continue;
        if (e->H != H) {
            RECT r;
            if (GetWindowRect(e->target, &r)) {
                int ndy = H - e->H;
                if (e->state == 1) {
                    RECT wa; GetWork(&wa);
                    e->H = H; SkinMove(e, wa.left, wa.top + H, wa.right - wa.left, wa.bottom - wa.top - H);
                } else {
                    SetWindowPos(e->target, NULL, r.left, r.top + ndy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                    e->H = H;
                }
            }
        }
        e->lastW = -1;
        if (e->over) InvalidateRect(e->over, NULL, FALSE);
    }
    SyncAll();
    if (g_main) InvalidateRect(g_main, &g_prev, FALSE);
}

static void SkinRemoveAll(void) { while (g_nskins > 0) SkinDestroy(g_skins[g_nskins - 1], TRUE); }

/* ------------------------------------------------------------------ */
/* Window enumeration                                                  */
/* ------------------------------------------------------------------ */
typedef struct { HWND h; wchar_t title[256]; wchar_t proc[64]; DWORD pid; BOOL iconic, zoomed, top, skinned; } WinInfo;
static WinInfo g_wins[MAXWIN], g_tmp[MAXWIN];
static int g_nwins, g_ntmp;
static wchar_t g_filter[128];
static unsigned g_sig;

static void LowerCopy(wchar_t *d, const wchar_t *s, int n) {
    int i = 0; for (; s[i] && i < n - 1; i++) d[i] = (wchar_t)towlower(s[i]); d[i] = 0;
}

static void ProcName(DWORD pid, wchar_t *out, int n) {
    HANDLE ph = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    out[0] = 0;
    if (ph) {
        wchar_t full[MAX_PATH]; DWORD sz = MAX_PATH;
        if (QueryFullProcessImageNameW(ph, 0, full, &sz)) {
            wchar_t *b = wcsrchr(full, L'\\'); if (!b) b = wcsrchr(full, L'/');
            wcsncpy(out, b ? b + 1 : full, n - 1); out[n - 1] = 0;
        }
        CloseHandle(ph);
    }
    if (!out[0]) _snwprintf(out, n - 1, L"pid %lu", (unsigned long)pid);
}

static BOOL CALLBACK EnumProc(HWND h, LPARAM lp) {
    wchar_t t[256], cls[64], lt[256], lf[128]; DWORD pid; LONG ex; WinInfo *w;
    (void)lp;
    if (g_ntmp >= MAXWIN) return FALSE;
    if (!IsWindowVisible(h)) return TRUE;
    if (GetWindowTextW(h, t, 256) <= 0) return TRUE;
    GetWindowThreadProcessId(h, &pid);
    if (pid == GetCurrentProcessId()) return TRUE;               /* skip ourselves + our overlays */
    ex = GetWindowLongW(h, GWL_EXSTYLE);
    if ((ex & WS_EX_TOOLWINDOW) && !(ex & WS_EX_APPWINDOW)) return TRUE;
    if (GetWindow(h, GW_OWNER) && !(ex & WS_EX_APPWINDOW)) return TRUE;
    cls[0] = 0; GetClassNameW(h, cls, 64);
    if (!wcscmp(cls, L"Progman") || !wcscmp(cls, L"Shell_TrayWnd") || !wcscmp(cls, L"WorkerW")) return TRUE;
    if (!wcscmp(t, L"Program Manager")) return TRUE;
    if (g_filter[0]) { LowerCopy(lt, t, 256); LowerCopy(lf, g_filter, 128); if (!wcsstr(lt, lf)) return TRUE; }
    w = &g_tmp[g_ntmp++];
    w->h = h; wcsncpy(w->title, t, 255); w->title[255] = 0; w->pid = pid;
    ProcName(pid, w->proc, 64);
    w->iconic = IsIconic(h); w->zoomed = IsZoomed(h) || (SkinFind(h) && SkinFind(h)->state == 1);
    w->top = (ex & WS_EX_TOPMOST) != 0; w->skinned = SkinFind(h) != NULL;
    return TRUE;
}

/* get_windows(): enumerate into g_tmp. Returns TRUE if the list changed. */
static BOOL get_windows(void) {
    unsigned sig = 2166136261u; int i; const wchar_t *p;
    g_ntmp = 0;
    GetWindowTextW(g_edSearch, g_filter, 128);
    EnumWindows(EnumProc, 0);
    for (i = 0; i < g_ntmp; i++) {
        sig = (sig ^ (unsigned)(ULONG_PTR)g_tmp[i].h) * 16777619u;
        for (p = g_tmp[i].title; *p; p++) sig = (sig ^ (unsigned)*p) * 16777619u;
        sig = (sig ^ (g_tmp[i].iconic | g_tmp[i].zoomed << 1 | g_tmp[i].top << 2 | g_tmp[i].skinned << 3)) * 16777619u;
    }
    sig ^= (unsigned)g_ntmp;
    if (sig == g_sig && g_ntmp == g_nwins) return FALSE;
    g_sig = sig;
    return TRUE;
}

static HWND SelectedHwnd(void) {
    int i = (int)SendMessageW(g_list, LB_GETCURSEL, 0, 0);
    if (i >= 0 && i < g_nwins && IsWindow(g_wins[i].h)) return g_wins[i].h;
    return NULL;
}

static void Refresh(BOOL force) {
    HWND sel; int top, i, idx = -1;
    if (!get_windows() && !force) return;
    sel = SelectedHwnd();
    memcpy(g_wins, g_tmp, sizeof(WinInfo) * g_ntmp); g_nwins = g_ntmp;
    top = (int)SendMessageW(g_list, LB_GETTOPINDEX, 0, 0);
    SendMessageW(g_list, WM_SETREDRAW, FALSE, 0);
    SendMessageW(g_list, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < g_nwins; i++) {
        SendMessageW(g_list, LB_ADDSTRING, 0, (LPARAM)i);
        if (g_wins[i].h == sel) idx = i;
    }
    if (idx >= 0) SendMessageW(g_list, LB_SETCURSEL, idx, 0);
    if (top >= 0 && top < g_nwins) SendMessageW(g_list, LB_SETTOPINDEX, top, 0);
    SendMessageW(g_list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_list, NULL, FALSE);
}

/* ------------------------------------------------------------------ */
/* Actions on a target window                                          */
/* ------------------------------------------------------------------ */
static void snap_window(HWND t, int right) {
    Skin *e = SkinFind(t); RECT wa; int hw;
    if (e) { SkinSnap(e, right); return; }
    GetWork(&wa); hw = (wa.right - wa.left) / 2;
    if (IsZoomed(t) || IsIconic(t)) ShowWindow(t, SW_RESTORE);
    MoveWindow(t, right ? wa.left + hw : wa.left, wa.top, right ? wa.right - wa.left - hw : hw, wa.bottom - wa.top, TRUE);
}

static void DoAction(int act, HWND t) {
    Skin *e;
    if (act == ACT_SKINALL) {
        int i, n = 0, f = 0;
        for (i = 0; i < g_nwins; i++) { int r = SkinApply(g_wins[i].h); if (r == 0) n++; else if (r != 3) f++; }
        SetStatus(L"Skinned %d window(s), %d skipped.", n, f); Refresh(TRUE); return;
    }
    if (act == ACT_UNSKINALL) { SkinRemoveAll(); SetStatus(L"All skins removed."); Refresh(TRUE); return; }
    if (!t) { SetStatus(L"Select a window in the list first."); return; }
    e = SkinFind(t);
    switch (act) {
    case ACT_SKIN: {
        int r = SkinApply(t);
        static const wchar_t *msg[] = { L"Skin applied.", L"Window has no title bar to replace.",
            L"Too many skinned windows.", L"Already skinned.", L"Could not create overlay window." };
        SetStatus(L"%s", msg[r]); break; }
    case ACT_UNSKIN:
        if (e) { SkinDestroy(e, TRUE); SetStatus(L"Skin removed."); } else SetStatus(L"That window is not skinned.");
        break;
    case ACT_MIN:   ShowWindow(t, SW_MINIMIZE); break;
    case ACT_MAX:   if (e) SkinToggleMax(e); else ShowWindow(t, IsZoomed(t) ? SW_RESTORE : SW_MAXIMIZE); break;
    case ACT_LEFT:  snap_window(t, 0); break;
    case ACT_RIGHT: snap_window(t, 1); break;
    case ACT_TOP:   TogglePin(t); break;
    case ACT_CLOSE: PostMessageW(t, WM_CLOSE, 0, 0); break;
    case ACT_KILL: {
        DWORD pid = 0; HANDLE ph; GetWindowThreadProcessId(t, &pid);
        if (MessageBoxW(g_main, L"Terminate this process?", APP_TITLE, MB_YESNO | MB_ICONWARNING) == IDYES) {
            if (e) SkinDestroy(e, FALSE);
            ph = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
            if (ph && TerminateProcess(ph, 1)) SetStatus(L"Process terminated.");
            else SetStatus(L"Could not terminate process (error %lu).", GetLastError());
            if (ph) CloseHandle(ph);
        }
        break; }
    }
    Refresh(TRUE);
}

/* ------------------------------------------------------------------ */
/* Preview of the current theme (title bar, work area, scroll bar)     */
/* ------------------------------------------------------------------ */
static void DrawArrow(HDC dc, int cx, int cy, int up, COLORREF c) {
    POINT p[3]; HBRUSH b = CreateSolidBrush(c); HPEN pn = CreatePen(PS_SOLID, 1, c);
    HBRUSH ob = (HBRUSH)SelectObject(dc, b); HPEN op = (HPEN)SelectObject(dc, pn);
    if (up) { p[0].x = cx - 3; p[0].y = cy + 2; p[1].x = cx + 4; p[1].y = cy + 2; p[2].x = cx; p[2].y = cy - 2; }
    else    { p[0].x = cx - 3; p[0].y = cy - 2; p[1].x = cx + 4; p[1].y = cy - 2; p[2].x = cx; p[2].y = cy + 2; }
    Polygon(dc, p, 3);
    SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b); DeleteObject(pn);
}

static void DrawScrollbar(HDC dc, RECT r, int kind) {
    int w = r.right - r.left, ty = r.top + w + 10, th = 44;
    COLORREF track, thumbT, thumbB, thumbBd, arrow;
    switch (kind) {
        case 0: track = RGB(240,240,240); thumbT = thumbB = RGB(205,205,205); thumbBd = RGB(205,205,205); arrow = RGB(96,96,96); break;
        case 1: track = RGB(233,236,240); thumbT = RGB(235,242,250); thumbB = RGB(190,205,225); thumbBd = RGB(120,135,160); arrow = RGB(50,70,100); break;
        case 2: track = RGB(240,240,240); thumbT = thumbB = RGB(166,166,166); thumbBd = RGB(166,166,166); arrow = RGB(100,100,100); break;
        case 3: track = RGB(244,243,238); thumbT = RGB(190,210,250); thumbB = RGB(110,150,240); thumbBd = RGB(60,100,200); arrow = RGB(60,90,170); break;
        default: track = RGB(228,227,225); thumbT = RGB(205,203,200); thumbB = RGB(185,183,180); thumbBd = RGB(150,148,144); arrow = RGB(90,90,90); break;
    }
    if (g_hasCol) { thumbT = Blend(thumbT, g_col, 45); thumbB = Blend(thumbB, g_col, 45); thumbBd = Blend(thumbBd, g_col, 45); }
    FillRectC(dc, r.left, r.top, r.right, r.bottom, track);
    if (kind == 3 || kind == 1) {
        RECT a = { r.left, r.top, r.right, r.top + w }, b = { r.left, r.bottom - w, r.right, r.bottom };
        RoundBtn(dc, &a, 4, thumbT, thumbB, thumbBd); RoundBtn(dc, &b, 4, thumbT, thumbB, thumbBd);
    }
    DrawArrow(dc, (r.left + r.right) / 2, r.top + w / 2, 1, arrow);
    DrawArrow(dc, (r.left + r.right) / 2, r.bottom - w / 2, 0, arrow);
    if (ty + th > r.bottom - w - 4) th = r.bottom - w - 4 - ty;
    if (th > 8) {
        RECT tb = { r.left + 2, ty, r.right - 2, ty + th };
        if (kind == 0) { tb.left += 2; tb.right -= 2; }
        RoundBtn(dc, &tb, kind == 4 ? 8 : (kind == 0 || kind == 2 ? 0 : 4), thumbT, thumbB, thumbBd);
    }
}

static void PaintPreview(HDC dc, RECT r) {
    const Theme *t = CurTheme();
    int w = r.right - r.left, h = r.bottom - r.top, mx = 10, my = 8, ww = w - 2 * mx, wh = h - 2 * my, H = t->height;
    HDC m = CreateCompatibleDC(dc), m2 = CreateCompatibleDC(dc);
    HBITMAP bm = CreateCompatibleBitmap(dc, w, h), bm2 = CreateCompatibleBitmap(dc, ww > 0 ? ww : 1, H);
    HBITMAP ob, ob2; Cols c; COLORREF wb; RECT mr; HBRUSH bd; PState s; int wy, i;
    static const wchar_t *TXT = L"File    Edit    View    Help";
    if (!m || !m2 || !bm || !bm2 || ww < 100 || wh < H + 30) goto done;
    ob = (HBITMAP)SelectObject(m, bm); ob2 = (HBITMAP)SelectObject(m2, bm2);
    FillRectC(m, 0, 0, w, h, g_dark ? RGB(20,20,20) : RGB(190,196,208));
    s.title = L"Notepad - Preview"; s.icon = LoadIconW(NULL, IDI_APPLICATION); s.active = TRUE;
    s.hot = BT_MAX; s.down = -1; s.maxed = FALSE; s.pinned = TRUE;
    PaintSkin(m2, ww, H, t, &s);
    BitBlt(m, mx, my, ww, H, m2, 0, 0, SRCCOPY);

    c = GetCols(t, TRUE);
    wb = (t->kind == 3) ? RGB(236,233,216) : (t->kind == 4 ? RGB(237,236,235) : C_W);
    wy = my + H;
    FillRectC(m, mx, wy, mx + ww, my + wh, wb);
    bd = CreateSolidBrush(c.border);
    if (bd) { FillRectC(m, mx, wy, mx + 1, my + wh, c.border); FillRectC(m, mx + ww - 1, wy, mx + ww, my + wh, c.border);
              FillRectC(m, mx, my + wh - 1, mx + ww, my + wh, c.border); DeleteObject(bd); }
    SetBkMode(m, TRANSPARENT); SetTextColor(m, C_K); SelectObject(m, g_fSmall);
    SetRect(&mr, mx + 8, wy + 3, mx + ww - 20, wy + 22); DrawTextW(m, TXT, -1, &mr, DT_SINGLELINE | DT_NOPREFIX);
    FillRectC(m, mx + 1, wy + 24, mx + ww - 1, wy + 25, Blend(wb, C_K, 15));
    for (i = 0; i < 3; i++) FillRectC(m, mx + 10, wy + 34 + i * 12, mx + 10 + ww / 2 - i * 20, wy + 38 + i * 12, Blend(wb, C_K, 30));
    { RECT sb = { mx + ww - 18, wy + 25, mx + ww - 1, my + wh - 1 }; DrawScrollbar(m, sb, t->kind); }

    BitBlt(dc, r.left, r.top, w, h, m, 0, 0, SRCCOPY);
    SelectObject(m, ob); SelectObject(m2, ob2);
done:
    if (bm) DeleteObject(bm); if (bm2) DeleteObject(bm2);
    if (m) DeleteDC(m); if (m2) DeleteDC(m2);
}

/* ------------------------------------------------------------------ */
/* Manager window                                                      */
/* ------------------------------------------------------------------ */
static HWND MkCtl(const wchar_t *cls, const wchar_t *txt, DWORD style, DWORD ex, HWND p, int id) {
    HWND h = CreateWindowExW(ex, cls, txt, WS_CHILD | WS_VISIBLE | style, 0, 0, 10, 10, p, (HMENU)(INT_PTR)id, g_inst, NULL);
    if (h) SendMessageW(h, WM_SETFONT, (WPARAM)g_fUi, TRUE);
    return h;
}

static void Layout(void) {
    RECT rc; int W, Hh, m = 6, bh = 44, g = 4, y, sw, i, bw, yStat, yRun, yAct, x;
    if (!GetClientRect(g_main, &rc)) return;
    W = rc.right; Hh = rc.bottom;
    y = m; sw = W - 2 * m - 3 * (44 + g);
    MoveWindow(g_edSearch, m, y, sw, bh, TRUE);
    MoveWindow(g_btnRef,  m + sw + g, y, 44, bh, TRUE);
    MoveWindow(g_btnPin,  m + sw + g + 48, y, 44, bh, TRUE);
    MoveWindow(g_btnMode, m + sw + g + 96, y, 44, bh, TRUE);
    y += bh + g;
    sw = W - 2 * m - 112 - 2 * (44 + g) - g;
    MoveWindow(g_cbTheme, m, y, sw, 300, TRUE);
    MoveWindow(g_cbIcon, m + sw + g, y, 112, 300, TRUE);
    MoveWindow(g_btnColor, m + sw + g + 116, y, 44, bh, TRUE);
    MoveWindow(g_btnReset, m + sw + g + 164, y, 44, bh, TRUE);
    y += bh + g;
    SetRect(&g_prev, m, y, W - m, y + 130);
    y += 130 + g;
    yStat = Hh - 22; yRun = yStat - g - bh; yAct = yRun - g - bh;
    MoveWindow(g_list, m, y, W - 2 * m, yAct - g - y > 40 ? yAct - g - y : 40, TRUE);
    bw = (W - 2 * m - 7 * g) / 8;
    for (i = 0; i < 8; i++) MoveWindow(g_btnAct[i], m + i * (bw + g), yAct, bw, bh, TRUE);
    sw = W - 2 * m - 70 - 56 - 2 * g;
    x = m;
    MoveWindow(g_edRun, x, yRun, sw, bh, TRUE);
    MoveWindow(g_btnBrowse, x + sw + g, yRun, 70, bh, TRUE);
    MoveWindow(g_btnGo, x + sw + g + 74, yRun, 56, bh, TRUE);
    MoveWindow(g_status, m, yStat, W - 2 * m, 20, TRUE);
}

static void DoRun(void) {
    wchar_t cmd[MAX_PATH * 2]; HINSTANCE r;
    GetWindowTextW(g_edRun, cmd, MAX_PATH * 2);
    if (!cmd[0]) { SetStatus(L"Type or browse a program to run."); return; }
    r = ShellExecuteW(g_main, L"open", cmd, NULL, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)r > 32) { SetStatus(L"Started."); return; }
    {   STARTUPINFOW si; PROCESS_INFORMATION pi;
        memset(&si, 0, sizeof si); si.cb = sizeof si;
        if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hThread); CloseHandle(pi.hProcess); SetStatus(L"Started.");
        } else SetStatus(L"Run failed (error %lu).", GetLastError());
    }
}

static void DoBrowse(void) {
    wchar_t f[MAX_PATH] = L""; OPENFILENAMEW o;
    memset(&o, 0, sizeof o); o.lStructSize = sizeof o; o.hwndOwner = g_main;
    o.lpstrFilter = L"Programs\0*.exe;*.bat;*.lnk\0All files\0*.*\0";
    o.lpstrFile = f; o.nMaxFile = MAX_PATH; o.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameW(&o)) SetWindowTextW(g_edRun, f);
}

static void PickColor(void) {
    static COLORREF cust[16]; CHOOSECOLORW cc; static int pi = 0;
    memset(&cc, 0, sizeof cc); cc.lStructSize = sizeof cc; cc.hwndOwner = g_main;
    cc.rgbResult = g_col; cc.lpCustColors = cust; cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (ChooseColorW(&cc)) { g_col = cc.rgbResult; g_hasCol = 1; }
    else if (CommDlgExtendedError() != 0) {          /* dialog unavailable: cycle presets */
        g_col = PALETTE[pi++ & 7]; g_hasCol = 1; SetStatus(L"Colour dialog unavailable - using preset colour.");
    } else return;
    ApplyLookAll();
}

static void DrawListItem(DRAWITEMSTRUCT *d) {
    WinInfo *w; RECT rc = d->rcItem, r1, r2; BOOL sel = (d->itemState & ODS_SELECTED) != 0;
    wchar_t line[160]; const wchar_t *st; COLORREF bg, fg, sub;
    if ((int)d->itemID < 0 || (int)d->itemID >= g_nwins) { FillRectC(d->hDC, rc.left, rc.top, rc.right, rc.bottom, g_dark ? RGB(37,37,38) : C_W); return; }
    w = &g_wins[d->itemID];
    bg  = sel ? RGB(58,134,255) : (g_dark ? RGB(37,37,38) : C_W);
    fg  = sel ? C_W : (g_dark ? RGB(235,235,235) : C_K);
    sub = sel ? RGB(225,235,255) : (g_dark ? RGB(150,150,150) : RGB(100,100,100));
    FillRectC(d->hDC, rc.left, rc.top, rc.right, rc.bottom, bg);
    FillRectC(d->hDC, rc.left, rc.bottom - 1, rc.right, rc.bottom, g_dark ? RGB(55,55,55) : RGB(225,225,225));
    st = w->iconic ? L"minimized" : (w->zoomed ? L"maximized" : L"normal");
    _snwprintf(line, 159, L"%s  |  %s%s%s", w->proc, st, w->top ? L"  |  pinned" : L"", w->skinned ? L"  |  SKINNED" : L"");
    line[159] = 0;
    SetBkMode(d->hDC, TRANSPARENT);
    r1 = rc; r1.left += 10; r1.right -= 8; r1.top += 6; r1.bottom = r1.top + 20;
    SelectObject(d->hDC, g_fUi); SetTextColor(d->hDC, fg);
    DrawTextW(d->hDC, w->title, -1, &r1, DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    r2 = rc; r2.left += 10; r2.right -= 8; r2.top += 28; r2.bottom -= 2;
    SelectObject(d->hDC, g_fSmall); SetTextColor(d->hDC, sub);
    DrawTextW(d->hDC, line, -1, &r2, DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
}

static void ShowContextMenu(int sx, int sy) {
    HWND t = SelectedHwnd(); HMENU mn; int cmd; Skin *e;
    if (!t) return;
    e = SkinFind(t); mn = CreatePopupMenu(); if (!mn) return;
    AppendMenuW(mn, MF_STRING, CM_BASE + (e ? ACT_UNSKIN : ACT_SKIN), e ? L"Remove skin" : L"Apply skin");
    AppendMenuW(mn, MF_SEPARATOR, 0, NULL);
    AppendMenuW(mn, MF_STRING, CM_BASE + ACT_MIN, L"Minimize");
    AppendMenuW(mn, MF_STRING, CM_BASE + ACT_MAX, L"Maximize / Restore");
    AppendMenuW(mn, MF_STRING, CM_BASE + ACT_LEFT, L"Snap left half");
    AppendMenuW(mn, MF_STRING, CM_BASE + ACT_RIGHT, L"Snap right half");
    AppendMenuW(mn, MF_STRING | ((GetWindowLongW(t, GWL_EXSTYLE) & WS_EX_TOPMOST) ? MF_CHECKED : 0), CM_BASE + ACT_TOP, L"Always on top");
    AppendMenuW(mn, MF_STRING, CM_BASE + ACT_CLOSE, L"Close");
    AppendMenuW(mn, MF_SEPARATOR, 0, NULL);
    AppendMenuW(mn, MF_STRING, CM_BASE + ACT_SKINALL, L"Skin all listed windows");
    AppendMenuW(mn, MF_STRING, CM_BASE + ACT_UNSKINALL, L"Remove all skins");
    AppendMenuW(mn, MF_STRING, CM_BASE + ACT_KILL, L"Kill process");
    cmd = TrackPopupMenu(mn, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, sx, sy, 0, g_main, NULL);
    DestroyMenu(mn);
    if (cmd >= CM_BASE) DoAction(cmd - CM_BASE, t);
}

static void SetMgrTop(void) {
    SetWindowPos(g_main, g_mgrTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    SetStatus(g_mgrTop ? L"Manager pinned on top." : L"Manager unpinned.");
}

static LRESULT CALLBACK MainProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        static const wchar_t *aTxt[8] = { L"Skin", L"Unskin", L"Min", L"Max", L"Left", L"Right", L"Pin", L"Close" };
        int i; g_main = h;
        g_edSearch = MkCtl(L"EDIT", L"", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, h, IDC_SEARCH);
        g_btnRef   = MkCtl(L"BUTTON", L"Refr", BS_PUSHBUTTON, 0, h, IDC_REFRESH);
        g_btnPin   = MkCtl(L"BUTTON", L"Pin", BS_PUSHBUTTON, 0, h, IDC_MPIN);
        g_btnMode  = MkCtl(L"BUTTON", L"Mode", BS_PUSHBUTTON, 0, h, IDC_MODE);
        g_cbTheme  = MkCtl(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, 0, h, IDC_THEME);
        g_cbIcon   = MkCtl(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, 0, h, IDC_ICONS);
        g_btnColor = MkCtl(L"BUTTON", L"Clr", BS_PUSHBUTTON, 0, h, IDC_COLOR);
        g_btnReset = MkCtl(L"BUTTON", L"Rst", BS_PUSHBUTTON, 0, h, IDC_RESET);
        g_list     = MkCtl(L"LISTBOX", L"", LBS_OWNERDRAWFIXED | LBS_NOTIFY | WS_VSCROLL | LBS_NOINTEGRALHEIGHT, WS_EX_CLIENTEDGE, h, IDC_LIST);
        for (i = 0; i < 8; i++) g_btnAct[i] = MkCtl(L"BUTTON", aTxt[i], BS_PUSHBUTTON, 0, h, IDC_ACT + i + 1);
        g_edRun    = MkCtl(L"EDIT", L"", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, h, IDC_RUN);
        g_btnBrowse= MkCtl(L"BUTTON", L"Browse", BS_PUSHBUTTON, 0, h, IDC_BROWSE);
        g_btnGo    = MkCtl(L"BUTTON", L"Run", BS_PUSHBUTTON, 0, h, IDC_GO);
        g_status   = MkCtl(L"STATIC", L"", SS_LEFT | SS_NOPREFIX, 0, h, IDC_STATUS);
        SendMessageW(g_status, WM_SETFONT, (WPARAM)g_fSmall, TRUE);
        for (i = 0; i < 5; i++) SendMessageW(g_cbTheme, CB_ADDSTRING, 0, (LPARAM)THEMES[i].name);
        for (i = 0; i < 5; i++) SendMessageW(g_cbIcon, CB_ADDSTRING, 0, (LPARAM)ICON_NAMES[i]);
        SendMessageW(g_cbTheme, CB_SETCURSEL, g_themeIdx, 0);
        SendMessageW(g_cbIcon, CB_SETCURSEL, g_iconStyle, 0);
        SendMessageW(g_cbTheme, CB_SETITEMHEIGHT, (WPARAM)-1, 32);
        SendMessageW(g_cbIcon, CB_SETITEMHEIGHT, (WPARAM)-1, 32);
        SendMessageW(g_cbTheme, CB_SETITEMHEIGHT, 0, 40);
        SendMessageW(g_cbIcon, CB_SETITEMHEIGHT, 0, 40);
        SetTimer(h, TM_REFRESH, 1500, NULL);
        SetTimer(h, TM_SYNC, 33, NULL);
        Layout();
        SetStatus(L"Type in the top box to filter. Tap a window, then Skin.");
        Refresh(TRUE);
        return 0; }
    case WM_GETMINMAXINFO: { MINMAXINFO *mi = (MINMAXINFO *)l; mi->ptMinTrackSize.x = 360; mi->ptMinTrackSize.y = 520; return 0; }
    case WM_SIZE: if (w != SIZE_MINIMIZED) { Layout(); InvalidateRect(h, NULL, FALSE); } return 0;
    case WM_ERASEBKGND: { RECT rc; GetClientRect(h, &rc); FillRect((HDC)w, &rc, g_dark ? g_brDarkBg : g_brLightBg); return 1; }
    case WM_PAINT: { PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps); PaintPreview(dc, g_prev); EndPaint(h, &ps); return 0; }
    case WM_MEASUREITEM: { MEASUREITEMSTRUCT *mi = (MEASUREITEMSTRUCT *)l; if (mi->CtlID == IDC_LIST) { mi->itemHeight = 52; return TRUE; } break; }
    case WM_DRAWITEM: { DRAWITEMSTRUCT *d = (DRAWITEMSTRUCT *)l; if (d->CtlID == IDC_LIST) { DrawListItem(d); return TRUE; } break; }
    case WM_CTLCOLOREDIT: case WM_CTLCOLORLISTBOX:
        SetTextColor((HDC)w, g_dark ? RGB(235,235,235) : C_K);
        SetBkColor((HDC)w, g_dark ? RGB(45,45,48) : C_W);
        return (LRESULT)(g_dark ? g_brDarkEd : g_brLightEd);
    case WM_CTLCOLORSTATIC:
        SetTextColor((HDC)w, g_dark ? RGB(200,200,200) : RGB(40,40,40));
        SetBkColor((HDC)w, g_dark ? RGB(30,30,30) : RGB(243,243,243));
        return (LRESULT)(g_dark ? g_brDarkBg : g_brLightBg);
    case WM_TIMER:
        if (w == TM_REFRESH) Refresh(FALSE);
        else if (w == TM_SYNC) SyncAll();
        return 0;
    case WM_CONTEXTMENU:
        if ((HWND)w == g_list) {
            POINT p, cp; LRESULT r;
            if ((int)(short)LOWORD(l) == -1 && (int)(short)HIWORD(l) == -1) GetCursorPos(&p);
            else { p.x = (short)LOWORD(l); p.y = (short)HIWORD(l); }
            cp = p; ScreenToClient(g_list, &cp);
            r = SendMessageW(g_list, LB_ITEMFROMPOINT, 0, MAKELPARAM(cp.x, cp.y));
            if (HIWORD(r) == 0 && (int)LOWORD(r) < g_nwins) { SendMessageW(g_list, LB_SETCURSEL, LOWORD(r), 0); ShowContextMenu(p.x, p.y); }
            return 0;
        }
        break;
    case WM_COMMAND: {
        int id = LOWORD(w), code = HIWORD(w);
        if (id >= IDC_ACT + 1 && id <= IDC_ACT + 8) { DoAction(id - IDC_ACT, SelectedHwnd()); return 0; }
        switch (id) {
        case IDC_SEARCH: if (code == EN_CHANGE) Refresh(TRUE); break;
        case IDC_REFRESH: Refresh(TRUE); SetStatus(L"%d window(s).", g_nwins); break;
        case IDC_MPIN: g_mgrTop = !g_mgrTop; SetMgrTop(); break;
        case IDC_MODE: g_dark = !g_dark; InvalidateRect(h, NULL, TRUE); InvalidateRect(g_list, NULL, TRUE);
                       InvalidateRect(g_edSearch, NULL, TRUE); InvalidateRect(g_edRun, NULL, TRUE); break;
        case IDC_THEME: if (code == CBN_SELCHANGE) { int i = (int)SendMessageW(g_cbTheme, CB_GETCURSEL, 0, 0);
                            if (i >= 0) { g_themeIdx = i; ApplyLookAll(); } } break;
        case IDC_ICONS: if (code == CBN_SELCHANGE) { int i = (int)SendMessageW(g_cbIcon, CB_GETCURSEL, 0, 0);
                            if (i >= 0) { g_iconStyle = i; ApplyLookAll(); } } break;
        case IDC_COLOR: PickColor(); break;
        case IDC_RESET: g_hasCol = 0; ApplyLookAll(); SetStatus(L"Theme colours restored."); break;
        case IDC_BROWSE: DoBrowse(); break;
        case IDC_GO: DoRun(); break;
        case IDC_LIST:
            if (code == LBN_SELCHANGE) {            /* tap a row: restore + focus that window */
                HWND t = SelectedHwnd();
                if (t) { if (IsIconic(t)) ShowWindow(t, SW_RESTORE); SetForegroundWindow(t); }
            } else if (code == LBN_DBLCLK) {
                HWND t = SelectedHwnd();
                if (t) DoAction(SkinFind(t) ? ACT_UNSKIN : ACT_SKIN, t);
            }
            break;
        }
        if (id == IDC_SEARCH || id == IDC_RUN) break;
        return 0; }
    case WM_CLOSE:
        SaveConfig(); KillTimer(h, TM_SYNC); SkinRemoveAll(); DestroyWindow(h); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

/* ------------------------------------------------------------------ */
int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cmd, int show) {
    WNDCLASSEXW wc; MSG msg; HWND h; int sw, sh;
    (void)hp; (void)cmd; g_inst = hi;
    InitIniPath(); LoadConfig();

    g_fUi    = CreateFontW(-15, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    g_fSmall = CreateFontW(-12, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    g_fTip   = g_fSmall;
    g_fTitle[0] = CreateFontW(-12, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    g_fTitle[1] = CreateFontW(-12, 0, 0, 0, FW_BOLD,   0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    if (!g_fUi) g_fUi = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (!g_fSmall) g_fSmall = g_fTip = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (!g_fTitle[0]) g_fTitle[0] = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (!g_fTitle[1]) g_fTitle[1] = g_fTitle[0];
    g_brDarkBg = CreateSolidBrush(RGB(30,30,30));  g_brDarkEd = CreateSolidBrush(RGB(45,45,48));
    g_brLightBg = CreateSolidBrush(RGB(243,243,243)); g_brLightEd = CreateSolidBrush(C_W);

    memset(&wc, 0, sizeof wc); wc.cbSize = sizeof wc; wc.hInstance = hi; wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.lpfnWndProc = MainProc; wc.lpszClassName = L"WLCMain"; wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    if (!RegisterClassExW(&wc)) { MessageBoxW(NULL, L"RegisterClass failed", APP_TITLE, MB_ICONERROR); return 1; }
    wc.lpfnWndProc = SkinProc; wc.lpszClassName = L"WLCSkin"; wc.style = CS_DBLCLKS; wc.hIcon = NULL;
    if (!RegisterClassExW(&wc)) { MessageBoxW(NULL, L"RegisterClass failed", APP_TITLE, MB_ICONERROR); return 1; }
    wc.lpfnWndProc = TipProc; wc.lpszClassName = L"WLCTip"; wc.style = 0;
    RegisterClassExW(&wc);
    g_tip = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE, L"WLCTip", L"", WS_POPUP | WS_BORDER,
                            0, 0, 10, 10, NULL, NULL, hi, NULL);

    sw = GetSystemMetrics(SM_CXSCREEN); sh = GetSystemMetrics(SM_CYSCREEN);
    if (g_winW > sw) g_winW = sw;  if (g_winH > sh) g_winH = sh;
    if (g_winX != CW_USEDEFAULT && (g_winX < -50 || g_winX > sw - 80)) g_winX = CW_USEDEFAULT;
    if (g_winY != CW_USEDEFAULT && (g_winY < -10 || g_winY > sh - 80)) g_winY = CW_USEDEFAULT;
    h = CreateWindowExW(0, L"WLCMain", APP_TITLE, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                        g_winX, g_winY, g_winW, g_winH, NULL, NULL, hi, NULL);
    if (!h) { MessageBoxW(NULL, L"Could not create window", APP_TITLE, MB_ICONERROR); return 1; }
    ShowWindow(h, show); UpdateWindow(h);
    if (g_mgrTop) SetMgrTop();

    while (GetMessageW(&msg, NULL, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return (int)msg.wParam;
}

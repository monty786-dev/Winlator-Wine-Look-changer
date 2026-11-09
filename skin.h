/* skin.h - overlay title bars, z-order, pin, roll-up, auto-skin of all windows */
static HWND g_tip;
static void TipHide(void) { if (g_tip) ShowWindow(g_tip, SW_HIDE); }
static void TipShow(const wchar_t *text, int sx, int sy) {
    SIZE sz; HDC dc; HFONT of;
    if (!g_tip) return;
    dc = GetDC(g_tip); if (!dc) return;
    of = (HFONT)SelectObject(dc, g_fSmall);
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
        GetWindowTextW(h, b, 128); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, K_);
        SelectObject(dc, g_fSmall); rc.left += 5; rc.top += 2;
        DrawTextW(dc, b, -1, &rc, DT_SINGLELINE | DT_NOPREFIX);
        EndPaint(h, &ps); return 0;
    }
    if (m == WM_NCHITTEST) return HTTRANSPARENT;
    return DefWindowProcW(h, m, w, l);
}
static const wchar_t *BTN_TIPS[BT_COUNT] = { L"Close", L"Maximize / Restore", L"Minimize",
    L"Snap to right half", L"Snap to left half", L"Always on top (pin)", L"Roll up / Unroll" };

typedef struct Skin {
    HWND target, over; LONG origStyle; int H, state, hide;      /* state 0 normal, 1 maximised, 2 snapped */
    RECT restore; int hot, down;
    BOOL pin, pinned, active, dragging, shaded;
    POINT dragStart; RECT dragRect;
    wchar_t title[256]; HICON icon, exeIcon;
    DWORD iconTick, titleTick, pinTick; int lastW, lastH; HWND lastFg;
} Skin;
static Skin *g_skins[MAXSKIN];
static int g_nskins;

static Skin *SkinFind(HWND t) { int i; for (i = 0; i < g_nskins; i++) if (g_skins[i]->target == t) return g_skins[i]; return NULL; }

static HICON GetWinIcon(HWND t) {
    DWORD_PTR r = 0;
    if (SendMessageTimeoutW(t, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 40, &r) && r) return (HICON)r;
    r = GetClassLongPtrW(t, GCLP_HICONSM); if (r) return (HICON)r;
    r = GetClassLongPtrW(t, GCLP_HICON);   return (HICON)r;
}
static HICON ExeIcon(DWORD pid) {
    HICON ic = NULL; HANDLE ph = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (ph) {
        wchar_t full[MAX_PATH]; DWORD sz = MAX_PATH;
        if (QueryFullProcessImageNameW(ph, 0, full, &sz)) ExtractIconExW(full, 0, NULL, &ic, 1);
        CloseHandle(ph);
    }
    return ic;
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

/* Z-order fix: keep the overlay directly above its window, whatever window was just raised. */
static void SkinZ(Skin *e) {
    HWND prev = GetWindow(e->target, GW_HWNDPREV);
    if (prev == e->over) return;
    {   BOOL top = (GetWindowLongW(e->target, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
        SetWindowPos(e->over, prev ? prev : (top ? HWND_TOPMOST : HWND_TOP), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); }
}
static void SkinRaise(Skin *e) {
    if (e->shaded) { SetWindowPos(e->over, e->pin ? HWND_TOPMOST : HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); return; }
    SetWindowPos(e->target, e->pin ? HWND_TOPMOST : HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetForegroundWindow(e->target);
    SkinZ(e);
}

static void SkinShade(Skin *e, int on) {
    if (on == e->shaded) return;
    e->shaded = on;
    if (on) ShowWindow(e->target, SW_HIDE);
    else { ShowWindow(e->target, SW_SHOWNOACTIVATE); SkinRaise(e); }
    InvalidateRect(e->over, NULL, FALSE);
}
static void SkinToggleMax(Skin *e) {
    RECT wa; GetWork(&wa);
    if (e->shaded) SkinShade(e, 0);
    if (e->state == 1) {
        e->state = 0; SkinMove(e, e->restore.left, e->restore.top, e->restore.right - e->restore.left, e->restore.bottom - e->restore.top);
    } else {
        if (e->state == 0) GetWindowRect(e->target, &e->restore);
        e->state = 1; SkinMove(e, wa.left, wa.top + e->H, wa.right - wa.left, wa.bottom - wa.top - e->H);
    }
    InvalidateRect(e->over, NULL, FALSE);
}
static void SkinSnap(Skin *e, int right) {
    RECT wa; int hw, x, w; GetWork(&wa);
    if (e->shaded) SkinShade(e, 0);
    if (e->state == 0) GetWindowRect(e->target, &e->restore);
    e->state = 2; hw = (wa.right - wa.left) / 2;
    x = right ? wa.left + hw : wa.left; w = right ? (wa.right - wa.left - hw) : hw;
    SkinMove(e, x, wa.top + e->H, w, wa.bottom - wa.top - e->H);
    InvalidateRect(e->over, NULL, FALSE);
}
static void SetPin(HWND t, BOOL on) {
    Skin *e = SkinFind(t);
    SetWindowPos(t, on ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    if (e) { e->pin = on; e->pinned = on; SkinZ(e); InvalidateRect(e->over, NULL, FALSE); }
}
static BOOL IsPinned(HWND t) {
    Skin *e = SkinFind(t);
    return e ? e->pin : (GetWindowLongW(t, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
}
static void SkinPress(Skin *e, int id) {
    switch (id) {
        case BT_MIN:   if (e->shaded) SkinShade(e, 0); ShowWindow(e->target, SW_MINIMIZE); break;
        case BT_MAX:   SkinToggleMax(e); break;
        case BT_CLOSE: if (e->shaded) SkinShade(e, 0); PostMessageW(e->target, WM_CLOSE, 0, 0); break;
        case BT_SNAPL: SkinSnap(e, 0); break;
        case BT_SNAPR: SkinSnap(e, 1); break;
        case BT_PIN:   SetPin(e->target, !e->pin); break;
        case BT_SHADE: SkinShade(e, !e->shaded); break;
    }
}

static int ButtonAt(const Skin *e, int w, int x, int y) {
    RECT br[BT_COUNT]; int vis[BT_COUNT], i, tl, tr; POINT p; p.x = x; p.y = y;
    LayoutBtns(CurTheme(), w, br, vis, &tl, &tr, e->hide);
    for (i = 0; i < BT_COUNT; i++) if (vis[i] && PtInRect(&br[i], p)) return i;
    return -1;
}
static void SkinPaintDC(Skin *e, HDC dc, int w, int h) {
    PState s; memset(&s, 0, sizeof s);
    s.title = e->title; s.icon = e->icon ? e->icon : e->exeIcon; s.active = e->active; s.hot = e->hot; s.down = e->down;
    s.maxed = (e->state == 1); s.pinned = e->pin; s.shaded = e->shaded; s.hide = e->hide;
    PaintSkin(dc, w, h, CurTheme(), &s, g_ovFont ? g_ovFont : (HFONT)GetStockObject(DEFAULT_GUI_FONT));
}

static LRESULT CALLBACK SkinProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    Skin *e = (Skin *)GetWindowLongPtrW(h, GWLP_USERDATA);
    if (m == WM_NCCREATE) {
        SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCTW *)l)->lpCreateParams);
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
            HDC mem = CreateCompatibleDC(dc); HBITMAP bm = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
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
        int x = (short)LOWORD(l), y = (short)HIWORD(l), b; RECT rc; GetClientRect(h, &rc);
        if (e->dragging && (w & MK_LBUTTON)) {
            POINT p; int ny; GetCursorPos(&p);
            ny = e->dragRect.top + p.y - e->dragStart.y; if (ny < e->H) ny = e->H;
            SetWindowPos(e->target, NULL, e->dragRect.left + p.x - e->dragStart.x, ny, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        }
        b = ButtonAt(e, rc.right, x, y);
        if (b != e->hot) {
            TRACKMOUSEEVENT te; te.cbSize = sizeof te; te.dwFlags = TME_LEAVE; te.hwndTrack = h; te.dwHoverTime = 0;
            TrackMouseEvent(&te);
            e->hot = b; TipHide(); KillTimer(h, TM_TIP);
            if (b >= 0) SetTimer(h, TM_TIP, 500, NULL);
            InvalidateRect(h, NULL, FALSE);
        }
        return 0; }
    case WM_MOUSELEAVE: e->hot = -1; TipHide(); KillTimer(h, TM_TIP); InvalidateRect(h, NULL, FALSE); return 0;
    case WM_TIMER:
        if (w == TM_TIP) {
            KillTimer(h, TM_TIP);
            if (e->hot >= 0) {
                RECT br[BT_COUNT], rc; int vis[BT_COUNT], tl, tr; POINT p;
                GetClientRect(h, &rc); LayoutBtns(CurTheme(), rc.right, br, vis, &tl, &tr, e->hide);
                p.x = br[e->hot].left; p.y = rc.bottom + 4; ClientToScreen(h, &p);
                TipShow(BTN_TIPS[e->hot], p.x, p.y);
            }
        }
        return 0;
    case WM_LBUTTONDOWN: {
        int x = (short)LOWORD(l), y = (short)HIWORD(l), b; RECT rc; GetClientRect(h, &rc); TipHide();
        b = ButtonAt(e, rc.right, x, y);
        SkinRaise(e);                                   /* clicking the bar always brings window + bar to front */
        SetCapture(h);
        if (b >= 0) { e->down = b; InvalidateRect(h, NULL, FALSE); }
        else if (e->state != 1) {
            e->dragging = TRUE; GetCursorPos(&e->dragStart); GetWindowRect(e->target, &e->dragRect);
            if (e->state == 2) e->state = 0;
        }
        return 0; }
    case WM_LBUTTONUP: {
        int x = (short)LOWORD(l), y = (short)HIWORD(l), d = e->down, b; RECT rc; GetClientRect(h, &rc);
        e->dragging = FALSE; e->down = -1;
        if (GetCapture() == h) ReleaseCapture();
        b = ButtonAt(e, rc.right, x, y);
        InvalidateRect(h, NULL, FALSE);
        if (d >= 0 && d == b) SkinPress(e, d);
        return 0; }
    case WM_LBUTTONDBLCLK: {
        int x = (short)LOWORD(l), y = (short)HIWORD(l); RECT rc; GetClientRect(h, &rc);
        if (ButtonAt(e, rc.right, x, y) < 0) { if (CurTheme()->dbl == 1) SkinShade(e, !e->shaded); else if (e->hide & (1 << BT_MAX)) {} else SkinToggleMax(e); }
        return 0; }
    case WM_CAPTURECHANGED: e->dragging = FALSE; e->down = -1; return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static void SkinDestroy(Skin *e, BOOL restoreWin) {
    int i;
    if (restoreWin && IsWindow(e->target)) {
        RECT r; LONG cur;
        if (e->shaded) { ShowWindow(e->target, SW_SHOWNOACTIVATE); e->shaded = FALSE; }
        if (e->state != 0) SkinMove(e, e->restore.left, e->restore.top, e->restore.right - e->restore.left, e->restore.bottom - e->restore.top);
        cur = GetWindowLongW(e->target, GWL_STYLE);
        SetWindowLongW(e->target, GWL_STYLE, (cur & ~WS_CAPTION) | (e->origStyle & WS_CAPTION));
        SetWindowPos(e->target, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        if (GetWindowRect(e->target, &r))
            SetWindowPos(e->target, NULL, r.left, r.top - e->H, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (e->over) { TipHide(); DestroyWindow(e->over); }
    if (e->exeIcon) DestroyIcon(e->exeIcon);
    for (i = 0; i < g_nskins; i++) if (g_skins[i] == e) { g_skins[i] = g_skins[--g_nskins]; break; }
    free(e);
}

/* ---- never-skin lists ---- */
static HWND g_noSkin[256]; static int g_nNoSkin;
static BOOL IsNoSkin(HWND h) { int i; for (i = 0; i < g_nNoSkin; i++) if (g_noSkin[i] == h) return TRUE; return FALSE; }
static void AddNoSkin(HWND h) { if (!IsNoSkin(h)) { if (g_nNoSkin >= 256) g_nNoSkin = 0; g_noSkin[g_nNoSkin++] = h; } }

/* returns 0 ok, 1 no title bar, 2 limit, 3 already skinned, 4 create failed */
static int SkinApply(HWND t) {
    Skin *e; LONG st; RECT r0, r1; int H = CurTheme()->height, x, y, w; BOOL wasMax = FALSE; DWORD pid = 0;
    if (SkinFind(t)) return 3;
    if (g_nskins >= MAXSKIN) return 2;
    st = GetWindowLongW(t, GWL_STYLE);
    if ((st & WS_CAPTION) != WS_CAPTION) return 1;
    if (IsIconic(t)) ShowWindow(t, SW_RESTORE);
    if (IsZoomed(t)) { wasMax = TRUE; ShowWindow(t, SW_RESTORE); }
    if (!GetWindowRect(t, &r0)) return 1;
    e = (Skin *)calloc(1, sizeof(Skin)); if (!e) return 4;
    e->target = t; e->origStyle = st; e->H = H; e->hot = -1; e->down = -1; e->lastW = -1; e->lastH = -1;
    if (!(st & WS_MAXIMIZEBOX)) e->hide |= 1 << BT_MAX;
    if (!(st & WS_MINIMIZEBOX)) e->hide |= 1 << BT_MIN;
    if (!(st & WS_THICKFRAME)) e->hide |= (1 << BT_SNAPL) | (1 << BT_SNAPR);
    e->pin = (GetWindowLongW(t, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
    GetWindowTextW(t, e->title, 256);
    GetWindowThreadProcessId(t, &pid); e->exeIcon = ExeIcon(pid);
    SetWindowLongW(t, GWL_STYLE, st & ~WS_CAPTION);
    SetWindowPos(t, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    if (!GetWindowRect(t, &r1)) r1 = r0;
    SetWindowPos(t, NULL, r0.left, r0.top + H, r1.right - r1.left, r1.bottom - r1.top, SWP_NOZORDER | SWP_NOACTIVATE);
    if (!GetWindowRect(t, &r1)) r1 = r0;
    x = r1.left; y = r1.top - H; w = r1.right - r1.left;
    e->over = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, L"WLCSkin", L"", WS_POPUP, x, y, w, H, NULL, NULL, g_inst, e);
    if (!e->over) {
        SetWindowLongW(t, GWL_STYLE, st);
        SetWindowPos(t, NULL, r0.left, r0.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        if (e->exeIcon) DestroyIcon(e->exeIcon);
        free(e); return 4;
    }
    g_skins[g_nskins++] = e;
    SkinRegion(e, w);
    ShowWindow(e->over, SW_SHOWNOACTIVATE);
    SkinZ(e);
    if (wasMax) { RECT wa; GetWork(&wa); GetWindowRect(t, &e->restore); e->state = 1;
                  SkinMove(e, wa.left, wa.top + H, wa.right - wa.left, wa.bottom - wa.top - H); }
    return 0;
}

static void SkinSync(Skin *e) {
    RECT tr, orc; HWND fg; BOOL act, hidden; int w, x, y; DWORD now = GetTickCount();
    if (!IsWindow(e->target)) { SkinDestroy(e, FALSE); return; }
    hidden = e->shaded ? FALSE : (IsIconic(e->target) || !IsWindowVisible(e->target));
    if (hidden) { if (IsWindowVisible(e->over)) ShowWindow(e->over, SW_HIDE); return; }
    if (!GetWindowRect(e->target, &tr)) return;
    w = tr.right - tr.left; x = tr.left; y = tr.top - e->H;
    if (!IsWindowVisible(e->over)) { ShowWindow(e->over, SW_SHOWNOACTIVATE); InvalidateRect(e->over, NULL, FALSE); }
    if (!GetWindowRect(e->over, &orc) || orc.left != x || orc.top != y || orc.right - orc.left != w || orc.bottom - orc.top != e->H) {
        SetWindowPos(e->over, NULL, x, y, w, e->H, SWP_NOACTIVATE | SWP_NOZORDER);
        InvalidateRect(e->over, NULL, FALSE);
    }
    if (e->lastW != w || e->lastH != e->H) { SkinRegion(e, w); e->lastW = w; e->lastH = e->H; }
    SkinZ(e);
    fg = GetForegroundWindow();
    act = (fg == e->target || fg == e->over || (fg && GetWindow(fg, GW_OWNER) == e->target));
    if (act != e->active) { e->active = act; InvalidateRect(e->over, NULL, FALSE); }
    if (e->pin) {          /* pin: keep topmost; re-assert once whenever focus moves to another window */
        if (!(GetWindowLongW(e->target, GWL_EXSTYLE) & WS_EX_TOPMOST) || (fg != e->lastFg && now - e->pinTick > 150)) {
            SetWindowPos(e->target, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            e->pinTick = now; SkinZ(e);
        }
    }
    e->lastFg = fg;
    if (now - e->titleTick > 300) {
        wchar_t t[256]; e->titleTick = now; t[0] = 0; GetWindowTextW(e->target, t, 256);
        if (wcscmp(t, e->title) != 0) { wcscpy(e->title, t); InvalidateRect(e->over, NULL, FALSE); }
        if (now - e->iconTick > 2000) { HICON ic = GetWinIcon(e->target); if (ic != e->icon) { e->icon = ic; InvalidateRect(e->over, NULL, FALSE); } e->iconTick = now; }
    }
}
static void SyncAll(void) { int i; for (i = g_nskins - 1; i >= 0; i--) if (i < g_nskins) SkinSync(g_skins[i]); }
static void SkinRemoveAll(void) { while (g_nskins > 0) SkinDestroy(g_skins[g_nskins - 1], TRUE); }

/* ---- process names (cached) + auto-skin of every running and future window ---- */
typedef struct { DWORD pid; wchar_t name[40]; } PidName;
static PidName g_pn[128]; static int g_npn;
static void ProcNameCached(DWORD pid, wchar_t *out, int n) {
    int i; HANDLE ph;
    for (i = 0; i < g_npn; i++) if (g_pn[i].pid == pid) { wcsncpy(out, g_pn[i].name, n - 1); out[n - 1] = 0; return; }
    out[0] = 0; ph = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (ph) {
        wchar_t full[MAX_PATH]; DWORD sz = MAX_PATH;
        if (QueryFullProcessImageNameW(ph, 0, full, &sz)) { wchar_t *b = wcsrchr(full, L'\\'); wcsncpy(out, b ? b + 1 : full, n - 1); out[n - 1] = 0; }
        CloseHandle(ph);
    }
    if (!out[0]) _snwprintf(out, n - 1, L"pid %lu", (unsigned long)pid);
    if (g_npn >= 128) g_npn = 0;
    g_pn[g_npn].pid = pid; wcsncpy(g_pn[g_npn].name, out, 39); g_pn[g_npn].name[39] = 0; g_npn++;
}
static BOOL IsExcludedExe(const wchar_t *exe) {
    wchar_t k[64]; int i;
    _snwprintf(k, 62, L";%ls;", exe); k[62] = 0;
    for (i = 0; k[i]; i++) k[i] = (wchar_t)towlower(k[i]);
    return wcsstr(g_exclude, k) != NULL;
}
static void AddExclude(const wchar_t *exe) {
    wchar_t k[64]; int i;
    if (IsExcludedExe(exe)) return;
    _snwprintf(k, 62, L"%ls;", exe); k[62] = 0; for (i = 0; k[i]; i++) k[i] = (wchar_t)towlower(k[i]);
    if (wcslen(g_exclude) + wcslen(k) < 500) wcscat(g_exclude, k);
}
static BOOL Eligible(HWND h) {
    DWORD pid; LONG st, ex; RECT r; wchar_t cls[64], exe[64];
    if (!IsWindowVisible(h) || IsIconic(h)) return FALSE;
    st = GetWindowLongW(h, GWL_STYLE);
    if ((st & WS_CAPTION) != WS_CAPTION || (st & WS_CHILD)) return FALSE;
    ex = GetWindowLongW(h, GWL_EXSTYLE);
    if ((ex & WS_EX_TOOLWINDOW) && !(ex & WS_EX_APPWINDOW)) return FALSE;
    if (SkinFind(h) || IsNoSkin(h)) return FALSE;
    if (!GetWindowRect(h, &r) || r.right - r.left < 80 || r.bottom - r.top < 40) return FALSE;
    if (GetWindowTextLengthW(h) <= 0) return FALSE;
    GetWindowThreadProcessId(h, &pid); if (pid == GetCurrentProcessId()) return FALSE;
    cls[0] = 0; GetClassNameW(h, cls, 64);
    if (!wcscmp(cls, L"Progman") || !wcscmp(cls, L"Shell_TrayWnd") || !wcscmp(cls, L"WorkerW") || !wcscmp(cls, L"#32768")) return FALSE;
    ProcNameCached(pid, exe, 64);
    if (IsExcludedExe(exe)) return FALSE;
    return TRUE;
}
static HWND g_new[8]; static int g_nnew;
static BOOL CALLBACK ScanProc(HWND h, LPARAM lp) {
    (void)lp; if (g_nnew < 8 && Eligible(h)) g_new[g_nnew++] = h; return TRUE;
}
static void AutoScan(void) {
    int i; g_nnew = 0; EnumWindows(ScanProc, 0);
    for (i = 0; i < g_nnew; i++) { int r = SkinApply(g_new[i]); if (r != 0 && r != 3) AddNoSkin(g_new[i]); }
}

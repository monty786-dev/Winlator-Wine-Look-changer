/* mdesk.h - desktop mode: wallpaper, shortcut icons on a grid, clock/apps widgets */
typedef struct { wchar_t name[64]; wchar_t path[MAX_PATH]; int x, y; } DItem;
static DItem g_di[160]; static int g_ndi, g_dSel = -1, g_dDown = -1, g_dMoved, g_dOffX, g_dOffY; static POINT g_dDP;
static HBITMAP g_wallBmp; static int g_wallW, g_wallH; static wchar_t g_wallPath[MAX_PATH];

static int DCell(void) { return ScaleI(g.dkGrid); }
static void DArea(RECT *r) { GetWork(r); }
static void DAddFile(const wchar_t *dir, const wchar_t *name) {
    DItem *d; size_t n = wcslen(name); int i;
    if (g_ndi >= 160 || name[0] == L'.' || !_wcsicmp(name, L"desktop.ini")) return;
    d = &g_di[g_ndi]; memset(d, 0, sizeof *d);
    _snwprintf(d->path, MAX_PATH - 1, L"%ls\\%ls", dir, name); d->path[MAX_PATH - 1] = 0;
    for (i = 0; i < g_ndi; i++) if (!_wcsicmp(g_di[i].path, d->path)) return;
    BaseName(d->path, d->name, 64); if (n > 4 && !_wcsicmp(name + n - 4, L".lnk")) {} d->x = d->y = -1; g_ndi++;
}
/* corner-aware auto placement: fills columns starting at the chosen corner */
static void DPlace(DItem *d, int idx) {
    RECT a; int cell = DCell(), cols, rows, c, r, k, i, ok; DArea(&a);
    cols = (a.right - a.left) / cell; rows = (a.bottom - a.top) / cell; if (cols < 1) cols = 1; if (rows < 1) rows = 1;
    for (k = 0; k < cols * rows; k++) {
        int col = k / rows, row = k % rows;
        c = (g.dkCorner & 1) ? cols - 1 - col : col; r = (g.dkCorner & 2) ? rows - 1 - row : row;
        ok = 1; for (i = 0; i < g_ndi && ok; i++) if (i != idx && g_di[i].x == a.left + c * cell && g_di[i].y == a.top + r * cell) ok = 0;
        if (ok) { d->x = a.left + c * cell; d->y = a.top + r * cell; return; }
    }
    d->x = a.left; d->y = a.top;
}
static void DScan(void) {
    wchar_t dir[MAX_PATH], pat[MAX_PATH], v[40]; WIN32_FIND_DATAW fd; HANDLE h; int i, pass; DWORD n;
    g_ndi = 0;
    for (pass = 0; pass < 2; pass++) {
        n = GetEnvironmentVariableW(pass ? L"PUBLIC" : L"USERPROFILE", dir, MAX_PATH);
        if (!n || n > MAX_PATH - 20) continue;
        wcscat(dir, L"\\Desktop"); _snwprintf(pat, MAX_PATH - 1, L"%ls\\*", dir); pat[MAX_PATH - 1] = 0;
        h = FindFirstFileW(pat, &fd);
        if (h != INVALID_HANDLE_VALUE) { do DAddFile(dir, fd.cFileName); while (FindNextFileW(h, &fd)); FindClose(h); }
    }
    for (i = 0; i < g_ndi; i++) {
        GetPrivateProfileStringW(L"desk", g_di[i].path, L"", v, 40, g_mini);
        if (v[0]) { g_di[i].x = _wtoi(v); { wchar_t *c = wcschr(v, L','); if (c) g_di[i].y = _wtoi(c + 1); } }
    }
    for (i = 0; i < g_ndi; i++) if (g_di[i].x < 0) DPlace(&g_di[i], i);
}
static void DSave(int i) { wchar_t v[40]; _snwprintf(v, 39, L"%d,%d", g_di[i].x, g_di[i].y); v[39] = 0; WritePrivateProfileStringW(L"desk", g_di[i].path, v, g_mini); }
static int DHit(int x, int y) {
    int i, cell = DCell(); for (i = g_ndi - 1; i >= 0; i--) if (x >= g_di[i].x && x < g_di[i].x + cell && y >= g_di[i].y && y < g_di[i].y + cell) return i;
    return -1;
}
static void DWidgetRect(int which, RECT *r) {
    int sw = GetSystemMetrics(SM_CXSCREEN), w = ScaleI(which == 0 ? 190 : 230), h = ScaleI(which == 0 ? 96 : 70), x = (g.dkCorner & 1) ? 16 : sw - w - 16, y = 16 + (which ? ScaleI(110) : 0);
    SetRect(r, x, y, x + w, y + h);
}
static void DPaint(HDC dc, int w, int h) {
    RECT all, a, r, q; int i, cell = DCell(), isz = cell / 2 > ScaleI(48) ? ScaleI(48) : cell / 2; wchar_t buf[64]; COLORREF tc, sh;
    SetRect(&all, 0, 0, w, h);
    if (wcscmp(g_wallPath, S_wall) != 0) { wcscpy(g_wallPath, S_wall); if (g_wallBmp) { DeleteObject(g_wallBmp); g_wallBmp = NULL; } if (S_wall[0]) g_wallBmp = LoadPng(S_wall, &g_wallW, &g_wallH); }
    FillRectC(dc, 0, 0, w, h, (COLORREF)g.dkBgCol);
    if (g_wallBmp) DrawBmp(dc, g_wallBmp, g_wallW, g_wallH, &all, g.dkWallMode);
    tc = g_wallBmp ? W_ : (Lum((COLORREF)g.dkBgCol) < 140 ? W_ : K_); sh = tc == W_ ? K_ : W_;
    SelectObject(dc, g_fTb); SetBkMode(dc, TRANSPARENT);
    if (g.dkIcons) for (i = 0; i < g_ndi; i++) {
        SetRect(&r, g_di[i].x, g_di[i].y, g_di[i].x + cell, g_di[i].y + cell);
        if (i == g_dSel) RRect(dc, &r, ScaleI(g.pRound ? 14 : 4), Blend(P.acc, K_, 20), P.acc);
        DrawIc(dc, PathIcon(g_di[i].path), (r.left + r.right) / 2, r.top + cell / 2 - ScaleI(8), isz);
        q = r; q.top = r.top + cell / 2 + isz / 2 - ScaleI(6); q.left += 2; q.right -= 2;
        { RECT s2 = q; OffsetRect(&s2, 1, 1); SetTextColor(dc, sh); DrawTextW(dc, g_di[i].name, -1, &s2, DT_CENTER | DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX); }
        SetTextColor(dc, tc); DrawTextW(dc, g_di[i].name, -1, &q, DT_CENTER | DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX);
    }
    if (g.dkClock) {
        SYSTEMTIME st; wchar_t tm[24], dt[40]; GetLocalTime(&st); DWidgetRect(0, &r);
        GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, tm, 24); GetDateFormatW(LOCALE_USER_DEFAULT, 0, &st, L"dddd, MMM d", dt, 40);
        RRect(dc, &r, ScaleI(14), Blend(P.barA, K_, 10), P.line);
        { HFONT big = MakeFont(S_font, ScaleI(34), 1), of = (HFONT)SelectObject(dc, big); q = r; q.bottom = r.top + (r.bottom - r.top) * 60 / 100; Txt(dc, tm, &q, DT_SINGLELINE | DT_VCENTER | DT_CENTER, P.txt); SelectObject(dc, of); DeleteObject(big); }
        q = r; q.top = r.top + (r.bottom - r.top) * 58 / 100; Txt(dc, dt, &q, DT_SINGLELINE | DT_VCENTER | DT_CENTER, P.dim);
    }
    if (g.dkApps) {
        DWidgetRect(1, &r); RRect(dc, &r, ScaleI(14), Blend(P.barA, K_, 10), P.line);
        for (i = 0; i < g_nL[L_FAV] && i < 5; i++) DrawIc(dc, PathIcon(g_L[L_FAV][i].p), r.left + ScaleI(30) + i * ScaleI(42), (r.top + r.bottom) / 2, ScaleI(30));
        if (!g_nL[L_FAV]) { q = r; Txt(dc, L"Favourites appear here", &q, DT_SINGLELINE | DT_VCENTER | DT_CENTER, P.dim); }
    }
    (void)buf; (void)a; (void)h; (void)w;
}
static void DesktopLayoutAll(void) {
    int i; DScan(); (void)i;
    if (g_dk) InvalidateRect(g_dk, NULL, FALSE);
}
static LRESULT CALLBACK DkProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_ERASEBKGND: return 1;
    case WM_MOUSEACTIVATE: return MA_NOACTIVATE;
    case WM_PAINT: {
        PAINTSTRUCT ps; RECT rc; HDC dc = BeginPaint(h, &ps), mem; HBITMAP bm, ob; GetClientRect(h, &rc);
        mem = CreateCompatibleDC(dc); bm = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
        if (mem && bm) { ob = (HBITMAP)SelectObject(mem, bm); DPaint(mem, rc.right, rc.bottom); BitBlt(dc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY); SelectObject(mem, ob); }
        if (bm) DeleteObject(bm); if (mem) DeleteDC(mem);
        EndPaint(h, &ps); return 0; }
    case WM_LBUTTONDOWN: {
        int x = (short)LOWORD(l), y = (short)HIWORD(l), i = DHit(x, y); StartHide();
        g_dSel = i; g_dDown = i; g_dMoved = 0; g_dDP.x = x; g_dDP.y = y; if (i >= 0) { g_dOffX = x - g_di[i].x; g_dOffY = y - g_di[i].y; SetCapture(h); }
        InvalidateRect(h, NULL, FALSE); return 0; }
    case WM_MOUSEMOVE:
        if ((w & MK_LBUTTON) && g_dDown >= 0 && !g.dkLock) {
            int x = (short)LOWORD(l), y = (short)HIWORD(l);
            if ((x - g_dDP.x) * (x - g_dDP.x) + (y - g_dDP.y) * (y - g_dDP.y) > 36) g_dMoved = 1;
            if (g_dMoved) { g_di[g_dDown].x = x - g_dOffX; g_di[g_dDown].y = y - g_dOffY; InvalidateRect(h, NULL, FALSE); }
        }
        return 0;
    case WM_LBUTTONUP:
        if (GetCapture() == h) ReleaseCapture();
        if (g_dDown >= 0 && g_dMoved) {
            DItem *d = &g_di[g_dDown]; RECT a; int cell = DCell(); DArea(&a);
            if (g.dkSnap) { d->x = a.left + (d->x - a.left + cell / 2) / cell * cell; d->y = a.top + (d->y - a.top + cell / 2) / cell * cell; if (d->x < a.left) d->x = a.left; if (d->y < a.top) d->y = a.top; }
            DSave(g_dDown); InvalidateRect(h, NULL, FALSE);
        }
        g_dDown = -1; return 0;
    case WM_LBUTTONDBLCLK: { int i = DHit((short)LOWORD(l), (short)HIWORD(l)); if (i >= 0) LaunchPath(g_di[i].path); return 0; }
    case WM_RBUTTONUP: {
        HMENU mn = CreatePopupMenu(); POINT pt; int c, i = DHit((short)LOWORD(l), (short)HIWORD(l)); GetCursorPos(&pt);
        if (i >= 0) AppendMenuW(mn, MF_STRING, 1, L"Open");
        AppendMenuW(mn, MF_STRING, 2, L"Refresh"); AppendMenuW(mn, MF_STRING, 3, L"Change wallpaper..."); AppendMenuW(mn, MF_STRING, 4, L"Reset icon positions");
        AppendMenuW(mn, MF_STRING, 5, L"Personalize / Settings"); AppendMenuW(mn, MF_STRING, 6, L"Turn desktop mode off");
        SetForegroundWindow(h); c = TrackPopupMenu(mn, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, h, NULL); DestroyMenu(mn);
        if (c == 1) LaunchPath(g_di[i].path); else if (c == 2) DesktopLayoutAll();
        else if (c == 3) { wchar_t f[MAX_PATH]; if (MBrowse(h, f, L"Images\0*.png;*.jpg;*.jpeg;*.bmp;*.gif\0All files\0*.*\0")) { wcscpy(S_wall, f); MWrS(L"wall", S_wall); InvalidateRect(h, NULL, FALSE); } }
        else if (c == 4) { WritePrivateProfileStringW(L"desk", NULL, NULL, g_mini); DesktopLayoutAll(); }
        else if (c == 5) OpenSettings(); else if (c == 6) { g.dkOn = 0; MWr(L"cfg", L"dkOn", 0); DesktopToggle(); }
        return 0; }
    }
    return DefWindowProcW(h, m, w, l);
}
static void DesktopToggle(void) {
    if (!g_dk) return;
    if (g.dkOn) { int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN); DesktopLayoutAll(); SetWindowPos(g_dk, HWND_BOTTOM, 0, 0, sw, sh, SWP_NOACTIVATE | SWP_SHOWWINDOW); InvalidateRect(g_dk, NULL, FALSE); }
    else ShowWindow(g_dk, SW_HIDE);
}

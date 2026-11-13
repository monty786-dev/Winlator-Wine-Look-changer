/* mst2.h - start panel: window, input, menus */
static BOOL MBrowse(HWND owner, wchar_t *f, const wchar_t *filter) {
    OPENFILENAMEW o; memset(&o, 0, sizeof o); f[0] = 0; o.lStructSize = sizeof o; o.hwndOwner = owner;
    o.lpstrFilter = filter; o.lpstrFile = f; o.nMaxFile = MAX_PATH; o.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    return GetOpenFileNameW(&o);
}
static int g_stDK, g_stDI, g_stMoved, g_stScroll0, g_stDragMode, g_stOffX, g_stOffY; static POINT g_stDP;
static BOOL PickColor(COLORREF *io) {
    static COLORREF cust[16]; CHOOSECOLORW cc; HWND own = GetActiveWindow();
    memset(&cc, 0, sizeof cc); cc.lStructSize = sizeof cc; cc.hwndOwner = own; cc.rgbResult = *io; cc.lpCustColors = cust; cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    return ChooseColorW(&cc) ? (*io = cc.rgbResult, TRUE) : FALSE;
}
enum { HK_VB = 1, HK_APP, HK_TILE, HK_ADD, HK_FILT, HK_USER, HK_CORNER };

static int StHit(int x, int y, int *idx) {
    POINT p; int i; p.x = x; p.y = y; *idx = -1;
    for (i = 0; i < 7; i++) if (PtInRect(&rVb[i], p)) { *idx = i; return HK_VB; }
    if (PtInRect(&rAdd, p)) return HK_ADD;
    if (PtInRect(&rFilt, p)) return HK_FILT;
    if (PtInRect(&rHead, p)) return HK_USER;
    if (rTiles.right > rTiles.left && PtInRect(&rTiles, p)) {
        for (i = g_nL[L_TILE] - 1; i >= 0; i--) { RECT r; TileRect(i, &r); if (PtInRect(&r, p)) { *idx = i; return (x > r.right - 16 && y > r.bottom - 16) ? HK_CORNER : HK_TILE; } }
    }
    if (rApps.right > rApps.left && PtInRect(&rApps, p)) { i = (y - rApps.top + g_stScroll) / RowH(); if (i >= 0 && i < g_stNvis) { *idx = i; return HK_APP; } }
    return 0;
}
static void StPosition(void) {
    int w, h, x, y, sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN), i; RECT ir, wa; GetWork(&wa); StSize(&w, &h);
    SetRect(&ir, g_tbRect.left, g_tbRect.top, g_tbRect.left + 40, g_tbRect.bottom);
    for (i = 0; i < g_nit; i++) if (g_it[i].type == IT_START) { ItemRect(i, &ir); OffsetRect(&ir, g_tbRect.left, g_tbRect.top); break; }
    if (g.stSize == 3) { x = wa.left; y = wa.top; }
    else if (g.tbPos == 0 || g.tbPos == 1) {
        x = g.tbAlign == 2 ? ir.right - w : ir.left; y = g.tbPos == 0 ? g_tbRect.top - h - 4 : g_tbRect.bottom + 4;
    } else { y = ir.top; x = g.tbPos == 2 ? g_tbRect.right + 4 : g_tbRect.left - w - 4; }
    if (x + w > sw) x = sw - w; if (y + h > sh) y = sh - h; if (x < 0) x = 0; if (y < 0) y = 0;
    SetWindowPos(g_st, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE);
    g_stW = w; g_stH = h; StLayout(w, h);
    MoveWindow(g_stEdit, rEdit.left, rEdit.top, rEdit.right - rEdit.left, rEdit.bottom - rEdit.top, TRUE);
    SetLayeredWindowAttributes(g_st, 0, (BYTE)(g.stOpacity * 255 / 100), LWA_ALPHA);
    if (g.pRound && g.stSize != 3) RoundRgn(g_st, w, h, ScaleI(12)); else SetWindowRgn(g_st, NULL, TRUE);
}
static void StartHide(void) { if (g_st && IsWindowVisible(g_st)) ShowWindow(g_st, SW_HIDE); g_stOpen = 0; g_stMenu = 0; if (g_tb) InvalidateRect(g_tb, NULL, FALSE); }
static void StartShow(void) {
    if (!g_st) return;
    if (g_stDirty || g_napps == 0) StReload(); else StBuildVisible();
    SendMessageW(g_stEdit, WM_SETFONT, (WPARAM)g_fTb, TRUE);
    StPosition(); ShowWindow(g_st, SW_SHOW); SetForegroundWindow(g_st); InvalidateRect(g_st, NULL, FALSE);
    g_stOpen = 1; if (g_tb) InvalidateRect(g_tb, NULL, FALSE);
}
static void StartToggle(void) { if (g_stOpen) StartHide(); else StartShow(); }

static void AddFlow(void) {
    HMENU mn = CreatePopupMenu(); POINT pt; int c; wchar_t f[MAX_PATH]; GetCursorPos(&pt);
    AppendMenuW(mn, MF_STRING, 1, L"Add to Start app list"); AppendMenuW(mn, MF_STRING, 2, L"Add pinned tile");
    AppendMenuW(mn, MF_STRING, 3, L"Add to taskbar"); AppendMenuW(mn, MF_STRING, 4, L"Add to app tray");
    g_stMenu++; c = TrackPopupMenu(mn, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, g_st, NULL); DestroyMenu(mn);
    if (c) {
        if (MBrowse(g_st, f, L"Programs and shortcuts\0*.exe;*.lnk;*.bat\0All files\0*.*\0")) {
            ListAdd(c == 1 ? L_FAV : (c == 2 ? L_TILE : (c == 3 ? L_TB : L_TRAY)), f);
            g_stDirty = 1; ApplyAll(); if (g_stOpen) { StReload(); InvalidateRect(g_st, NULL, FALSE); }
        }
    }
    g_stMenu--;
}
static void PowerMenu(void) {
    HMENU mn = CreatePopupMenu(); POINT pt; int c; GetCursorPos(&pt);
    AppendMenuW(mn, MF_STRING, 1, L"Shut down Wine"); AppendMenuW(mn, MF_STRING, 2, L"Restart Wine");
    AppendMenuW(mn, MF_STRING, 3, L"Restart shell"); AppendMenuW(mn, MF_STRING, 4, L"Exit Monty shell");
    g_stMenu++; c = TrackPopupMenu(mn, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, 0, g_st, NULL); DestroyMenu(mn); g_stMenu--;
    if (c == 1) ShellExecuteW(NULL, L"open", L"wineboot.exe", L"-s", NULL, SW_HIDE);
    else if (c == 2) ShellExecuteW(NULL, L"open", L"wineboot.exe", L"-r", NULL, SW_HIDE);
    else if (c == 3) { wchar_t me[MAX_PATH]; GetModuleFileNameW(NULL, me, MAX_PATH); ShellExecuteW(NULL, L"open", me, L"/restart", NULL, SW_SHOWNORMAL); QuitShell(); }
    else if (c == 4) QuitShell();
}
static void VbarAction(int i) {
    switch (i) {
    case 0: g.showApps = !g.showApps; if (!g.showApps && !g.showTiles) g.showTiles = 1; MWr(L"cfg", L"showApps", g.showApps); MWr(L"cfg", L"showTiles", g.showTiles); break;
    case 1: g.showTiles = !g.showTiles; if (!g.showApps && !g.showTiles) g.showApps = 1; MWr(L"cfg", L"showApps", g.showApps); MWr(L"cfg", L"showTiles", g.showTiles); break;
    case 2: g_stFilter = 0; g.showApps = 1; StBuildVisible(); break;
    case 3: g_stFilter = 4; g.showApps = 1; StBuildVisible(); break;
    case 4: {
        wchar_t f[MAX_PATH]; GetWindowTextW(g_stEdit, f, MAX_PATH);
        if (f[0]) { if ((INT_PTR)ShellExecuteW(NULL, L"open", f, NULL, NULL, SW_SHOWNORMAL) > 32) StartHide(); }
        else { g_stMenu++; if (MBrowse(g_st, f, L"Programs\0*.exe;*.bat;*.lnk\0All files\0*.*\0")) { LaunchPath(f); StartHide(); } g_stMenu--; }
        return; }
    case 5: StartHide(); OpenSettings(); return;
    case 6: PowerMenu(); return;
    }
    InvalidateRect(g_st, NULL, FALSE);
}
static void AppMenu(int vi) {
    const AppEnt *a = &g_apps[g_stVis[vi]]; HMENU mn = CreatePopupMenu(); POINT pt; int c; GetCursorPos(&pt);
    AppendMenuW(mn, MF_STRING, 1, L"Open"); AppendMenuW(mn, MF_STRING, 2, L"Pin to Start tiles"); AppendMenuW(mn, MF_STRING, 3, L"Pin to taskbar");
    AppendMenuW(mn, MF_STRING, 4, L"Add to app tray"); AppendMenuW(mn, MF_STRING | (ListFind(L_FAV, a->path) >= 0 ? MF_CHECKED : 0), 5, L"Favourite");
    AppendMenuW(mn, MF_STRING, 6, L"Remove from list");
    g_stMenu++; c = TrackPopupMenu(mn, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, g_st, NULL); DestroyMenu(mn); g_stMenu--;
    if (c == 1) { LaunchPath(a->path); StartHide(); return; }
    if (c == 2) ListAdd(L_TILE, a->path); else if (c == 3) ListAdd(L_TB, a->path); else if (c == 4) ListAdd(L_TRAY, a->path);
    else if (c == 5) { int k = ListFind(L_FAV, a->path); if (k >= 0) ListRemove(L_FAV, k); else ListAdd(L_FAV, a->path); }
    else if (c == 6) ListAdd(L_HIDE, a->path);
    if (c) { StBuildVisible(); ApplyAll(); InvalidateRect(g_st, NULL, FALSE); }
}
static void TileMenu(int ti) {
    HMENU mn = CreatePopupMenu(); POINT pt; int c; LItem *t = &g_L[L_TILE][ti]; GetCursorPos(&pt);
    AppendMenuW(mn, MF_STRING, 1, L"Open"); AppendMenuW(mn, MF_STRING, 2, L"Small (1x1)"); AppendMenuW(mn, MF_STRING, 3, L"Medium (2x2)");
    AppendMenuW(mn, MF_STRING, 4, L"Wide (4x2)"); AppendMenuW(mn, MF_STRING, 5, L"Large (4x4)"); AppendMenuW(mn, MF_STRING, 6, L"Tile colour...");
    AppendMenuW(mn, MF_STRING, 7, L"Auto colour"); AppendMenuW(mn, MF_STRING, 8, L"Unpin");
    g_stMenu++; c = TrackPopupMenu(mn, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, g_st, NULL); DestroyMenu(mn);
    if (c == 1) { LaunchPath(t->p); StartHide(); }
    else if (c >= 2 && c <= 5) { static const int W2[4] = { 1, 2, 4, 4 }, H2[4] = { 1, 2, 2, 4 }; t->w = W2[c - 2]; t->h = H2[c - 2]; SaveList(L_TILE); }
    else if (c == 6) { COLORREF col = t->col ? t->col : P.acc; if (PickColor(&col)) { t->col = col ? col : RGB(1,1,1); SaveList(L_TILE); } }
    else if (c == 7) { t->col = 0; SaveList(L_TILE); }
    else if (c == 8) { ListRemove(L_TILE, ti); }
    g_stMenu--; if (g_st) InvalidateRect(g_st, NULL, FALSE);
}
static void StAnimTick(void) {
    int i, moving = 0;
    for (i = 0; i < g_nL[L_TILE]; i++) {
        int dx, dy; if (i == g_stDragTile) continue;
        dx = g_ttx[i] - g_tcx[i]; dy = g_tty[i] - g_tcy[i];
        if (dx > -3 && dx < 3 && dy > -3 && dy < 3) { g_tcx[i] = g_ttx[i]; g_tcy[i] = g_tty[i]; continue; }
        g_tcx[i] += dx * 35 / 100 + (dx > 0 ? 1 : -1); g_tcy[i] += dy * 35 / 100 + (dy > 0 ? 1 : -1); moving = 1;
    }
    InvalidateRect(g_st, &rTiles, FALSE);
    if (!moving) { KillTimer(g_st, 1); g_stAnimOn = 0; }
}
static void StKick(void) { if (g.stTileAnim && !g_stAnimOn) { g_stAnimOn = 1; SetTimer(g_st, 1, 16, NULL); } }

static LRESULT CALLBACK StProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps; RECT rc; HDC dc = BeginPaint(h, &ps), mem; HBITMAP bm, ob; GetClientRect(h, &rc);
        mem = CreateCompatibleDC(dc); bm = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
        if (mem && bm) { ob = (HBITMAP)SelectObject(mem, bm); StPaint(mem, rc.right, rc.bottom); BitBlt(dc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY); SelectObject(mem, ob); }
        if (bm) DeleteObject(bm); if (mem) DeleteDC(mem);
        EndPaint(h, &ps); StKick(); return 0; }
    case WM_ACTIVATE: if (LOWORD(w) == WA_INACTIVE && !g_stMenu && !g_stDragMode) StartHide(); return 0;
    case WM_TIMER: if (w == 1) StAnimTick(); return 0;
    case WM_CTLCOLOREDIT: {
        static HBRUSH br; static COLORREF last = (COLORREF)-1; COLORREF c = Blend(P.panel, P.panelTxt, 10);
        if (c != last) { if (br) DeleteObject(br); br = CreateSolidBrush(c); last = c; }
        SetTextColor((HDC)w, P.panelTxt); SetBkColor((HDC)w, c); return (LRESULT)br; }
    case WM_COMMAND:
        if (LOWORD(w) == 1 && HIWORD(w) == EN_CHANGE) { GetWindowTextW(g_stEdit, g_stSearch, 64); g_stScroll = 0; StBuildVisible(); InvalidateRect(h, &rApps, FALSE); }
        return 0;
    case WM_MOUSEWHEEL: { int d = (short)HIWORD(w) / 120; g_stScroll -= d * RowH() * 2; if (g_stScroll < 0) g_stScroll = 0; { int mx = g_stNvis * RowH() - (rApps.bottom - rApps.top); if (mx < 0) mx = 0; if (g_stScroll > mx) g_stScroll = mx; } InvalidateRect(h, &rApps, FALSE); return 0; }
    case WM_LBUTTONDOWN: {
        int idx, k = StHit((short)LOWORD(l), (short)HIWORD(l), &idx); SetCapture(h);
        g_stDK = k; g_stDI = idx; g_stDP.x = (short)LOWORD(l); g_stDP.y = (short)HIWORD(l); g_stMoved = 0; g_stScroll0 = g_stScroll; g_stDragMode = 0;
        if (k == HK_CORNER) g_stDragMode = 3;
        if (k == HK_TILE) { RECT r; TileRect(idx, &r); g_stOffX = g_stDP.x - r.left; g_stOffY = g_stDP.y - r.top; }
        return 0; }
    case WM_MOUSEMOVE: {
        int x = (short)LOWORD(l), y = (short)HIWORD(l), idx, k;
        if (w & MK_LBUTTON) {
            int dx = x - g_stDP.x, dy = y - g_stDP.y;
            if (!g_stMoved && (dx * dx + dy * dy) > 64) g_stMoved = 1;
            if (g_stMoved) {
                if (g_stDragMode == 0 && (g_stDK == HK_APP || (g_stDK == 0 && PtInRect(&rApps, g_stDP)))) g_stDragMode = 1;
                if (g_stDragMode == 0 && g_stDK == HK_TILE) { g_stDragMode = 2; g_stDragTile = g_stDI; }
                if (g_stDragMode == 1) { int mx = g_stNvis * RowH() - (rApps.bottom - rApps.top); if (mx < 0) mx = 0; g_stScroll = g_stScroll0 - dy; if (g_stScroll < 0) g_stScroll = 0; if (g_stScroll > mx) g_stScroll = mx; InvalidateRect(h, &rApps, FALSE); }
                else if (g_stDragMode == 2) { g_tcx[g_stDI] = x - g_stOffX; g_tcy[g_stDI] = y - g_stOffY; InvalidateRect(h, &rTiles, FALSE); }
                else if (g_stDragMode == 3) {
                    RECT r; LItem *t = &g_L[L_TILE][g_stDI]; int nw, nh; TileRect(g_stDI, &r);
                    nw = (x - r.left + g_cell / 2) / (g_cell + TGAP); nh = (y - r.top + g_cell / 2) / (g_cell + TGAP);
                    if (nw < 1) nw = 1; if (nw > 4) nw = 4; if (nh < 1) nh = 1; if (nh > 4) nh = 4;
                    if (nw != t->w || nh != t->h) { t->w = nw; t->h = nh; StKick(); InvalidateRect(h, &rTiles, FALSE); }
                }
            }
            return 0;
        }
        k = StHit(x, y, &idx);
        if (k != g_stHotK || idx != g_stHotI) { g_stHotK = k; g_stHotI = idx; InvalidateRect(h, NULL, FALSE); }
        return 0; }
    case WM_LBUTTONUP: {
        int x = (short)LOWORD(l), y = (short)HIWORD(l), idx, k = StHit(x, y, &idx);
        if (GetCapture() == h) ReleaseCapture();
        if (!g_stMoved) {
            if (g_stDK == HK_VB && k == HK_VB && idx == g_stDI) VbarAction(idx);
            else if (g_stDK == HK_ADD && k == HK_ADD) AddFlow();
            else if (g_stDK == HK_FILT && k == HK_FILT) {
                HMENU mn = CreatePopupMenu(); POINT pt; int c, i; GetCursorPos(&pt);
                for (i = 0; i < 6; i++) AppendMenuW(mn, MF_STRING | (i == g_stFilter ? MF_CHECKED : 0), i + 1, FILT_NAMES[i]);
                g_stMenu++; c = TrackPopupMenu(mn, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, h, NULL); DestroyMenu(mn); g_stMenu--;
                if (c) { g_stFilter = c - 1; g_stScroll = 0; StBuildVisible(); InvalidateRect(h, NULL, FALSE); }
            } else if (g_stDK == HK_APP && k == HK_APP && idx == g_stDI) { LaunchPath(g_apps[g_stVis[idx]].path); StartHide(); }
            else if ((g_stDK == HK_TILE) && k >= HK_TILE && idx == g_stDI) { LaunchPath(g_L[L_TILE][idx].p); StartHide(); }
        } else if (g_stDragMode == 2) {
            int target = -1, i; POINT p; p.x = x; p.y = y;
            for (i = 0; i < g_nL[L_TILE]; i++) if (i != g_stDI) { RECT r; r.left = g_ttx[i]; r.top = g_tty[i]; r.right = r.left + g_L[L_TILE][i].w * g_cell; r.bottom = r.top + g_L[L_TILE][i].h * g_cell; if (PtInRect(&r, p)) target = i; }
            if (target >= 0) {                       /* move the dragged tile to the target slot (neighbours slide) */
                LItem tmp = g_L[L_TILE][g_stDI]; int a = g_stDI;
                if (a < target) for (i = a; i < target; i++) g_L[L_TILE][i] = g_L[L_TILE][i + 1]; else for (i = a; i > target; i--) g_L[L_TILE][i] = g_L[L_TILE][i - 1];
                g_L[L_TILE][target] = tmp; SaveList(L_TILE);
            }
            g_stDragTile = -1; StKick(); InvalidateRect(h, &rTiles, FALSE);
        } else if (g_stDragMode == 3) SaveList(L_TILE);
        g_stDragMode = 0; g_stDragTile = -1; g_stDK = 0; return 0; }
    case WM_RBUTTONUP: {
        int idx, k = StHit((short)LOWORD(l), (short)HIWORD(l), &idx);
        if (k == HK_APP) AppMenu(idx); else if (k == HK_TILE || k == HK_CORNER) TileMenu(idx);
        return 0; }
    case WM_KEYDOWN: if (w == VK_ESCAPE) StartHide(); return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

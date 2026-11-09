/* ui2.h - pages, layout, lab actions, main window procedure */
static HWND g_pg[4][20]; static int g_pgn[4];
static RECT g_prev;
static const wchar_t *ISTYLE_NAMES[6] = { L"Theme default", L"Thin lines", L"Bold lines", L"Colour dots", L"Boxed", L"Filled shapes" };

static HWND Mk(int page, const wchar_t *cls, const wchar_t *txt, DWORD style, DWORD ex, int id) {
    HWND h = CreateWindowExW(ex, cls, txt, WS_CHILD | style | (page == g_tab ? WS_VISIBLE : 0), 0, 0, 10, 10, g_main, (HMENU)(INT_PTR)id, g_inst, NULL);
    if (h) { SendMessageW(h, WM_SETFONT, (WPARAM)g_fUi, TRUE); if (g_pgn[page] < 20) g_pg[page][g_pgn[page]++] = h; }
    return h;
}
#define BTN(page, txt, id) Mk(page, L"BUTTON", txt, BS_OWNERDRAW, 0, id)

static void FillGallery(void) {
    int i; SendMessageW(g_gal, WM_SETREDRAW, FALSE, 0); SendMessageW(g_gal, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < g_nthemes; i++) SendMessageW(g_gal, LB_ADDSTRING, 0, (LPARAM)i);
    SendMessageW(g_gal, LB_SETCURSEL, g_themeIdx, 0); SendMessageW(g_gal, WM_SETREDRAW, TRUE, 0); InvalidateRect(g_gal, NULL, TRUE);
}
static void UpdateIStyleText(void) {
    wchar_t b[64]; _snwprintf(b, 63, L"Icons: %ls", ISTYLE_NAMES[g_iconOver + 1]); b[63] = 0; SetWindowTextW(g_btnIStyle, b);
}
static void ShowTab(int t) {
    int p, i; g_tab = t;
    for (p = 0; p < 4; p++) for (i = 0; i < g_pgn[p]; i++) ShowWindow(g_pg[p][i], p == t ? SW_SHOW : SW_HIDE);
    for (i = 0; i < 4; i++) InvalidateRect(g_tabBtn[i], NULL, FALSE);
    InvalidateRect(g_main, NULL, TRUE);
    if (t == 0) Refresh(TRUE);
}

static void CreateControls(void) {
    static const wchar_t *tabs[4] = { L"Windows", L"Themes", L"Lab", L"Options" };
    static const wchar_t *acts[10] = { L"Skin", L"Unskin", L"Roll up", L"Pin", L"Close", L"Left", L"Right", L"Min", L"Max", L"Kill" };
    static const wchar_t *togs[5] = { L"Auto-skin all windows (running + new)", L"Apply theme to system colours/fonts",
                                      L"Keep running when closed (hidden)", L"Keep this manager on top", L"Dark manager interface" };
    static const wchar_t *obts[4] = { L"Restore original system look", L"Remove all skins now", L"Clear never-skin list", L"Re-apply theme to system" };
    int i;
    for (i = 0; i < 4; i++) g_tabBtn[i] = BTN(0, tabs[i], IDC_TAB0 + i);   /* tabs live on all pages: show always */
    for (i = 0; i < 4; i++) { ShowWindow(g_tabBtn[i], SW_SHOW); g_pgn[0]--; }
    g_edSearch = Mk(0, L"EDIT", L"", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, IDC_SEARCH);
    g_btnRef = BTN(0, L"Refresh", IDC_REFRESH);
    g_list = Mk(0, L"LISTBOX", L"", LBS_OWNERDRAWFIXED | LBS_NOTIFY | WS_VSCROLL | LBS_NOINTEGRALHEIGHT, WS_EX_CLIENTEDGE, IDC_LIST);
    for (i = 0; i < 10; i++) g_act[i] = BTN(0, acts[i], IDC_ACT + i + 1);
    g_edRun = Mk(0, L"EDIT", L"", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, IDC_RUN);
    g_btnBrowse = BTN(0, L"Browse", IDC_BROWSE); g_btnGo = BTN(0, L"Run", IDC_GO);
    g_btnIStyle = BTN(1, L"Icons", IDC_ISTYLE); g_btnColor = BTN(1, L"Accent", IDC_COLOR); g_btnReset = BTN(1, L"Reset", IDC_RESET);
    g_gal = Mk(1, L"LISTBOX", L"", LBS_OWNERDRAWFIXED | LBS_NOTIFY | WS_VSCROLL | LBS_NOINTEGRALHEIGHT, 0, IDC_GAL);
    g_edLab = Mk(2, L"EDIT", L"My theme", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, IDC_LABNAME);
    g_btnFrom = BTN(2, L"Start from...", IDC_LABFROM);
    g_props = Mk(2, L"LISTBOX", L"", LBS_OWNERDRAWFIXED | LBS_NOTIFY | WS_VSCROLL | LBS_NOINTEGRALHEIGHT, 0, IDC_PROPS);
    g_btnLSave = BTN(2, L"Save new", IDC_LSAVE); g_btnLUpd = BTN(2, L"Update", IDC_LUPD);
    g_btnLDel = BTN(2, L"Delete", IDC_LDEL); g_btnLRev = BTN(2, L"Revert", IDC_LREV);
    for (i = 0; i < 5; i++) g_tog[i] = BTN(3, togs[i], IDC_O_AUTO + i);
    for (i = 0; i < 4; i++) g_btnO[i] = BTN(3, obts[i], IDC_O_RESTORE + i);
    g_status = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX, 0, 0, 10, 10, g_main, (HMENU)(INT_PTR)IDC_STATUS, g_inst, NULL);
    SendMessageW(g_status, WM_SETFONT, (WPARAM)g_fSmall, TRUE);
    SendMessageW(g_edSearch, EM_SETLIMITTEXT, 100, 0);
    SendMessageW(g_edLab, EM_SETLIMITTEXT, 40, 0);
    for (i = 0; i < NPROPS; i++) SendMessageW(g_props, LB_ADDSTRING, 0, (LPARAM)i);
}

static void Layout(void) {
    RECT rc; int W, H, m = 6, bh = 44, g = 4, i, top, sw, yStat, yRun, yA2, yA1, bw, ph, y;
    if (!GetClientRect(g_main, &rc)) return;
    W = rc.right; H = rc.bottom; yStat = H - 22; top = m + bh + g;
    bw = (W - 2 * m - 3 * g) / 4;
    for (i = 0; i < 4; i++) MoveWindow(g_tabBtn[i], m + i * (bw + g), m, bw, bh, TRUE);
    /* Windows page */
    sw = W - 2 * m - g - 90;
    MoveWindow(g_edSearch, m, top, sw, bh, TRUE); MoveWindow(g_btnRef, m + sw + g, top, 90, bh, TRUE);
    yRun = yStat - g - bh; yA2 = yRun - g - bh; yA1 = yA2 - g - bh;
    y = top + bh + g; MoveWindow(g_list, m, y, W - 2 * m, yA1 - g - y > 40 ? yA1 - g - y : 40, TRUE);
    bw = (W - 2 * m - 4 * g) / 5;
    for (i = 0; i < 5; i++) { MoveWindow(g_act[i], m + i * (bw + g), yA1, bw, bh, TRUE); MoveWindow(g_act[5 + i], m + i * (bw + g), yA2, bw, bh, TRUE); }
    sw = W - 2 * m - 80 - 60 - 2 * g;
    MoveWindow(g_edRun, m, yRun, sw, bh, TRUE); MoveWindow(g_btnBrowse, m + sw + g, yRun, 80, bh, TRUE); MoveWindow(g_btnGo, m + sw + g + 84, yRun, 60, bh, TRUE);
    /* preview rect (Themes + Lab) */
    ph = (H >= 520) ? 150 : 110; SetRect(&g_prev, m, top, W - m, top + ph);
    /* Themes page */
    y = top + ph + g; sw = W - 2 * m - 2 * (84 + g);
    MoveWindow(g_btnIStyle, m, y, sw, bh, TRUE); MoveWindow(g_btnColor, m + sw + g, y, 84, bh, TRUE); MoveWindow(g_btnReset, m + sw + g + 88, y, 84, bh, TRUE);
    MoveWindow(g_gal, m, y + bh + g, W - 2 * m, yStat - g - (y + bh + g), TRUE);
    /* Lab page */
    sw = W - 2 * m - g - 120;
    MoveWindow(g_edLab, m, y, sw, bh, TRUE); MoveWindow(g_btnFrom, m + sw + g, y, 120, bh, TRUE);
    bw = (W - 2 * m - 3 * g) / 4;
    MoveWindow(g_props, m, y + bh + g, W - 2 * m, yStat - g - bh - g - (y + bh + g), TRUE);
    yA1 = yStat - g - bh;
    MoveWindow(g_btnLSave, m, yA1, bw, bh, TRUE); MoveWindow(g_btnLUpd, m + (bw + g), yA1, bw, bh, TRUE);
    MoveWindow(g_btnLDel, m + 2 * (bw + g), yA1, bw, bh, TRUE); MoveWindow(g_btnLRev, m + 3 * (bw + g), yA1, bw, bh, TRUE);
    /* Options page */
    y = top;
    for (i = 0; i < 5; i++) { MoveWindow(g_tog[i], m, y, W - 2 * m, bh, TRUE); y += bh + g; }
    y += 6;
    for (i = 0; i < 4; i++) { MoveWindow(g_btnO[i], m, y, W - 2 * m, bh, TRUE); y += bh + g; }
    MoveWindow(g_status, m, yStat, W - 2 * m, 20, TRUE);
}

/* ---- Theme Lab actions ---- */
static void LabLoadFrom(int idx) {
    if (idx < 0 || idx >= g_nthemes) return;
    g_lab = g_themes[idx]; g_labIdx = idx;
    SetWindowTextW(g_edLab, g_lab.custom ? g_lab.name : L"My theme");
    InvalidateRect(g_props, NULL, FALSE); InvalidateRect(g_main, NULL, FALSE);
}
static void LabReadName(void) {
    wchar_t n[48]; GetWindowTextW(g_edLab, n, 47); n[47] = 0;
    if (!n[0]) wcscpy(n, L"Custom theme");
    wcscpy(g_lab.name, n);
}
static void LabSaveNew(void) {
    int idx; Theme t; LabReadName(); t = g_lab; ThemeFixup(&t);
    idx = SaveNewCustom(&t);
    if (idx < 0) { SetStatus(L"Theme list is full."); return; }
    ImgClear(); FillGallery(); LabLoadFrom(idx); ApplyTheme(idx); FillGallery();
    SetStatus(L"Saved \"%ls\" to themes\\%ls", g_themes[idx].name, g_themes[idx].file);
}
static void LabUpdate(void) {
    wchar_t path[MAX_PATH]; int i;
    if (g_labIdx < g_nbuiltin || !g_themes[g_labIdx].custom) { LabSaveNew(); return; }
    LabReadName();
    for (i = 0; i < g_nthemes; i++) if (i != g_labIdx && !_wcsicmp(g_themes[i].name, g_lab.name)) { SetStatus(L"Another theme already uses that name."); return; }
    wcscpy(g_lab.file, g_themes[g_labIdx].file); g_lab.custom = 1; ThemeFixup(&g_lab);
    g_themes[g_labIdx] = g_lab; ThemeFilePath(&g_lab, path); ThemeSave(&g_lab, path); ImgClear();
    FillGallery(); ApplyTheme(g_labIdx); SetStatus(L"Theme updated.");
}
static void LabDelete(void) {
    wchar_t path[MAX_PATH]; int i = g_themeIdx;
    if (i < g_nbuiltin || !g_themes[i].custom) { SetStatus(L"Built-in themes cannot be deleted. Select a custom theme first."); return; }
    if (MessageBoxW(g_main, L"Delete this custom theme?", APP_TITLE, MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    ThemeFilePath(&g_themes[i], path); DeleteFileW(path);
    for (; i < g_nthemes - 1; i++) g_themes[i] = g_themes[i + 1];
    g_nthemes--; g_labIdx = 0; FillGallery(); ApplyTheme(0); FillGallery(); SetStatus(L"Theme deleted.");
}
static void ImportThemeFile(const wchar_t *file) {
    wchar_t name[MAX_PATH], dest[MAX_PATH]; const wchar_t *sl; Theme t; int idx;
    sl = wcsrchr(file, L'\\'); sl = sl ? sl + 1 : file;
    wcsncpy(name, sl, MAX_PATH - 1); name[MAX_PATH - 1] = 0;
    wcscpy(dest, g_themeDir); wcscat(dest, name);
    if (_wcsicmp(file, dest) != 0) CopyFileW(file, dest, FALSE);
    if (!ThemeLoad(&t, dest)) return;
    wcsncpy(t.file, name, 63); t.file[63] = 0;
    idx = FindTheme(t.name);
    if (idx < 0) idx = AddTheme(&t);
    if (idx >= 0) { if (g_gal) FillGallery(); ApplyTheme(idx); }
}

static void SetMgrTop(void) {
    SetWindowPos(g_main, g_mgrTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}
static void SaveConfig(void) {
    RECT r;
    WritePrivateProfileStringW(L"cfg", L"theme", g_themes[g_themeIdx].name, g_ini);
    WrI(L"icons", g_iconOver); WrI(L"hascolor", g_hasCol); WrI(L"color", (int)g_col); WrI(L"dark", g_dark); WrI(L"top", g_mgrTop);
    WrI(L"auto", g_auto); WrI(L"sys", g_sysApply); WrI(L"bg", g_bg);
    WritePrivateProfileStringW(L"cfg", L"exclude", g_exclude, g_ini);
    if (g_main && !IsIconic(g_main) && IsWindowVisible(g_main) && GetWindowRect(g_main, &r)) {
        WrI(L"x", r.left); WrI(L"y", r.top); WrI(L"w", r.right - r.left); WrI(L"h", r.bottom - r.top);
    }
}

static void DoRun(void) {
    wchar_t cmd[MAX_PATH * 2]; HINSTANCE r;
    GetWindowTextW(g_edRun, cmd, MAX_PATH * 2);
    if (!cmd[0]) { SetStatus(L"Type or browse a program to run."); return; }
    r = ShellExecuteW(g_main, L"open", cmd, NULL, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)r > 32) { SetStatus(L"Started."); return; }
    {   STARTUPINFOW si; PROCESS_INFORMATION pi; memset(&si, 0, sizeof si); si.cb = sizeof si;
        if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) { CloseHandle(pi.hThread); CloseHandle(pi.hProcess); SetStatus(L"Started."); }
        else SetStatus(L"Run failed (error %lu).", GetLastError()); }
}
static void DoBrowse(void) {
    wchar_t f[MAX_PATH] = L""; OPENFILENAMEW o; memset(&o, 0, sizeof o); o.lStructSize = sizeof o; o.hwndOwner = g_main;
    o.lpstrFilter = L"Programs\0*.exe;*.bat;*.lnk\0All files\0*.*\0"; o.lpstrFile = f; o.nMaxFile = MAX_PATH;
    o.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameW(&o)) SetWindowTextW(g_edRun, f);
}

static void ListContextMenu(int sx, int sy) {
    HWND t = SelectedHwnd(); HMENU mn; int cmd; Skin *e;
    if (!t) return;
    e = SkinFind(t); mn = CreatePopupMenu(); if (!mn) return;
    AppendMenuW(mn, MF_STRING, CM_BASE + (e ? ACT_UNSKIN : ACT_SKIN), e ? L"Remove skin" : L"Apply skin");
    AppendMenuW(mn, MF_STRING, CM_BASE + ACT_SHADE, L"Roll up / Unroll");
    AppendMenuW(mn, MF_STRING | (IsPinned(t) ? MF_CHECKED : 0), CM_BASE + ACT_TOP, L"Always on top (pin)");
    AppendMenuW(mn, MF_SEPARATOR, 0, NULL);
    AppendMenuW(mn, MF_STRING, CM_BASE + ACT_MIN, L"Minimize"); AppendMenuW(mn, MF_STRING, CM_BASE + ACT_MAX, L"Maximize / Restore");
    AppendMenuW(mn, MF_STRING, CM_BASE + ACT_LEFT, L"Snap left half"); AppendMenuW(mn, MF_STRING, CM_BASE + ACT_RIGHT, L"Snap right half");
    AppendMenuW(mn, MF_STRING, CM_BASE + ACT_CLOSE, L"Close");
    AppendMenuW(mn, MF_SEPARATOR, 0, NULL);
    AppendMenuW(mn, MF_STRING, CM_BASE + ACT_NEVER, L"Never skin this app");
    AppendMenuW(mn, MF_STRING, CM_BASE + ACT_SKINALL, L"Skin all listed windows");
    AppendMenuW(mn, MF_STRING, CM_BASE + ACT_UNSKINALL, L"Remove all skins");
    AppendMenuW(mn, MF_STRING, CM_BASE + ACT_KILL, L"Kill process");
    cmd = TrackPopupMenu(mn, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, sx, sy, 0, g_main, NULL);
    DestroyMenu(mn);
    if (cmd >= CM_BASE) DoAction(cmd - CM_BASE, t);
}

static void DrawWinItem(const DRAWITEMSTRUCT *d) {
    WinInfo *w; RECT rc = d->rcItem, r1, r2; BOOL sel = (d->itemState & ODS_SELECTED) != 0;
    wchar_t line[200]; const wchar_t *st; COLORREF bg, fg, sub; HDC dc = d->hDC;
    bg = sel ? UiAccent() : UiPanel(); fg = sel ? (Lum(UiAccent()) < 150 ? W_ : K_) : UiText(); sub = sel ? Blend(fg, bg, 30) : UiSub();
    FillRectC(dc, rc.left, rc.top, rc.right, rc.bottom, bg);
    if ((int)d->itemID < 0 || (int)d->itemID >= g_nwins) return;
    w = &g_wins[d->itemID];
    FillRectC(dc, rc.left, rc.bottom - 1, rc.right, rc.bottom, UiBg());
    st = w->shaded ? L"rolled up" : (w->iconic ? L"minimized" : (w->zoomed ? L"maximized" : L"normal"));
    _snwprintf(line, 199, L"%ls  |  %ls%ls%ls", w->proc, st, w->top ? L"  |  pinned" : L"", w->skinned ? L"  |  skinned" : L""); line[199] = 0;
    SetBkMode(dc, TRANSPARENT);
    r1 = rc; r1.left += 10; r1.right -= 8; r1.top += 6; r1.bottom = r1.top + 20;
    SelectObject(dc, g_fUi); SetTextColor(dc, fg); DrawTextW(dc, w->title, -1, &r1, DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    r2 = rc; r2.left += 10; r2.right -= 8; r2.top += 28; r2.bottom -= 2;
    SelectObject(dc, g_fSmall); SetTextColor(dc, sub); DrawTextW(dc, line, -1, &r2, DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
}

static void PickAccent(void) {
    COLORREF c = g_col;
    if (PickColor(&c)) { g_col = c; g_hasCol = 1; RebuildEff(); ApplyTheme(g_themeIdx); }
}

static LRESULT CALLBACK MainProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE:
        g_main = h; CreateControls(); Layout(); FillGallery(); UpdateIStyleText();
        LabLoadFrom(g_themeIdx);
        SetTimer(h, TM_REFRESH, 1500, NULL); SetTimer(h, TM_SYNC, 33, NULL); SetTimer(h, TM_SCAN, 200, NULL);
        SetStatus(L"Auto-skin is %ls. Pick a look on the Themes tab.", g_auto ? L"ON" : L"OFF");
        Refresh(TRUE);
        return 0;
    case WM_GETMINMAXINFO: { MINMAXINFO *mi = (MINMAXINFO *)l; mi->ptMinTrackSize.x = 360; mi->ptMinTrackSize.y = 520; return 0; }
    case WM_SIZE: if (w != SIZE_MINIMIZED) { Layout(); InvalidateRect(h, NULL, TRUE); } return 0;
    case WM_ERASEBKGND: { RECT rc; GetClientRect(h, &rc); FillRectC((HDC)w, rc.left, rc.top, rc.right, rc.bottom, UiBg()); return 1; }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        if (g_tab == 1) PaintThemePreview(dc, g_prev, CurTheme());
        else if (g_tab == 2) PaintThemePreview(dc, g_prev, &g_lab);
        EndPaint(h, &ps); return 0; }
    case WM_MEASUREITEM: { MEASUREITEMSTRUCT *mi = (MEASUREITEMSTRUCT *)l;
        if (mi->CtlID == IDC_LIST) mi->itemHeight = 52; else if (mi->CtlID == IDC_GAL) mi->itemHeight = 66; else if (mi->CtlID == IDC_PROPS) mi->itemHeight = 44;
        return TRUE; }
    case WM_DRAWITEM: { DRAWITEMSTRUCT *d = (DRAWITEMSTRUCT *)l;
        if (d->CtlType == ODT_BUTTON) { DrawUiButton(d); return TRUE; }
        if (d->CtlID == IDC_LIST) DrawWinItem(d); else if (d->CtlID == IDC_GAL) DrawGalleryItem(d); else if (d->CtlID == IDC_PROPS) DrawPropItem(d);
        return TRUE; }
    case WM_CTLCOLOREDIT: case WM_CTLCOLORLISTBOX: {
        static HBRUSH br; static COLORREF last = (COLORREF)-1; COLORREF c = g_dark ? RGB(45,45,48) : W_;
        if (c != last) { if (br) DeleteObject(br); br = CreateSolidBrush(c); last = c; }
        SetTextColor((HDC)w, UiText()); SetBkColor((HDC)w, c); return (LRESULT)br; }
    case WM_CTLCOLORSTATIC: {
        static HBRUSH br; static COLORREF last = (COLORREF)-1; COLORREF c = UiBg();
        if (c != last) { if (br) DeleteObject(br); br = CreateSolidBrush(c); last = c; }
        SetTextColor((HDC)w, UiSub()); SetBkColor((HDC)w, c); return (LRESULT)br; }
    case WM_TIMER:
        if (w == TM_REFRESH) { if (g_tab == 0) Refresh(FALSE); }
        else if (w == TM_SYNC) SyncAll();
        else if (w == TM_SCAN) { if (g_auto) AutoScan(); }
        return 0;
    case WM_CONTEXTMENU:
        if ((HWND)w == g_list) {
            POINT p, cp; LRESULT r;
            if ((int)(short)LOWORD(l) == -1) GetCursorPos(&p); else { p.x = (short)LOWORD(l); p.y = (short)HIWORD(l); }
            cp = p; ScreenToClient(g_list, &cp);
            r = SendMessageW(g_list, LB_ITEMFROMPOINT, 0, MAKELPARAM(cp.x, cp.y));
            if (HIWORD(r) == 0 && (int)LOWORD(r) < g_nwins) { SendMessageW(g_list, LB_SETCURSEL, LOWORD(r), 0); ListContextMenu(p.x, p.y); }
            return 0;
        }
        break;
    case WM_COMMAND: {
        int id = LOWORD(w), code = HIWORD(w);
        if (id >= IDC_TAB0 && id <= IDC_TAB3) { ShowTab(id - IDC_TAB0); return 0; }
        if (id > IDC_ACT && id <= IDC_ACT + 10) { DoAction(id - IDC_ACT, SelectedHwnd()); return 0; }
        if (id >= IDC_O_AUTO && id <= IDC_O_DARK) {
            int *tg = TogPtr(id); *tg = !*tg;
            if (id == IDC_O_TOP) SetMgrTop();
            if (id == IDC_O_SYS && g_sysApply) ApplySystemLook(CurTheme());
            if (id == IDC_O_DARK) InvalidateRect(h, NULL, TRUE);
            if (id == IDC_O_AUTO) SetStatus(g_auto ? L"Auto-skin ON: all windows will be skinned." : L"Auto-skin OFF.");
            SaveConfig(); InvalidateRect((HWND)l, NULL, FALSE); return 0;
        }
        switch (id) {
        case IDC_SEARCH: if (code == EN_CHANGE) Refresh(TRUE); break;
        case IDC_REFRESH: Refresh(TRUE); SetStatus(L"%d window(s).", g_nwins); break;
        case IDC_LIST:
            if (code == LBN_SELCHANGE) { HWND t = SelectedHwnd(); if (t) { if (IsIconic(t)) ShowWindow(t, SW_RESTORE); { Skin *e = SkinFind(t); if (e) SkinRaise(e); else SetForegroundWindow(t); } } }
            else if (code == LBN_DBLCLK) { HWND t = SelectedHwnd(); if (t) DoAction(SkinFind(t) ? ACT_UNSKIN : ACT_SKIN, t); }
            break;
        case IDC_BROWSE: DoBrowse(); break;
        case IDC_GO: DoRun(); break;
        case IDC_GAL: if (code == LBN_SELCHANGE) { int i = (int)SendMessageW(g_gal, LB_GETCURSEL, 0, 0); if (i >= 0 && i != g_themeIdx) { ApplyTheme(i); LabLoadFrom(i); } } break;
        case IDC_ISTYLE: {
            RECT r; POINT pt; int c; GetWindowRect(g_btnIStyle, &r); pt.x = r.left; pt.y = r.bottom;
            c = PopupChoose(pt, ISTYLE_NAMES, 6, g_iconOver + 1);
            if (c >= 0) { g_iconOver = c - 1; RebuildEff(); UpdateIStyleText(); ApplyLookAll(); SaveConfig(); }
            break; }
        case IDC_COLOR: PickAccent(); SaveConfig(); break;
        case IDC_RESET: g_hasCol = 0; g_iconOver = -1; UpdateIStyleText(); RebuildEff(); ApplyTheme(g_themeIdx); SaveConfig(); break;
        case IDC_PROPS: if (code == LBN_SELCHANGE) { int i = (int)SendMessageW(g_props, LB_GETCURSEL, 0, 0); SendMessageW(g_props, LB_SETCURSEL, (WPARAM)-1, 0); if (i >= 0) EditProp(i); } break;
        case IDC_LABFROM: {
            static const wchar_t *nm[MAXTHEME]; RECT r; POINT pt; int i, c;
            for (i = 0; i < g_nthemes; i++) nm[i] = g_themes[i].name;
            GetWindowRect(g_btnFrom, &r); pt.x = r.left; pt.y = r.bottom; c = PopupChoose(pt, nm, g_nthemes, g_labIdx);
            if (c >= 0) LabLoadFrom(c);
            break; }
        case IDC_LSAVE: LabSaveNew(); break;
        case IDC_LUPD: LabUpdate(); break;
        case IDC_LDEL: LabDelete(); break;
        case IDC_LREV: LabLoadFrom(g_labIdx); SetStatus(L"Reverted."); break;
        case IDC_O_RESTORE: SetStatus(RestoreSystemLook() ? L"Original system colours/fonts restored." : L"No backup found yet."); break;
        case IDC_O_UNSKIN: DoAction(ACT_UNSKINALL, NULL); break;
        case IDC_O_CLEAR: wcscpy(g_exclude, L";explorer.exe;"); g_nNoSkin = 0; SetStatus(L"Never-skin list cleared."); SaveConfig(); break;
        case IDC_O_REAPPLY: ApplySystemLook(CurTheme()); SetStatus(L"System colours and fonts re-applied."); break;
        }
        return 0; }
    case WM_SHOWAPP: ShowWindow(h, SW_SHOWNORMAL); SetForegroundWindow(h); Refresh(TRUE); return 0;
    case WM_QUITAPP: SaveConfig(); KillTimer(h, TM_SYNC); SkinRemoveAll(); DestroyWindow(h); return 0;
    case WM_CLOSE:
        if (g_bg) { SaveConfig(); ShowWindow(h, SW_HIDE); return 0; }
        SaveConfig(); KillTimer(h, TM_SYNC); SkinRemoveAll(); DestroyWindow(h); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

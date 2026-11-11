/* ui2.h - pages (Themes / Lab / Options), layout, lab actions, main window procedure */
static HWND g_pg[3][20]; static int g_pgn[3];
static RECT g_prev;
static const wchar_t *ISTYLE_NAMES[6] = { L"Theme default", L"Thin lines", L"Bold lines", L"Colour dots", L"Boxed", L"Filled shapes" };

static HWND MkRaw(const wchar_t *cls, const wchar_t *txt, DWORD style, DWORD ex, int id, BOOL visible) {
    HWND h = CreateWindowExW(ex, cls, txt, WS_CHILD | style | (visible ? WS_VISIBLE : 0), 0, 0, 10, 10, g_main, (HMENU)(INT_PTR)id, g_inst, NULL);
    if (h) SendMessageW(h, WM_SETFONT, (WPARAM)g_fUi, TRUE);
    return h;
}
static HWND Mk(int page, const wchar_t *cls, const wchar_t *txt, DWORD style, DWORD ex, int id) {
    HWND h = MkRaw(cls, txt, style, ex, id, page == g_tab);
    if (h && g_pgn[page] < 20) g_pg[page][g_pgn[page]++] = h;
    return h;
}
#define BTN(page, txt, id) Mk(page, L"BUTTON", txt, BS_OWNERDRAW, 0, id)

static void FillGallery(void) {
    int i; SendMessageW(g_gal, WM_SETREDRAW, FALSE, 0); SendMessageW(g_gal, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < g_nthemes; i++) SendMessageW(g_gal, LB_ADDSTRING, 0, (LPARAM)i);
    SendMessageW(g_gal, LB_SETCURSEL, g_themeIdx, 0); SendMessageW(g_gal, WM_SETREDRAW, TRUE, 0); InvalidateRect(g_gal, NULL, TRUE);
}
static void UpdateButtonTexts(void) {
    wchar_t b[64];
    _snwprintf(b, 63, L"Icons: %ls", ISTYLE_NAMES[g_iconOver + 1]); b[63] = 0; SetWindowTextW(g_btnIStyle, b);
    if (g_presetAll) _snwprintf(b, 63, L"Presets: #%d", g_presetAll); else _snwprintf(b, 63, L"Presets: theme"); b[63] = 0;
    SetWindowTextW(g_btnPresets, b);
}
static void ShowTab(int t) {
    int p, i; g_tab = t;
    for (p = 0; p < 3; p++) for (i = 0; i < g_pgn[p]; i++) ShowWindow(g_pg[p][i], p == t ? SW_SHOW : SW_HIDE);
    for (i = 0; i < 3; i++) InvalidateRect(g_tabBtn[i], NULL, FALSE);
    InvalidateRect(g_main, NULL, TRUE);
}

static void CreateControls(void) {
    static const wchar_t *tabs[3] = { L"Themes", L"Lab", L"Options" };
    static const wchar_t *togs[8] = { L"Auto-skin all windows (running + new)", L"Theme system colours + fonts",
        L"Show Pin button", L"Show Roll-up button", L"Show Snap buttons", L"Keep running when closed", L"Manager always on top", L"Dark interface" };
    static const wchar_t *acts[6] = { L"Run a program...", L"Never skin an app...", L"Clear never-skin list",
        L"Remove all skins now", L"Re-apply system look", L"Restore original look" };
    int i;
    for (i = 0; i < 3; i++) g_tabBtn[i] = MkRaw(L"BUTTON", tabs[i], BS_OWNERDRAW, 0, IDC_TAB0 + i, TRUE);
    /* Themes */
    g_btnIStyle = BTN(0, L"Icons", IDC_ISTYLE); g_btnPresets = BTN(0, L"Presets", IDC_PRESETS);
    g_btnColor = BTN(0, L"Accent", IDC_COLOR); g_btnReset = BTN(0, L"Reset", IDC_RESET);
    g_gal = Mk(0, L"LISTBOX", L"", LBS_OWNERDRAWFIXED | LBS_NOTIFY | WS_VSCROLL | LBS_NOINTEGRALHEIGHT, 0, IDC_GAL);
    /* Lab */
    g_edLab = Mk(1, L"EDIT", L"My theme", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, IDC_LABNAME);
    g_btnFrom = BTN(1, L"Start from...", IDC_LABFROM);
    g_props = Mk(1, L"LISTBOX", L"", LBS_OWNERDRAWFIXED | LBS_NOTIFY | WS_VSCROLL | LBS_NOINTEGRALHEIGHT, 0, IDC_PROPS);
    g_btnLSave = BTN(1, L"Save new", IDC_LSAVE); g_btnLUpd = BTN(1, L"Update", IDC_LUPD);
    g_btnLDel = BTN(1, L"Delete", IDC_LDEL); g_btnLRev = BTN(1, L"Revert", IDC_LREV);
    /* Options */
    for (i = 0; i < 8; i++) g_tog[i] = BTN(2, togs[i], IDC_O_AUTO + i);
    for (i = 0; i < 6; i++) g_btnO[i] = BTN(2, acts[i], IDC_O_RUN + i);
    g_status = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX, 0, 0, 10, 10, g_main, (HMENU)(INT_PTR)IDC_STATUS, g_inst, NULL);
    SendMessageW(g_status, WM_SETFONT, (WPARAM)g_fSmall, TRUE);
    SendMessageW(g_edLab, EM_SETLIMITTEXT, 40, 0);
    for (i = 0; i < NPROPS; i++) SendMessageW(g_props, LB_ADDSTRING, 0, (LPARAM)i);
}

static void Layout(void) {
    RECT rc; int W, H, m = 6, bh = 44, g = 4, i, top, sw, yStat, bw, ph, y, rw, cw, y2;
    if (!GetClientRect(g_main, &rc)) return;
    W = rc.right; H = rc.bottom; yStat = H - 22; top = m + bh + g;
    bw = (W - 2 * m - 2 * g) / 3;
    for (i = 0; i < 3; i++) MoveWindow(g_tabBtn[i], m + i * (bw + g), m, bw, bh, TRUE);
    ph = (H >= 520) ? 150 : 110; SetRect(&g_prev, m, top, W - m, top + ph);
    y = top + ph + g;
    /* Themes */
    rw = (W - 2 * m - 3 * g - 76 - 64) / 2;
    MoveWindow(g_btnIStyle, m, y, rw, bh, TRUE); MoveWindow(g_btnPresets, m + rw + g, y, rw, bh, TRUE);
    MoveWindow(g_btnColor, m + 2 * (rw + g), y, 76, bh, TRUE); MoveWindow(g_btnReset, m + 2 * (rw + g) + 76 + g, y, 64, bh, TRUE);
    MoveWindow(g_gal, m, y + bh + g, W - 2 * m, yStat - g - (y + bh + g), TRUE);
    /* Lab */
    sw = W - 2 * m - g - 120;
    MoveWindow(g_edLab, m, y, sw, bh, TRUE); MoveWindow(g_btnFrom, m + sw + g, y, 120, bh, TRUE);
    bw = (W - 2 * m - 3 * g) / 4;
    MoveWindow(g_props, m, y + bh + g, W - 2 * m, yStat - g - bh - g - (y + bh + g), TRUE);
    y2 = yStat - g - bh;
    MoveWindow(g_btnLSave, m, y2, bw, bh, TRUE); MoveWindow(g_btnLUpd, m + (bw + g), y2, bw, bh, TRUE);
    MoveWindow(g_btnLDel, m + 2 * (bw + g), y2, bw, bh, TRUE); MoveWindow(g_btnLRev, m + 3 * (bw + g), y2, bw, bh, TRUE);
    /* Options: two columns */
    cw = (W - 2 * m - g) / 2;
    for (i = 0; i < 8; i++) MoveWindow(g_tog[i], m + (i % 2) * (cw + g), top + (i / 2) * (bh + g), cw, bh, TRUE);
    y2 = top + 4 * (bh + g) + 8;
    for (i = 0; i < 6; i++) MoveWindow(g_btnO[i], m + (i % 2) * (cw + g), y2 + (i / 2) * (bh + g), cw, bh, TRUE);
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
    ImgClear(); LabLoadFrom(idx); ApplyTheme(idx); FillGallery();
    SetStatus(L"Saved \"%ls\" to themes\\%ls", g_themes[idx].name, g_themes[idx].file);
}
static void LabUpdate(void) {
    wchar_t path[MAX_PATH]; int i;
    if (g_labIdx < g_nbuiltin || !g_themes[g_labIdx].custom) { LabSaveNew(); return; }
    LabReadName();
    for (i = 0; i < g_nthemes; i++) if (i != g_labIdx && !_wcsicmp(g_themes[i].name, g_lab.name)) { SetStatus(L"Another theme already uses that name."); return; }
    wcscpy(g_lab.file, g_themes[g_labIdx].file); g_lab.custom = 1; ThemeFixup(&g_lab);
    g_themes[g_labIdx] = g_lab; ThemeFilePath(&g_lab, path); ThemeSave(&g_lab, path); ImgClear();
    ApplyTheme(g_labIdx); FillGallery(); SetStatus(L"Theme updated.");
}
static void LabDelete(void) {
    wchar_t path[MAX_PATH]; int i = g_themeIdx;
    if (i < g_nbuiltin || !g_themes[i].custom) { SetStatus(L"Built-in themes cannot be deleted. Select a custom theme first."); return; }
    if (MessageBoxW(g_main, L"Delete this custom theme?", APP_TITLE, MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    ThemeFilePath(&g_themes[i], path); DeleteFileW(path);
    for (; i < g_nthemes - 1; i++) g_themes[i] = g_themes[i + 1];
    g_nthemes--; g_labIdx = 0; ApplyTheme(0); FillGallery(); SetStatus(L"Theme deleted.");
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
    WrI(L"icons", g_iconOver); WrI(L"presetall", g_presetAll); WrI(L"hascolor", g_hasCol); WrI(L"color", (int)g_col);
    WrI(L"dark", g_dark); WrI(L"top", g_mgrTop); WrI(L"auto", g_auto); WrI(L"sys", g_sysApply); WrI(L"bg", g_bg);
    WrI(L"showpin", g_showPin); WrI(L"showshade", g_showShade); WrI(L"showsnap", g_showSnap);
    WritePrivateProfileStringW(L"cfg", L"exclude", g_exclude, g_ini);
    if (g_main && !IsIconic(g_main) && IsWindowVisible(g_main) && GetWindowRect(g_main, &r)) {
        WrI(L"x", r.left); WrI(L"y", r.top); WrI(L"w", r.right - r.left); WrI(L"h", r.bottom - r.top);
    }
}

static BOOL BrowseFile(wchar_t *f, const wchar_t *filter) {
    OPENFILENAMEW o; memset(&o, 0, sizeof o); f[0] = 0; o.lStructSize = sizeof o; o.hwndOwner = g_main;
    o.lpstrFilter = filter; o.lpstrFile = f; o.nMaxFile = MAX_PATH; o.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    return GetOpenFileNameW(&o);
}
static void RunProgram(void) {
    wchar_t f[MAX_PATH]; HINSTANCE r;
    if (!BrowseFile(f, L"Programs\0*.exe;*.bat;*.lnk\0All files\0*.*\0")) return;
    r = ShellExecuteW(g_main, L"open", f, NULL, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)r > 32) SetStatus(L"Started."); else SetStatus(L"Could not start the program (code %d).", (int)(INT_PTR)r);
}
static void NeverSkinApp(void) {
    wchar_t f[MAX_PATH], *sl;
    if (!BrowseFile(f, L"Programs\0*.exe\0All files\0*.*\0")) return;
    sl = wcsrchr(f, L'\\'); sl = sl ? sl + 1 : f;
    AddExclude(sl); UnskinExe(sl); SaveConfig();
    SetStatus(L"%ls will never be skinned.", sl);
}
static void PickAccent(void) {
    COLORREF c = g_col;
    if (PickColor(&c)) { g_col = c; g_hasCol = 1; RebuildEff(); ApplyTheme(g_themeIdx); }
}

static LRESULT CALLBACK MainProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE:
        g_main = h; CreateControls(); Layout(); FillGallery(); UpdateButtonTexts(); LabLoadFrom(g_themeIdx);
        SetTimer(h, TM_SYNC, 15, NULL); SetTimer(h, TM_SCAN, 200, NULL);
        SetStatus(L"Auto-skin is %ls. Pick a look on the Themes tab.", g_auto ? L"ON" : L"OFF");
        return 0;
    case WM_GETMINMAXINFO: { MINMAXINFO *mi = (MINMAXINFO *)l; mi->ptMinTrackSize.x = 360; mi->ptMinTrackSize.y = 520; return 0; }
    case WM_SIZE: if (w != SIZE_MINIMIZED) { Layout(); InvalidateRect(h, NULL, TRUE); } return 0;
    case WM_ERASEBKGND: { RECT rc; GetClientRect(h, &rc); FillRectC((HDC)w, rc.left, rc.top, rc.right, rc.bottom, UiBg()); return 1; }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        if (g_tab == 0) PaintThemePreview(dc, g_prev, CurTheme());
        else if (g_tab == 1) PaintThemePreview(dc, g_prev, &g_lab);
        EndPaint(h, &ps); return 0; }
    case WM_MEASUREITEM: { MEASUREITEMSTRUCT *mi = (MEASUREITEMSTRUCT *)l;
        if (mi->CtlType == ODT_MENU) { MeasureMenuPreset(mi); return TRUE; }
        if (mi->CtlID == IDC_GAL) mi->itemHeight = 66; else if (mi->CtlID == IDC_PROPS) mi->itemHeight = 44;
        return TRUE; }
    case WM_DRAWITEM: { DRAWITEMSTRUCT *d = (DRAWITEMSTRUCT *)l;
        if (d->CtlType == ODT_MENU) { DrawMenuPreset(d); return TRUE; }
        if (d->CtlType == ODT_BUTTON) { DrawUiButton(d); return TRUE; }
        if (d->CtlID == IDC_GAL) DrawGalleryItem(d); else if (d->CtlID == IDC_PROPS) DrawPropItem(d);
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
        if (w == TM_SYNC) SyncAll(); else if (w == TM_SCAN) { if (g_auto) AutoScan(); }
        return 0;
    case WM_COMMAND: {
        int id = LOWORD(w), code = HIWORD(w);
        if (id >= IDC_TAB0 && id <= IDC_TAB2) { ShowTab(id - IDC_TAB0); return 0; }
        if (id >= IDC_O_AUTO && id <= IDC_O_DARK) {
            int *tg = TogPtr(id); *tg = !*tg;
            if (id == IDC_O_TOP) SetMgrTop();
            if (id == IDC_O_SYS && g_sysApply) ApplySystemLook(CurTheme());
            if (id == IDC_O_DARK) InvalidateRect(h, NULL, TRUE);
            if (id == IDC_O_PIN || id == IDC_O_SHADE || id == IDC_O_SNAP) { RecalcHide(); ApplyLookAll(); SetStatus(L"Title bar buttons updated."); }
            if (id == IDC_O_AUTO) SetStatus(g_auto ? L"Auto-skin ON: all windows will be skinned." : L"Auto-skin OFF.");
            SaveConfig(); InvalidateRect((HWND)l, NULL, FALSE); return 0;
        }
        switch (id) {
        case IDC_GAL: if (code == LBN_SELCHANGE) { int i = (int)SendMessageW(g_gal, LB_GETCURSEL, 0, 0); if (i >= 0 && i != g_themeIdx) { ApplyTheme(i); LabLoadFrom(i); SaveConfig(); } } break;
        case IDC_ISTYLE: {
            RECT r; POINT pt; int c; GetWindowRect(g_btnIStyle, &r); pt.x = r.left; pt.y = r.bottom;
            c = PopupChoose(pt, ISTYLE_NAMES, 6, g_iconOver + 1);
            if (c >= 0) { g_iconOver = c - 1; RebuildEff(); UpdateButtonTexts(); ApplyLookAll(); SaveConfig(); }
            break; }
        case IDC_PRESETS: {
            RECT r; POINT pt; int c; GetWindowRect(g_btnPresets, &r); pt.x = r.left; pt.y = r.bottom;
            c = PresetPick(pt, 1, 0, g_presetAll);
            if (c >= 0) { g_presetAll = c; RebuildEff(); UpdateButtonTexts(); ApplyLookAll(); SaveConfig(); SetStatus(c ? L"Preset %d applied to all buttons." : L"Presets: using each theme's own icons.", c); }
            break; }
        case IDC_COLOR: PickAccent(); SaveConfig(); break;
        case IDC_RESET: g_hasCol = 0; g_iconOver = -1; g_presetAll = 0; UpdateButtonTexts(); RebuildEff(); ApplyTheme(g_themeIdx); SaveConfig(); break;
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
        case IDC_O_RUN: RunProgram(); break;
        case IDC_O_NEVER: NeverSkinApp(); break;
        case IDC_O_CLEAR: wcscpy(g_exclude, L";explorer.exe;"); g_nNoSkin = 0; SetStatus(L"Never-skin list cleared."); SaveConfig(); break;
        case IDC_O_UNSKIN: { int i; for (i = 0; i < g_nskins; i++) AddNoSkin(g_skins[i]->target); SkinRemoveAll(); SetStatus(L"All skins removed (they stay off until the window reopens)."); break; }
        case IDC_O_REAPPLY: ApplySystemLook(CurTheme()); SetStatus(L"System colours and fonts re-applied."); break;
        case IDC_O_RESTORE: SetStatus(RestoreSystemLook() ? L"Original system colours/fonts restored." : L"No backup found yet."); break;
        }
        return 0; }
    case WM_SHOWAPP: ShowWindow(h, SW_SHOWNORMAL); SetForegroundWindow(h); return 0;
    case WM_QUITAPP: SaveConfig(); KillTimer(h, TM_SYNC); SkinRemoveAll(); DestroyWindow(h); return 0;
    case WM_CLOSE:
        if (g_bg) { SaveConfig(); ShowWindow(h, SW_HIDE); return 0; }
        SaveConfig(); KillTimer(h, TM_SYNC); SkinRemoveAll(); DestroyWindow(h); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

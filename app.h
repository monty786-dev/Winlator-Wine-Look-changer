/* app.h - window list, actions, theme application */
static HWND g_tabBtn[4], g_edSearch, g_btnRef, g_list, g_act[10], g_edRun, g_btnBrowse, g_btnGo,
            g_gal, g_btnIStyle, g_btnColor, g_btnReset, g_edLab, g_btnFrom, g_props,
            g_btnLSave, g_btnLUpd, g_btnLDel, g_btnLRev, g_tog[5], g_btnO[4];
static int g_tab = 0;

typedef struct { HWND h; wchar_t title[256], proc[64]; BOOL iconic, zoomed, top, skinned, shaded; } WinInfo;
static WinInfo g_wins[MAXWIN], g_tmp[MAXWIN];
static int g_nwins, g_ntmp;
static wchar_t g_filter[128];
static unsigned g_sig;

static void LowerCopy(wchar_t *d, const wchar_t *s, int n) {
    int i = 0; for (; s[i] && i < n - 1; i++) d[i] = (wchar_t)towlower(s[i]); d[i] = 0;
}
static BOOL CALLBACK EnumProc(HWND h, LPARAM lp) {
    wchar_t t[256], cls[64], lt[256], lf[128]; DWORD pid; LONG ex; WinInfo *w; Skin *e;
    (void)lp;
    if (g_ntmp >= MAXWIN) return FALSE;
    e = SkinFind(h);
    if (!IsWindowVisible(h) && !(e && e->shaded)) return TRUE;
    if (GetWindowTextW(h, t, 256) <= 0) return TRUE;
    GetWindowThreadProcessId(h, &pid);
    if (pid == GetCurrentProcessId()) return TRUE;
    ex = GetWindowLongW(h, GWL_EXSTYLE);
    if ((ex & WS_EX_TOOLWINDOW) && !(ex & WS_EX_APPWINDOW)) return TRUE;
    if (GetWindow(h, GW_OWNER) && !(ex & WS_EX_APPWINDOW)) return TRUE;
    cls[0] = 0; GetClassNameW(h, cls, 64);
    if (!wcscmp(cls, L"Progman") || !wcscmp(cls, L"Shell_TrayWnd") || !wcscmp(cls, L"WorkerW") || !wcscmp(t, L"Program Manager")) return TRUE;
    if (g_filter[0]) { LowerCopy(lt, t, 256); LowerCopy(lf, g_filter, 128); if (!wcsstr(lt, lf)) return TRUE; }
    w = &g_tmp[g_ntmp++];
    w->h = h; wcsncpy(w->title, t, 255); w->title[255] = 0; ProcNameCached(pid, w->proc, 64);
    w->iconic = IsIconic(h); w->zoomed = IsZoomed(h) || (e && e->state == 1);
    w->top = IsPinned(h); w->skinned = e != NULL; w->shaded = e && e->shaded;
    return TRUE;
}
static BOOL get_windows(void) {
    unsigned sig = 2166136261u; int i; const wchar_t *p;
    g_ntmp = 0; GetWindowTextW(g_edSearch, g_filter, 128);
    EnumWindows(EnumProc, 0);
    for (i = 0; i < g_ntmp; i++) {
        sig = (sig ^ (unsigned)(ULONG_PTR)g_tmp[i].h) * 16777619u;
        for (p = g_tmp[i].title; *p; p++) sig = (sig ^ (unsigned)*p) * 16777619u;
        sig = (sig ^ (unsigned)(g_tmp[i].iconic | g_tmp[i].zoomed << 1 | g_tmp[i].top << 2 | g_tmp[i].skinned << 3 | g_tmp[i].shaded << 4)) * 16777619u;
    }
    sig ^= (unsigned)g_ntmp;
    if (sig == g_sig && g_ntmp == g_nwins) return FALSE;
    g_sig = sig; return TRUE;
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
    for (i = 0; i < g_nwins; i++) { SendMessageW(g_list, LB_ADDSTRING, 0, (LPARAM)i); if (g_wins[i].h == sel) idx = i; }
    if (idx >= 0) SendMessageW(g_list, LB_SETCURSEL, idx, 0);
    if (top >= 0 && top < g_nwins) SendMessageW(g_list, LB_SETTOPINDEX, top, 0);
    SendMessageW(g_list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_list, NULL, FALSE);
}

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
    if (act == ACT_UNSKINALL) {
        int i; for (i = 0; i < g_nskins; i++) AddNoSkin(g_skins[i]->target);
        SkinRemoveAll(); SetStatus(L"All skins removed (auto-skin skips these windows)."); Refresh(TRUE); return;
    }
    if (!t) { SetStatus(L"Select a window in the list first."); return; }
    e = SkinFind(t);
    switch (act) {
    case ACT_SKIN: {
        static const wchar_t *msg[] = { L"Skin applied.", L"Window has no title bar to replace.", L"Too many skinned windows.",
                                        L"Already skinned.", L"Could not create overlay window." };
        int r = SkinApply(t); SetStatus(L"%ls", msg[r]); break; }
    case ACT_UNSKIN:
        if (e) { AddNoSkin(t); SkinDestroy(e, TRUE); SetStatus(L"Skin removed."); } else SetStatus(L"That window is not skinned.");
        break;
    case ACT_SHADE: if (e) SkinShade(e, !e->shaded); else SetStatus(L"Skin the window first to roll it up."); break;
    case ACT_TOP:   SetPin(t, !IsPinned(t)); SetStatus(IsPinned(t) ? L"Pinned on top." : L"Unpinned."); break;
    case ACT_MIN:   ShowWindow(t, SW_MINIMIZE); break;
    case ACT_MAX:   if (e) SkinToggleMax(e); else ShowWindow(t, IsZoomed(t) ? SW_RESTORE : SW_MAXIMIZE); break;
    case ACT_LEFT:  snap_window(t, 0); break;
    case ACT_RIGHT: snap_window(t, 1); break;
    case ACT_CLOSE: if (e && e->shaded) SkinShade(e, 0); PostMessageW(t, WM_CLOSE, 0, 0); break;
    case ACT_NEVER: {
        DWORD pid = 0; wchar_t exe[64]; GetWindowThreadProcessId(t, &pid); ProcNameCached(pid, exe, 64);
        AddExclude(exe); if (e) SkinDestroy(e, TRUE);
        SetStatus(L"%ls will never be skinned.", exe); break; }
    case ACT_KILL: {
        DWORD pid = 0; HANDLE ph; GetWindowThreadProcessId(t, &pid);
        if (MessageBoxW(g_main, L"Terminate this process?", APP_TITLE, MB_YESNO | MB_ICONWARNING) == IDYES) {
            if (e) SkinDestroy(e, FALSE);
            ph = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
            if (ph && TerminateProcess(ph, 1)) SetStatus(L"Process terminated."); else SetStatus(L"Could not terminate (error %lu).", GetLastError());
            if (ph) CloseHandle(ph);
        }
        break; }
    }
    Refresh(TRUE);
}

/* theme change: new font, new bar height, repaint every overlay */
static void ApplyLookAll(void) {
    int i, H = CurTheme()->height; HFONT old = g_ovFont;
    g_ovFont = MakeTitleFont(CurTheme());
    for (i = 0; i < g_nskins; i++) {
        Skin *e = g_skins[i]; RECT r;
        if (!IsWindow(e->target)) continue;
        if (e->H != H && GetWindowRect(e->target, &r)) {
            int ndy = H - e->H; e->H = H;
            if (e->state == 1) { RECT wa; GetWork(&wa); SkinMove(e, wa.left, wa.top + H, wa.right - wa.left, wa.bottom - wa.top - H); }
            else SetWindowPos(e->target, NULL, r.left, r.top + ndy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        e->lastW = -1;
        if (e->over) InvalidateRect(e->over, NULL, FALSE);
    }
    if (old) DeleteObject(old);
    SyncAll();
    if (g_main) InvalidateRect(g_main, NULL, FALSE);
    if (g_gal) InvalidateRect(g_gal, NULL, FALSE);
}
static void ApplyTheme(int idx) {
    if (idx < 0 || idx >= g_nthemes) return;
    g_themeIdx = idx; RebuildEff();
    if (g_sysApply) ApplySystemLook(CurTheme());
    ApplyLookAll();
    WritePrivateProfileStringW(L"cfg", L"theme", g_themes[idx].name, g_ini);
    SetStatus(L"Theme: %ls%ls", g_themes[idx].name, g_sysApply ? L"  (system colours + fonts updated)" : L"");
}

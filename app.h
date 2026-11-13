/* app.h - theme application helpers */
static HWND g_tabBtn[3], g_gal, g_btnIStyle, g_btnPresets, g_btnColor, g_btnReset, g_edLab, g_btnFrom, g_props,
            g_btnLSave, g_btnLUpd, g_btnLDel, g_btnLRev, g_tog[8], g_btnO[6];
static int g_tab = 0;

/* remove skins from every window of one exe (used by "Never skin an app") */
static void UnskinExe(const wchar_t *exe) {
    int i; wchar_t n[64]; DWORD pid;
    for (i = g_nskins - 1; i >= 0; i--) {
        pid = 0; GetWindowThreadProcessId(g_skins[i]->target, &pid); ProcNameCached(pid, n, 64);
        if (!_wcsicmp(n, exe)) { AddNoSkin(g_skins[i]->target); SkinDestroy(g_skins[i], TRUE); }
    }
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

/* mset.h - settings window: custom touch-friendly list (scroll by drag, toggles, sliders, pickers) */
enum { ST_TOG, ST_INT, ST_ENUM, ST_COL, ST_STR, ST_FILE, ST_ACT, ST_THEME, ST_FONT };
enum { A_FAV = 1, A_TILE, A_TB, A_TRAY, A_REMOVE, A_RVX, A_KBX, A_BACKUP, A_RESTORE, A_RESET, A_STAR, A_WLC, A_KBTEST, A_CURAPPLY };
enum { S_TB, S_VIS, S_RV, S_ST, S_CUR, S_KB, S_WL, S_PER, S_DK, S_APPS, S_BK };
static const wchar_t *SECS[] = { L"Taskbar", L"Visibility", L"Reveal bar", L"Start & panel", L"Cursor", L"Keyboard", L"Window look", L"Personalize", L"Desktop", L"Apps", L"Backup" };
#define NSECS 11
static const wchar_t *O_POS[] = { L"Bottom", L"Top", L"Left", L"Right" }, *O_AL[] = { L"Left", L"Middle", L"Right" },
    *O_DIV[] = { L"None", L"Thin", L"Medium", L"Bold" }, *O_FS[] = { L"Keep visible", L"Visible over app", L"Floating icon", L"Hide bar" },
    *O_AP[] = { L"Full screen", L"Maximized", L"Both" }, *O_RVP[] = { L"Bottom", L"Top" }, *O_RVA[] = { L"Blink", L"Pop-up", L"Slide from edge", L"Slide from left" },
    *O_SZ[] = { L"Normal", L"Medium", L"Large", L"Full screen" },
    *O_SI[] = { L"Windows flag", L"Windows 98 colours", L"Windows 7 orb", L"Windows 8 tiles", L"Ubuntu circle", L"Arch", L"Mint leaf", L"Xfce mouse", L"Cinnamon", L"KDE K", L"Grid dots", L"Menu lines", L"Diamond", L"Star", L"Heart", L"Disc", L"Letter S", L"Monty M" },
    *O_BM[] = { L"Theme", L"Colour", L"Transparent" }, *O_BG[] = { L"Theme", L"Colour", L"Picture" }, *O_EF[] = { L"None", L"Glass", L"Frosted glass", L"3D" },
    *O_IP[] = { L"Preset 1 thin", L"Preset 2 ring", L"Preset 3 disc", L"Preset 4 box", L"Preset 5 filled box", L"Preset 6 medium", L"Preset 7 ring", L"Preset 8 disc", L"Preset 9 box", L"Preset 10 filled box", L"Preset 11 bold", L"Preset 12 ring", L"Preset 13 disc", L"Preset 14 box", L"Preset 15 filled box" },
    *O_MD[] = { L"None", L"Dark", L"Light", L"AMOLED", L"Rose", L"Sapphire", L"Aero" }, *O_CN[] = { L"Top-left", L"Top-right", L"Bottom-left", L"Bottom-right" },
    *O_FT[] = { L"Crop to fill", L"Fit", L"Stretch", L"Center", L"Tile" },
    *FONTS2[] = { L"Segoe UI", L"Tahoma", L"Arial", L"Verdana", L"Trebuchet MS", L"DejaVu Sans", L"Ubuntu", L"Cantarell", L"Noto Sans", L"Courier New", L"Times New Roman", L"Helvetica" };

typedef struct { int sec; const wchar_t *label; int type; int *pi; wchar_t *ps; int lo, hi, step; const wchar_t *const *opts; int nopt; const wchar_t *key; int act; } Set;
#define TG(s,l,f)           { s, l, ST_TOG,  &g.f, NULL, 0, 1, 1, NULL, 0, L"" #f, 0 }
#define SL(s,l,f,lo,hi,st)  { s, l, ST_INT,  &g.f, NULL, lo, hi, st, NULL, 0, L"" #f, 0 }
#define EN(s,l,f,o,n)       { s, l, ST_ENUM, &g.f, NULL, 0, (n) - 1, 1, o, n, L"" #f, 0 }
#define CL(s,l,f)           { s, l, ST_COL,  &g.f, NULL, 0, 0, 0, NULL, 0, L"" #f, 0 }
#define SS(s,l,var,key)     { s, l, ST_STR,  NULL, var, 0, 0, 0, NULL, 0, key, 0 }
#define FI(s,l,var,key,flt) { s, l, ST_FILE, NULL, var, 0, 0, 0, NULL, 0, key, flt }
#define AC(s,l,id)          { s, l, ST_ACT,  NULL, NULL, 0, 0, 0, NULL, 0, NULL, id }
static Set SETS[] = {
    EN(S_TB, L"Position", tbPos, O_POS, 4), EN(S_TB, L"Alignment", tbAlign, O_AL, 3), SL(S_TB, L"Taskbar height", tbHeight, 24, 90, 2),
    TG(S_TB, L"Auto-fit width to running apps", tbAutoW), SL(S_TB, L"Icon size", tbIcon, 12, 48, 2), TG(S_TB, L"Show window titles on buttons", tbTitles),
    TG(S_TB, L"Rounded corners", tbRound), SL(S_TB, L"Detach from edge", tbDetach, 0, 40, 1), TG(S_TB, L"Custom taskbar colour", tbUseCol), CL(S_TB, L"Taskbar colour", tbCol),
    SL(S_TB, L"Taskbar opacity", tbOpacity, 30, 100, 5), TG(S_TB, L"Running app underline", tbUnder), SL(S_TB, L"Underline height", tbUnderH, 1, 8, 1),
    TG(S_TB, L"Underline colour from app icon", tbUnderAdapt), EN(S_TB, L"Divider line", tbDivider, O_DIV, 4), TG(S_TB, L"Show clock", tbClock), TG(S_TB, L"Show day name", tbDay),
    TG(S_TB, L"Show-desktop button", tbDesk), TG(S_TB, L"App tray", tbTray), TG(S_TB, L"Hover preview of running apps", tbPreview),
    SL(S_TB, L"Preview width", pvW, 120, 480, 10), SL(S_TB, L"Preview height", pvH, 80, 360, 10), TG(S_TB, L"Classic original look of the theme", tbClassic),
    TG(S_VIS, L"Hide Winlator's built-in taskbar", hideWine), EN(S_VIS, L"Over full-screen apps", fsMode, O_FS, 4), EN(S_VIS, L"Applies to", fsApplies, O_AP, 3),
    TG(S_VIS, L"Start with the container", autoStart), SL(S_VIS, L"Startup delay (seconds)", startDelay, 0, 60, 1),
    TG(S_RV, L"Reveal bar", rvOn), TG(S_RV, L"Auto-hide taskbar always (normal + desktop mode)", rvAlways), CL(S_RV, L"Bar colour", rvCol), EN(S_RV, L"Bar position", rvPos, O_RVP, 2),
    EN(S_RV, L"Bar alignment", rvAlign, O_AL, 3), TG(S_RV, L"L-shaped corners", rvL), SL(S_RV, L"Bar opacity", rvOpacity, 10, 100, 5), SL(S_RV, L"Bar height", rvH, 2, 24, 1),
    SL(S_RV, L"Bar width", rvW, 20, 1600, 20), SL(S_RV, L"Taskbar visible for (seconds)", rvSecs, 1, 30, 1), EN(S_RV, L"Reveal animation", rvAnim, O_RVA, 4), AC(S_RV, L"Add exception app...", A_RVX),
    EN(S_ST, L"Start panel size", stSize, O_SZ, 4), EN(S_ST, L"Start button icon", stIcon, O_SI, 18), TG(S_ST, L"Custom start button colour", stUseCol), CL(S_ST, L"Start button colour", stCol),
    SS(S_ST, L"User name", S_user, L"user"), FI(S_ST, L"Profile picture", S_pic, L"pic", 0), EN(S_ST, L"Vertical bar background", stBarMode, O_BM, 3), CL(S_ST, L"Vertical bar colour", stBarCol),
    EN(S_ST, L"Panel background", stBgMode, O_BG, 3), CL(S_ST, L"Panel colour", stBgCol), FI(S_ST, L"Panel picture", S_stbg, L"stbg", 0), SL(S_ST, L"Panel opacity", stOpacity, 30, 100, 5),
    TG(S_ST, L"Different colour per tile", stTileCol), SL(S_ST, L"Tile opacity", stTileAlpha, 30, 100, 5), TG(S_ST, L"Tile move animation", stTileAnim), SL(S_ST, L"Tile size", stTilePx, 40, 160, 4),
    TG(S_ST, L"Show app list", showApps), TG(S_ST, L"Show pinned tiles", showTiles),
    TG(S_CUR, L"Custom cursor", curOn), TG(S_CUR, L"Hide cursor", curHide), SL(S_CUR, L"Cursor size", curSize, 16, 64, 2), CL(S_CUR, L"Cursor colour", curCol), CL(S_CUR, L"Cursor border", curBorder),
    FI(S_CUR, L"Normal (.cur .ani .png .ico)", S_cur[0], L"cur0", 1), FI(S_CUR, L"Working in background", S_cur[1], L"cur1", 1), FI(S_CUR, L"Busy / wait", S_cur[2], L"cur2", 1),
    FI(S_CUR, L"Link / grab", S_cur[3], L"cur3", 1), FI(S_CUR, L"Text select", S_cur[4], L"cur4", 1), FI(S_CUR, L"Precision", S_cur[5], L"cur5", 1), FI(S_CUR, L"Move", S_cur[6], L"cur6", 1),
    FI(S_CUR, L"Unavailable", S_cur[7], L"cur7", 1), AC(S_CUR, L"Apply cursor now", A_CURAPPLY),
    TG(S_KB, L"Open Android keyboard on text fields", kbOn), TG(S_KB, L"Only after a click", kbClick), SL(S_KB, L"Delay (ms)", kbDelay, 0, 2000, 50),
    SS(S_KB, L"Bridge command (runs with show/hide)", S_kbCmd, L"kbcmd"), AC(S_KB, L"Add exception app...", A_KBX), AC(S_KB, L"Send test signal", A_KBTEST),
    TG(S_WL, L"Window look changer", wlOn), SL(S_WL, L"Title bar transparency", wlAlpha, 30, 100, 5), TG(S_WL, L"Apply to all window elements", wlAll), AC(S_WL, L"Open window look changer...", A_WLC),
    { S_PER, L"Theme", ST_THEME, NULL, NULL, 0, 0, 0, NULL, 0, L"theme", 0 }, EN(S_PER, L"Effect", pEffect, O_EF, 4), SL(S_PER, L"Blur / frost amount", pBlur, 0, 100, 5),
    TG(S_PER, L"Custom accent colour", pAccOn), CL(S_PER, L"Accent colour", pAcc), SL(S_PER, L"Glass shine", pShine, 0, 100, 5), SL(S_PER, L"Glass gloss", pGloss, 0, 100, 5),
    TG(S_PER, L"Shadow", pShadow), TG(S_PER, L"Round all corners", pRound), TG(S_PER, L"Effect on taskbar", effTb), TG(S_PER, L"Effect on start panel", effSt), TG(S_PER, L"Effect on preview", effPv),
    TG(S_PER, L"Effect on window title bar", effWin), TG(S_PER, L"Effect on installer", effInst), EN(S_PER, L"Icon preset", pIcon, O_IP, 15), EN(S_PER, L"Mode", pMode, O_MD, 7),
    TG(S_PER, L"Icons follow theme colour", pTint), TG(S_PER, L"Fill icon shapes with colour", pFill), { S_PER, L"Font", ST_FONT, NULL, NULL, 0, 0, 0, NULL, 0, L"font", 0 },
    SL(S_PER, L"Font size", fontSize, 8, 24, 1), SL(S_PER, L"UI scale", uiScale, 70, 170, 5),
    TG(S_DK, L"Desktop mode", dkOn), SL(S_DK, L"Icon grid size", dkGrid, 56, 160, 4), TG(S_DK, L"Snap icons to grid", dkSnap), EN(S_DK, L"Shortcut start corner", dkCorner, O_CN, 4),
    TG(S_DK, L"Show desktop icons", dkIcons), TG(S_DK, L"Clock widget", dkClock), TG(S_DK, L"Apps widget", dkApps), FI(S_DK, L"Wallpaper image", S_wall, L"wall", 0),
    EN(S_DK, L"Wallpaper fit", dkWallMode, O_FT, 5), CL(S_DK, L"Background colour", dkBgCol), TG(S_DK, L"Lock icon positions", dkLock),
    AC(S_APPS, L"Add to Start app list...", A_FAV), AC(S_APPS, L"Add pinned tile...", A_TILE), AC(S_APPS, L"Add to taskbar...", A_TB), AC(S_APPS, L"Add to app tray...", A_TRAY), AC(S_APPS, L"Remove an app...", A_REMOVE),
    AC(S_BK, L"Back up settings", A_BACKUP), AC(S_BK, L"Restore settings", A_RESTORE), AC(S_BK, L"Reset all settings", A_RESET), AC(S_BK, L"Star / support on GitHub", A_STAR),
};
#define NSETS ((int)(sizeof SETS / sizeof SETS[0]))

static int g_setSec, g_setScroll, g_setDrag, g_setMoved, g_setScroll0, g_setSlide = -1; static POINT g_setDP; static int g_setList[NSETS], g_setN;
static int SetRowH(void) { return ScaleI(54); }
static int SetBarH(void) { return ScaleI(50); }

/* ---- small helpers ---- */
static void ApplyAll(void) {
    int ti = FindTheme(S_theme); if (ti >= 0) g_themeIdx = ti;
    g_hasCol = 0; RebuildEff(); BuildPal(); TbRelayout();
    if (g_st && g_stOpen) { StPosition(); InvalidateRect(g_st, NULL, FALSE); }
    if (g_dk && g.dkOn) InvalidateRect(g_dk, NULL, FALSE);
    if (g_set) InvalidateRect(g_set, NULL, FALSE);
}
static int MPopup(POINT pt, const wchar_t *const *items, int n, int checked) {
    HMENU mn = CreatePopupMenu(); int i, cmd;
    if (!mn) return -1;
    for (i = 0; i < n; i++) AppendMenuW(mn, MF_STRING | (i == checked ? MF_CHECKED : 0) | (i > 0 && i % 12 == 0 ? MF_MENUBREAK : 0), (UINT_PTR)(i + 1), items[i]);
    cmd = TrackPopupMenu(mn, TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, g_set, NULL);
    DestroyMenu(mn); return cmd > 0 ? cmd - 1 : -1;
}
static wchar_t g_inBuf[256]; static int g_inDone;
static LRESULT CALLBACK InProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_COMMAND) { if (LOWORD(w) == 1) { GetWindowTextW(GetDlgItem(h, 2), g_inBuf, 256); g_inDone = 1; } else if (LOWORD(w) == 3) g_inDone = 2; return 0; }
    if (m == WM_CLOSE) { g_inDone = 2; return 0; }
    return DefWindowProcW(h, m, w, l);
}
static BOOL MInput(const wchar_t *title, wchar_t *buf, int n) {
    HWND d, e; MSG msg; int sw = GetSystemMetrics(SM_CXSCREEN), W = ScaleI(380); g_inDone = 0;
    d = CreateWindowExW(WS_EX_TOPMOST, L"MontyIn", title, WS_POPUP | WS_CAPTION | WS_VISIBLE, (sw - W) / 2, ScaleI(120), W, ScaleI(150), g_set, NULL, g_inst, NULL);
    if (!d) return FALSE;
    e = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", buf, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 10, 10, W - 30, ScaleI(34), d, (HMENU)(INT_PTR)2, g_inst, NULL);
    CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE, 10, ScaleI(54), ScaleI(150), ScaleI(44), d, (HMENU)(INT_PTR)1, g_inst, NULL);
    CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE, ScaleI(170), ScaleI(54), ScaleI(150), ScaleI(44), d, (HMENU)(INT_PTR)3, g_inst, NULL);
    SetFocus(e); SendMessageW(e, EM_SETSEL, 0, -1);
    while (!g_inDone && GetMessageW(&msg, NULL, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    DestroyWindow(d);
    if (g_inDone == 1) { wcsncpy(buf, g_inBuf, n - 1); buf[n - 1] = 0; return TRUE; }
    return FALSE;
}
static void SetBuild(void) {
    int i; g_setN = 0;
    for (i = 0; i < NSETS; i++) if (SETS[i].sec == g_setSec) g_setList[g_setN++] = i;
}
static void Changed(const Set *s, BOOL fin) {
    const wchar_t *k = s->key;
    if (fin && k) { if (s->pi) MWr(L"cfg", k, *s->pi); else if (s->ps) MWrS(k, s->ps); }
    if (k && fin) {
        if (!wcscmp(k, L"autoStart")) SetAutostart(g.autoStart);
        else if (!wcsncmp(k, L"cur", 3)) ApplyCursor();
        else if (!wcsncmp(k, L"wl", 2)) SkinBridge();
        else if (!wcscmp(k, L"dkOn")) { DesktopToggle(); }
        else if (!wcscmp(k, L"theme")) SkinBridge();
    }
    ApplyAll();
}
static void DoAct(int id) {
    wchar_t f[MAX_PATH]; POINT pt; static const wchar_t *LN[4] = { L"Start app list", L"Pinned tiles", L"Taskbar", L"App tray" }; int l, i;
    GetCursorPos(&pt);
    switch (id) {
    case A_FAV: case A_TILE: case A_TB: case A_TRAY:
        if (MBrowse(g_set, f, L"Programs and shortcuts\0*.exe;*.lnk;*.bat\0All files\0*.*\0")) {
            ListAdd(id == A_FAV ? L_FAV : (id == A_TILE ? L_TILE : (id == A_TB ? L_TB : L_TRAY)), f); g_stDirty = 1; ApplyAll(); SetStatus(L"Added.");
        } break;
    case A_REMOVE: {
        static const int MAPL[4] = { L_FAV, L_TILE, L_TB, L_TRAY }; static wchar_t nm[MAXL][64]; const wchar_t *it[MAXL];
        l = MPopup(pt, LN, 4, -1); if (l < 0) break; l = MAPL[l];
        for (i = 0; i < g_nL[l]; i++) { BaseName(g_L[l][i].p, nm[i], 64); it[i] = nm[i]; }
        if (g_nL[l] == 0) { SetStatus(L"That list is empty."); break; }
        i = MPopup(pt, it, g_nL[l], -1); if (i >= 0) { ListRemove(l, i); g_stDirty = 1; ApplyAll(); SetStatus(L"Removed."); }
        break; }
    case A_RVX: case A_KBX:
        if (MBrowse(g_set, f, L"Programs\0*.exe\0All files\0*.*\0")) { ListAdd(id == A_RVX ? L_RVX : L_KBX, f); SetStatus(L"Exception added."); } break;
    case A_BACKUP: SetStatus(CfgBackup() ? L"Settings backed up." : L"Backup failed."); break;
    case A_RESTORE: if (CfgRestore()) { ApplyAll(); ApplyCursor(); SetStatus(L"Settings restored."); } else SetStatus(L"No backup found."); break;
    case A_RESET: if (MessageBoxW(g_set, L"Reset ALL settings to defaults?", APP_TITLE, MB_YESNO | MB_ICONWARNING) == IDYES) { CfgReset(); ApplyAll(); ApplyCursor(); SetStatus(L"Settings reset."); } break;
    case A_STAR: ShellExecuteW(NULL, L"open", L"https://github.com/monty786-dev/Winlator-Wine-Look-changer", NULL, NULL, SW_SHOWNORMAL); break;
    case A_WLC: OpenSkinUi(); break;
    case A_KBTEST: KbdSignal(1); SetStatus(L"Signal written: %lskbd.show", g_dir); break;
    case A_CURAPPLY: ApplyCursor(); if (!g_curFail) SetStatus(L"Cursor applied."); break;
    }
}

/* ---- paint ---- */
static void SetPaint(HDC dc, int w, int h) {
    RECT all, r, q, bar; int i, rh = SetRowH(), bh = SetBarH(); wchar_t v[96];
    SetRect(&all, 0, 0, w, h); FillRectC(dc, 0, 0, w, h, P.panel);
    SetRect(&bar, 0, 0, w, bh); PaintSurface(dc, &bar, P.barA, P.barB, g.effWin);
    SelectObject(dc, g_fTbB);
    q = bar; q.right = bh; Txt(dc, L"<", &q, DT_SINGLELINE | DT_VCENTER | DT_CENTER, P.txt);
    q = bar; q.left = w - bh; Txt(dc, L">", &q, DT_SINGLELINE | DT_VCENTER | DT_CENTER, P.txt);
    q = bar; q.left = bh; q.right = w - bh; Txt(dc, SECS[g_setSec], &q, DT_SINGLELINE | DT_VCENTER | DT_CENTER, P.txt);
    SelectObject(dc, g_fTb);
    {   HRGN rg = CreateRectRgn(0, bh, w, h - ScaleI(26)); SelectClipRgn(dc, rg);
        for (i = 0; i < g_setN; i++) {
            const Set *s = &SETS[g_setList[i]]; int y = bh + i * rh - g_setScroll;
            if (y + rh < bh || y > h) continue;
            SetRect(&r, 0, y, w, y + rh); FillRectC(dc, 0, y + rh - 1, w, y + rh, Blend(P.panel, P.panelTxt, 14));
            q = r; q.left = 16; q.right = w - 16;
            if (s->type == ST_ACT) { RECT b2 = r; InflateRect(&b2, -12, -6); RRect(dc, &b2, 12, P.acc, P.acc); Txt(dc, s->label, &b2, DT_SINGLELINE | DT_VCENTER | DT_CENTER, P.accTxt); continue; }
            if (s->type == ST_INT) {
                int val = *s->pi, tl = 20, tr = w - 20, ty = y + rh - ScaleI(14), kx = tl + (tr - tl) * (val - s->lo) / (s->hi - s->lo ? s->hi - s->lo : 1);
                q.bottom = y + rh / 2 + 4; Txt(dc, s->label, &q, DT_SINGLELINE | DT_VCENTER, P.panelTxt);
                _snwprintf(v, 95, L"%d", val); v[95] = 0; Txt(dc, v, &q, DT_SINGLELINE | DT_VCENTER | DT_RIGHT, P.panelDim);
                FillRectC(dc, tl, ty - 2, tr, ty + 2, Blend(P.panel, P.panelTxt, 25)); FillRectC(dc, tl, ty - 2, kx, ty + 2, P.acc); Disc(dc, kx, ty, ScaleI(9), P.acc, P.acc);
                continue;
            }
            { RECT lab = q; lab.right = w - ScaleI(190); Txt(dc, s->label, &lab, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS, P.panelTxt); }
            if (s->type == ST_TOG) {
                RECT p2; int on = *s->pi; p2.right = w - 16; p2.left = p2.right - 46; p2.top = (r.top + r.bottom) / 2 - 12; p2.bottom = p2.top + 24;
                RRect(dc, &p2, 24, on ? P.acc : Blend(P.panel, P.panelTxt, 30), on ? P.acc : Blend(P.panel, P.panelTxt, 30));
                Disc(dc, on ? p2.right - 12 : p2.left + 12, (p2.top + p2.bottom) / 2, 9, W_, W_);
            } else if (s->type == ST_COL) {
                RECT sq; sq.right = w - 16; sq.left = sq.right - 44; sq.top = (r.top + r.bottom) / 2 - 12; sq.bottom = sq.top + 24; RRect(dc, &sq, 8, (COLORREF)*s->pi, P.panelDim);
            } else {
                const wchar_t *txt = L"";
                if (s->type == ST_ENUM) txt = s->opts[*s->pi < s->nopt ? *s->pi : 0];
                else if (s->type == ST_THEME) txt = S_theme; else if (s->type == ST_FONT) txt = S_font;
                else if (s->ps) { const wchar_t *b = wcsrchr(s->ps, L'\\'); txt = s->ps[0] ? (b ? b + 1 : s->ps) : L"(none)"; }
                q.left = w - ScaleI(200); Txt(dc, txt, &q, DT_SINGLELINE | DT_VCENTER | DT_RIGHT | DT_END_ELLIPSIS, P.acc);
            }
        }
        SelectClipRgn(dc, NULL); if (rg) DeleteObject(rg);
    }
    (void)all;
}

/* ---- input ---- */
static int SetRowAt(int y) { int i = (y - SetBarH() + g_setScroll) / SetRowH(); return (y >= SetBarH() && i >= 0 && i < g_setN) ? i : -1; }
static void SetSlide(int row, int x) {
    const Set *s = &SETS[g_setList[row]]; int w, tl = 20, v; RECT rc; GetClientRect(g_set, &rc); w = rc.right - 40; if (w < 1) w = 1;
    v = s->lo + (x - tl) * (s->hi - s->lo) / w; v = s->lo + (v - s->lo + s->step / 2) / s->step * s->step;
    if (v < s->lo) v = s->lo; if (v > s->hi) v = s->hi; *s->pi = v; Changed(s, FALSE);
}
static void SetClick(int row, int x, int y) {
    Set *s = &SETS[g_setList[row]]; POINT pt; RECT rc; GetClientRect(g_set, &rc); pt.x = x; pt.y = y; ClientToScreen(g_set, &pt);
    switch (s->type) {
    case ST_TOG: *s->pi = !*s->pi; Changed(s, TRUE); break;
    case ST_ENUM: { int c = MPopup(pt, s->opts, s->nopt, *s->pi); if (c >= 0) { *s->pi = c; Changed(s, TRUE); } break; }
    case ST_COL: { COLORREF c = (COLORREF)*s->pi; if (PickColor(&c)) { *s->pi = (int)c; Changed(s, TRUE); } break; }
    case ST_THEME: { static const wchar_t *nm[MAXTHEME]; int i, c; for (i = 0; i < g_nthemes; i++) nm[i] = g_themes[i].name; c = MPopup(pt, nm, g_nthemes, g_themeIdx);
        if (c >= 0) { wcsncpy(S_theme, g_themes[c].name, 47); MWrS(L"theme", S_theme); Changed(s, TRUE); } break; }
    case ST_FONT: { int c = MPopup(pt, FONTS2, 12, -1); if (c >= 0) { wcsncpy(S_font, FONTS2[c], 31); MWrS(L"font", S_font); ApplyAll(); } break; }
    case ST_STR: { wchar_t b[MAX_PATH]; wcsncpy(b, s->ps, MAX_PATH - 1); b[MAX_PATH - 1] = 0; if (MInput(s->label, b, MAX_PATH)) { wcscpy(s->ps, b); Changed(s, TRUE); } break; }
    case ST_FILE: {
        static const wchar_t *it[2] = { L"Choose file...", L"Clear" }; int c = MPopup(pt, it, 2, -1); wchar_t f[MAX_PATH];
        if (c == 0 && MBrowse(g_set, f, s->act == 1 ? L"Cursors / images\0*.cur;*.ani;*.png;*.ico;*.bmp\0All files\0*.*\0" : L"Images\0*.png;*.jpg;*.jpeg;*.bmp;*.gif\0All files\0*.*\0")) { wcsncpy(s->ps, f, MAX_PATH - 1); Changed(s, TRUE); }
        else if (c == 1) { s->ps[0] = 0; Changed(s, TRUE); }
        break; }
    case ST_ACT: DoAct(s->act); break;
    }
    (void)rc;
}
static void SetClamp(int h) {
    int mx = g_setN * SetRowH() - (h - SetBarH() - ScaleI(26)); if (mx < 0) mx = 0;
    if (g_setScroll > mx) g_setScroll = mx; if (g_setScroll < 0) g_setScroll = 0;
}
static LRESULT CALLBACK SetProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: g_set = h; g_status = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX, 0, 0, 10, 10, h, (HMENU)(INT_PTR)99, g_inst, NULL);
        SendMessageW(g_status, WM_SETFONT, (WPARAM)g_fSmall, TRUE); SetBuild(); return 0;
    case WM_SIZE: { RECT rc; GetClientRect(h, &rc); MoveWindow(g_status, 8, rc.bottom - ScaleI(24), rc.right - 16, ScaleI(22), TRUE); SetClamp(rc.bottom); return 0; }
    case WM_ERASEBKGND: return 1;
    case WM_CTLCOLORSTATIC: { static HBRUSH br; static COLORREF last = (COLORREF)-1; if (P.panel != last) { if (br) DeleteObject(br); br = CreateSolidBrush(P.panel); last = P.panel; }
        SetTextColor((HDC)w, P.panelDim); SetBkColor((HDC)w, P.panel); return (LRESULT)br; }
    case WM_PAINT: {
        PAINTSTRUCT ps; RECT rc; HDC dc = BeginPaint(h, &ps), mem; HBITMAP bm, ob; GetClientRect(h, &rc);
        mem = CreateCompatibleDC(dc); bm = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
        if (mem && bm) { ob = (HBITMAP)SelectObject(mem, bm); SetPaint(mem, rc.right, rc.bottom); BitBlt(dc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY); SelectObject(mem, ob); }
        if (bm) DeleteObject(bm); if (mem) DeleteDC(mem);
        EndPaint(h, &ps); return 0; }
    case WM_MOUSEWHEEL: { RECT rc; GetClientRect(h, &rc); g_setScroll -= (short)HIWORD(w) / 120 * SetRowH(); SetClamp(rc.bottom); InvalidateRect(h, NULL, FALSE); return 0; }
    case WM_LBUTTONDOWN: {
        int x = (short)LOWORD(l), y = (short)HIWORD(l), row = SetRowAt(y); SetCapture(h);
        g_setDP.x = x; g_setDP.y = y; g_setMoved = 0; g_setScroll0 = g_setScroll; g_setDrag = 1; g_setSlide = -1;
        if (row >= 0 && SETS[g_setList[row]].type == ST_INT) {
            int ry = SetBarH() + row * SetRowH() - g_setScroll;
            if (y > ry + SetRowH() / 2 - 4) { g_setSlide = row; g_setDrag = 0; SetSlide(row, x); InvalidateRect(h, NULL, FALSE); }
        }
        return 0; }
    case WM_MOUSEMOVE:
        if (w & MK_LBUTTON) {
            int x = (short)LOWORD(l), y = (short)HIWORD(l);
            if (g_setSlide >= 0) { SetSlide(g_setSlide, x); InvalidateRect(h, NULL, FALSE); }
            else if (g_setDrag) { RECT rc; int dy = y - g_setDP.y; if (dy * dy > 64) g_setMoved = 1; if (g_setMoved) { GetClientRect(h, &rc); g_setScroll = g_setScroll0 - dy; SetClamp(rc.bottom); InvalidateRect(h, NULL, FALSE); } }
        }
        return 0;
    case WM_LBUTTONUP: {
        int x = (short)LOWORD(l), y = (short)HIWORD(l); if (GetCapture() == h) ReleaseCapture();
        if (g_setSlide >= 0) { Changed(&SETS[g_setList[g_setSlide]], TRUE); g_setSlide = -1; g_setDrag = 0; return 0; }
        if (g_setDrag && !g_setMoved) {
            RECT rc; GetClientRect(h, &rc);
            if (y < SetBarH()) {
                int bh = SetBarH();
                if (x < bh) g_setSec = (g_setSec + NSECS - 1) % NSECS; else if (x > rc.right - bh) g_setSec = (g_setSec + 1) % NSECS;
                else { POINT pt; int c; pt.x = x; pt.y = y; ClientToScreen(h, &pt); c = MPopup(pt, SECS, NSECS, g_setSec); if (c < 0) { g_setDrag = 0; return 0; } g_setSec = c; }
                g_setScroll = 0; SetBuild(); InvalidateRect(h, NULL, FALSE);
            } else { int row = SetRowAt(y); if (row >= 0) SetClick(row, x, y); }
        }
        g_setDrag = 0; return 0; }
    case WM_CLOSE: ShowWindow(h, SW_HIDE); SaveCfg(); return 0;
    }
    return DefWindowProcW(h, m, w, l);
}
static void OpenSettings(void) {
    if (!g_set) return;
    SetBuild(); ShowWindow(g_set, SW_SHOWNORMAL); SetForegroundWindow(g_set); InvalidateRect(g_set, NULL, FALSE);
}

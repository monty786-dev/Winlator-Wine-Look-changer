/* mtb1.h - taskbar state, running-window list, layout engine, painting */
static HWND g_tb, g_pv, g_rv, g_fl, g_st, g_dk, g_set, g_msg;
static HFONT g_fTb, g_fTbB;
static int g_stOpen;

enum { IT_START, IT_PIN, IT_RUN, IT_DIV, IT_TRAY, IT_CLOCK, IT_DESK };
typedef struct { HWND h; wchar_t title[96]; HICON ic; wchar_t exe[40]; COLORREF und; BOOL active; DWORD icTick; } RunItem;
typedef struct { int type, idx, pos, len; } Item;
static RunItem g_run[64]; static int g_nrun;
static Item g_it[220]; static int g_nit;
static RECT g_tbRect;                 /* screen rectangle of the taskbar when shown */
static int g_tbHoriz = 1, g_tbHot = -1, g_tbDown = -1, g_tbThick;
static unsigned g_runSig;

static void MakeFonts(void) {
    HFONT a = g_fTb, b = g_fTbB;
    int px = ScaleI(g.fontSize) * 4 / 3;
    g_fTb = MakeFont(S_font, px, 0); g_fTbB = MakeFont(S_font, px, 1);
    if (!g_fTb) g_fTb = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (!g_fTbB) g_fTbB = g_fTb;
    if (a && a != g_fUi) DeleteObject(a);
    if (b && b != a) DeleteObject(b);
}

/* ---- helpers on windows ---- */
typedef struct { DWORD pid; wchar_t exe[40]; } PE;
static PE g_pe[48]; static int g_npe;
static void MWinExe(HWND h, wchar_t *out, int n) {
    DWORD pid = 0; HANDLE ph; int i; out[0] = 0;
    GetWindowThreadProcessId(h, &pid);
    for (i = 0; i < g_npe; i++) if (g_pe[i].pid == pid) { wcsncpy(out, g_pe[i].exe, n - 1); out[n - 1] = 0; return; }
    ph = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (ph) {
        wchar_t full[MAX_PATH]; DWORD sz = MAX_PATH;
        if (QueryFullProcessImageNameW(ph, 0, full, &sz)) { wchar_t *b = wcsrchr(full, L'\\'); wcsncpy(out, b ? b + 1 : full, n - 1); out[n - 1] = 0; }
        CloseHandle(ph);
    }
    if (g_npe >= 48) g_npe = 0;
    g_pe[g_npe].pid = pid; wcsncpy(g_pe[g_npe].exe, out, 39); g_pe[g_npe].exe[39] = 0; g_npe++;
}
static BOOL ListHasExe(int l, const wchar_t *exe) {
    int i; for (i = 0; i < g_nL[l]; i++) { const wchar_t *b = wcsrchr(g_L[l][i].p, L'\\'); b = b ? b + 1 : g_L[l][i].p; if (!_wcsicmp(b, exe)) return TRUE; }
    return FALSE;
}
static HICON MWinIcon(HWND t) {
    DWORD_PTR r = 0;
    if (SendMessageTimeoutW(t, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG, 40, &r) && r) return (HICON)r;
    if (SendMessageTimeoutW(t, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 40, &r) && r) return (HICON)r;
    r = GetClassLongPtrW(t, GCLP_HICON); if (r) return (HICON)r;
    r = GetClassLongPtrW(t, GCLP_HICONSM); if (r) return (HICON)r;
    return LoadIconW(NULL, IDI_APPLICATION);
}
/* average colour of an icon: underline can adapt to the app */
static COLORREF IconAvg(HICON ic, COLORREF fallback) {
    HDC dc = CreateCompatibleDC(NULL); BITMAPINFO bi; void *bits = NULL; HBITMAP bm, ob; COLORREF r = fallback;
    if (!dc) return fallback;
    memset(&bi, 0, sizeof bi); bi.bmiHeader.biSize = sizeof bi.bmiHeader; bi.bmiHeader.biWidth = 16; bi.bmiHeader.biHeight = -16;
    bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
    bm = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (bm && bits) {
        BYTE *p = (BYTE *)bits; int i, n = 0; long sr = 0, sg = 0, sb = 0;
        ob = (HBITMAP)SelectObject(dc, bm);
        for (i = 0; i < 256; i++) { p[i * 4] = 255; p[i * 4 + 1] = 0; p[i * 4 + 2] = 255; p[i * 4 + 3] = 0; }
        DrawIconEx(dc, 0, 0, ic, 16, 16, 0, NULL, DI_NORMAL);
        for (i = 0; i < 256; i++) {
            int b = p[i * 4], gg = p[i * 4 + 1], rr = p[i * 4 + 2], mx, mn;
            if (b == 255 && gg == 0 && rr == 255) continue;
            mx = rr > gg ? (rr > b ? rr : b) : (gg > b ? gg : b); mn = rr < gg ? (rr < b ? rr : b) : (gg < b ? gg : b);
            if (mx - mn < 40 || mx < 60) continue;                       /* skip greys / near black */
            sr += rr; sg += gg; sb += b; n++;
        }
        if (n > 4) r = RGB((int)(sr / n), (int)(sg / n), (int)(sb / n));
        SelectObject(dc, ob);
    }
    if (bm) DeleteObject(bm);
    DeleteDC(dc);
    return r;
}

/* ---- running windows ---- */
static RunItem g_tmpRun[64]; static int g_ntmp2;
static BOOL CALLBACK RunEnum(HWND h, LPARAM lp) {
    wchar_t t[96], cls[48]; LONG ex; DWORD pid; RunItem *r; (void)lp;
    if (g_ntmp2 >= 64 || !IsWindowVisible(h)) return TRUE;
    ex = GetWindowLongW(h, GWL_EXSTYLE);
    if ((ex & WS_EX_TOOLWINDOW) && !(ex & WS_EX_APPWINDOW)) return TRUE;
    if (GetWindow(h, GW_OWNER) && !(ex & WS_EX_APPWINDOW)) return TRUE;
    if (GetWindowTextW(h, t, 96) <= 0) return TRUE;
    GetWindowThreadProcessId(h, &pid); if (pid == GetCurrentProcessId()) return TRUE;
    cls[0] = 0; GetClassNameW(h, cls, 48);
    if (!wcscmp(cls, L"Progman") || !wcscmp(cls, L"WorkerW") || !wcscmp(cls, L"Shell_TrayWnd") || !wcscmp(t, L"Program Manager")) return TRUE;
    r = &g_tmpRun[g_ntmp2++]; memset(r, 0, sizeof *r); r->h = h; wcscpy(r->title, t); MWinExe(h, r->exe, 40);
    return TRUE;
}
/* returns TRUE when the visible list changed */
static BOOL RefreshRun(void) {
    int i, j, n = 0; HWND fg = GetForegroundWindow(); unsigned sig = 2166136261u; RunItem out[64]; DWORD now = GetTickCount();
    g_ntmp2 = 0; EnumWindows(RunEnum, 0);
    for (i = 0; i < g_nrun && n < 64; i++)                       /* keep the old order */
        for (j = 0; j < g_ntmp2; j++) if (g_tmpRun[j].h == g_run[i].h) { out[n] = g_tmpRun[j]; out[n].ic = g_run[i].ic; out[n].und = g_run[i].und; out[n].icTick = g_run[i].icTick; n++; g_tmpRun[j].h = NULL; break; }
    for (j = 0; j < g_ntmp2 && n < 64; j++) if (g_tmpRun[j].h) { out[n] = g_tmpRun[j]; out[n].ic = NULL; n++; }
    for (i = 0; i < n; i++) {
        RunItem *r = &out[i];
        if (!r->ic || now - r->icTick > 4000) { HICON ic = MWinIcon(r->h); if (ic != r->ic) { r->ic = ic; r->und = IconAvg(ic, P.acc); } r->icTick = now; }
        r->active = (r->h == fg);
        sig = (sig ^ (unsigned)(ULONG_PTR)r->h) * 16777619u; sig = (sig ^ (unsigned)r->active) * 16777619u;
        { const wchar_t *p; for (p = r->title; *p; p++) sig = (sig ^ (unsigned)*p) * 16777619u; }
    }
    sig ^= (unsigned)n;
    memcpy(g_run, out, sizeof(RunItem) * n); g_nrun = n;
    if (sig == g_runSig) return FALSE;
    g_runSig = sig; return TRUE;
}

/* ---- layout ---- */
static void ItemRect(int i, RECT *r) {
    if (g_tbHoriz) SetRect(r, g_it[i].pos, 0, g_it[i].pos + g_it[i].len, g_tbThick);
    else SetRect(r, 0, g_it[i].pos, g_tbThick, g_it[i].pos + g_it[i].len);
}
static int MinI(int a, int b) { return a < b ? a : b; }
static int MaxI(int a, int b) { return a > b ? a : b; }
static void TbLayout(void) {
    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN), i, n = g_nrun, nPin = g_nL[L_TB], nTray = g_nL[L_TRAY];
    int thick = ScaleI(g.tbHeight), icon = ScaleI(g.tbIcon), pad = ScaleI(4), det = g.tbDetach;
    int horiz = g.tbPos < 2, lenMax = (horiz ? sw : sh) - 2 * det;
    int startLen = (horiz && P.startText[0]) ? thick + ScaleI(40) : thick, pinLen = icon + ScaleI(16);
    int divLen = (nPin > 0 && n > 0) ? ScaleI(10) + g.tbDivider : 0;
    int trayLen = (g.tbTray && nTray) ? nTray * ScaleI(22) + ScaleI(8) : 0;
    int clockLen = g.tbClock ? (horiz ? ScaleI(g.tbDay ? 100 : 80) : ScaleI(46)) : 0, deskLen = g.tbDesk ? ScaleI(14) : 0;
    int sysLen = trayLen + clockLen + deskLen, fixed = startLen + nPin * pinLen + divLen, want, w = 0, clusterLen, lenBar, pc, ps;
    int order[3], k, cnt = 0;
    g_tbHoriz = horiz; g_tbThick = thick;
    want = (g.tbTitles && horiz) ? ScaleI(150) : pinLen;
    if (n > 0) { w = (lenMax - 2 * pad - sysLen - fixed - pad) / n; w = MinI(w, want); w = MaxI(w, icon + ScaleI(10)); }
    clusterLen = fixed + n * w;
    lenBar = g.tbAutoW ? MinI(lenMax, clusterLen + sysLen + 3 * pad) : lenMax;
    if (g.tbAlign == 0) { pc = pad; ps = lenBar - pad - sysLen; }
    else if (g.tbAlign == 1) { pc = (lenBar - clusterLen) / 2; if (pc + clusterLen > lenBar - sysLen - pad) pc = lenBar - sysLen - pad - clusterLen; if (pc < pad) pc = pad; ps = lenBar - pad - sysLen; }
    else { pc = lenBar - pad - clusterLen; ps = pad; }
    g_nit = 0;
    {   int total = 1 + nPin + (divLen ? 1 : 0) + n, x = pc, rev = (g.tbAlign == 2);
        for (k = 0; k < total; k++) {
            int lk = rev ? total - 1 - k : k, type, idx = 0, len;
            if (lk == 0) { type = IT_START; len = startLen; }
            else if (lk <= nPin) { type = IT_PIN; idx = lk - 1; len = pinLen; }
            else if (divLen && lk == nPin + 1) { type = IT_DIV; len = divLen; }
            else { type = IT_RUN; idx = lk - nPin - 1 - (divLen ? 1 : 0); len = w; }
            g_it[g_nit].type = type; g_it[g_nit].idx = idx; g_it[g_nit].pos = x; g_it[g_nit].len = len; g_nit++; x += len;
        }
    }
    if (trayLen) order[cnt++] = IT_TRAY;
    if (clockLen) order[cnt++] = IT_CLOCK;
    if (deskLen) order[cnt++] = IT_DESK;
    {   int x = ps;
        for (k = 0; k < cnt; k++) {
            int t = order[g.tbAlign == 2 ? cnt - 1 - k : k], len = (t == IT_TRAY) ? trayLen : (t == IT_CLOCK ? clockLen : deskLen);
            g_it[g_nit].type = t; g_it[g_nit].idx = 0; g_it[g_nit].pos = x; g_it[g_nit].len = len; g_nit++; x += len;
        }
    }
    if (horiz) {
        int x = g.tbAutoW ? (g.tbAlign == 0 ? det : (g.tbAlign == 1 ? (sw - lenBar) / 2 : sw - lenBar - det)) : det;
        SetRect(&g_tbRect, x, g.tbPos == 0 ? sh - thick - det : det, x + lenBar, (g.tbPos == 0 ? sh - thick - det : det) + thick);
    } else {
        int y = g.tbAutoW ? (g.tbAlign == 0 ? det : (g.tbAlign == 1 ? (sh - lenBar) / 2 : sh - lenBar - det)) : det;
        SetRect(&g_tbRect, g.tbPos == 2 ? det : sw - thick - det, y, (g.tbPos == 2 ? det : sw - thick - det) + thick, y + lenBar);
    }
}
static int TbHit(int x, int y, int *sub) {
    int i, v = g_tbHoriz ? x : y; *sub = 0;
    for (i = 0; i < g_nit; i++) if (v >= g_it[i].pos && v < g_it[i].pos + g_it[i].len) {
        if (g_it[i].type == IT_TRAY) *sub = (v - g_it[i].pos - ScaleI(4)) / ScaleI(22);
        return i;
    }
    return -1;
}

/* ---- painting ---- */
static void Txt(HDC dc, const wchar_t *s, RECT *r, UINT fl, COLORREF c) { SetBkMode(dc, TRANSPARENT); SetTextColor(dc, c); DrawTextW(dc, s, -1, r, fl | DT_NOPREFIX); }
static void BtnBg(HDC dc, const RECT *r, BOOL hot, BOOL active, BOOL down) {
    if (P.style == 2) { if (active || down) PaintSunken(dc, r, P.btn); else PaintRaised(dc, r, P.btn); return; }
    if (hot || active) {
        COLORREF c = active ? P.btnAct : P.btnHot; RECT q = *r; InflateRect(&q, -1, -1);
        if (g.pRound || P.style == 1) RRect(dc, &q, ScaleI(8), c, Blend(c, P.txt, 15)); else FillRectC(dc, q.left, q.top, q.right, q.bottom, c);
    }
}
static void DrawIc(HDC dc, HICON ic, int cx, int cy, int sz) { if (ic) DrawIconEx(dc, cx - sz / 2, cy - sz / 2, ic, sz, sz, 0, NULL, DI_NORMAL); }
static void TbPaint(HDC dc, int w, int h) {
    RECT all, r, q; int i, icon = ScaleI(g.tbIcon); wchar_t buf[80];
    SetRect(&all, 0, 0, w, h);
    PaintSurface(dc, &all, P.barA, P.barB, g.effTb);
    {   COLORREF lc = P.line; int e = g.tbPos;                       /* line on the screen-facing edge */
        if (e == 0) FillRectC(dc, 0, 0, w, 1, lc); else if (e == 1) FillRectC(dc, 0, h - 1, w, h, lc);
        else if (e == 2) FillRectC(dc, w - 1, 0, w, h, lc); else FillRectC(dc, 0, 0, 1, h, lc);
    }
    SelectObject(dc, g_fTb);
    for (i = 0; i < g_nit; i++) {
        Item *it = &g_it[i]; BOOL hot = (i == g_tbHot), dn = (i == g_tbDown && hot);
        ItemRect(i, &r); InflateRect(&r, g_tbHoriz ? 0 : -3, g_tbHoriz ? -3 : 0);
        switch (it->type) {
        case IT_START: {
            COLORREF ic = g.stUseCol ? (COLORREF)g.stCol : (P.style == 2 ? K_ : P.txt);
            BtnBg(dc, &r, hot, g_stOpen, dn);
            q = r; if (P.startText[0] && g_tbHoriz) q.right = q.left + (r.bottom - r.top);
            DrawStartIcon(dc, &q, g.stIcon, ic, P.barA);
            if (P.startText[0] && g_tbHoriz) { RECT t2 = r; t2.left = q.right - 2; SelectObject(dc, g_fTbB); Txt(dc, P.startText, &t2, DT_SINGLELINE | DT_VCENTER, P.style == 2 ? K_ : P.txt); SelectObject(dc, g_fTb); }
            break; }
        case IT_PIN: BtnBg(dc, &r, hot, FALSE, dn); DrawIc(dc, PathIcon(g_L[L_TB][it->idx].p), (r.left + r.right) / 2, (r.top + r.bottom) / 2, icon); break;
        case IT_RUN: {
            RunItem *ru = &g_run[it->idx]; int ix, wide = (g_tbHoriz && g.tbTitles && it->len > icon + ScaleI(40));
            BtnBg(dc, &r, hot, ru->active, dn);
            ix = wide ? r.left + ScaleI(8) + icon / 2 : (r.left + r.right) / 2;
            DrawIc(dc, ru->ic, ix, (r.top + r.bottom) / 2, icon);
            if (wide) { q = r; q.left = ix + icon / 2 + ScaleI(6); q.right -= ScaleI(4); Txt(dc, ru->title, &q, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS, P.txt); }
            if (g.tbUnder) {
                COLORREF uc = g.tbUnderAdapt ? ru->und : P.acc; int uh = g.tbUnderH; if (!ru->active) uc = Blend(uc, P.barA, 45);
                if (g_tbHoriz) { if (g.tbPos == 1) FillRectC(dc, r.left + 4, r.top, r.right - 4, r.top + uh, uc); else FillRectC(dc, r.left + 4, r.bottom - uh, r.right - 4, r.bottom, uc); }
                else { if (g.tbPos == 3) FillRectC(dc, r.left, r.top + 4, r.left + uh, r.bottom - 4, uc); else FillRectC(dc, r.right - uh, r.top + 4, r.right, r.bottom - 4, uc); }
            }
            break; }
        case IT_DIV: {
            int th = g.tbDivider; COLORREF c = P.dim;
            if (g_tbHoriz) FillRectC(dc, (r.left + r.right) / 2 - th / 2, r.top + ScaleI(6), (r.left + r.right) / 2 - th / 2 + th, r.bottom - ScaleI(6), c);
            else FillRectC(dc, r.left + ScaleI(6), (r.top + r.bottom) / 2 - th / 2, r.right - ScaleI(6), (r.top + r.bottom) / 2 - th / 2 + th, c);
            break; }
        case IT_TRAY: {
            int k, sz = ScaleI(16), cx0 = (g_tbHoriz ? r.left + ScaleI(4) : (r.left + r.right) / 2 - sz / 2);
            for (k = 0; k < g_nL[L_TRAY]; k++) {
                int x = g_tbHoriz ? cx0 + k * ScaleI(22) + ScaleI(11) : cx0 + sz / 2, y = g_tbHoriz ? (r.top + r.bottom) / 2 : r.top + ScaleI(4) + k * ScaleI(22) + ScaleI(11);
                if (!g_tbHoriz) { /* vertical tray: icons stacked */ }
                if (g_tbHoriz || y < r.bottom) DrawIc(dc, PathIcon(g_L[L_TRAY][k].p), x, y, sz);
            }
            break; }
        case IT_CLOCK: {
            SYSTEMTIME st; wchar_t tm[24], dt[24], dy[12]; GetLocalTime(&st);
            GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, tm, 24);
            GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, dt, 24);
            GetDateFormatW(LOCALE_USER_DEFAULT, 0, &st, L"ddd", dy, 12);
            BtnBg(dc, &r, hot, FALSE, FALSE);
            if (g_tbHoriz) { _snwprintf(buf, 79, g.tbDay ? L"%ls\n%ls %ls" : L"%ls\n%ls", tm, dy, dt); }
            else { _snwprintf(buf, 79, L"%ls", tm); }
            buf[79] = 0; Txt(dc, buf, &r, DT_CENTER | DT_VCENTER | (g_tbHoriz ? 0 : DT_SINGLELINE), P.txt);
            break; }
        case IT_DESK: {
            BtnBg(dc, &r, hot, FALSE, dn);
            if (g_tbHoriz) FillRectC(dc, r.left, r.top + 2, r.left + 1, r.bottom - 2, P.dim); else FillRectC(dc, r.left + 2, r.top, r.right - 2, r.top + 1, P.dim);
            break; }
        }
    }
}

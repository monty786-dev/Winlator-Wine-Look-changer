/* mst1.h - start panel: state, layout, painting */
static int g_stDragTile = -1;
static int g_stFilter, g_stScroll, g_stNvis, g_stVis[MAXAPPS], g_stDirty = 1, g_stMenu, g_stW, g_stH;
static int g_stHotK, g_stHotI = -1;            /* hot kinds: 1 vbar button, 2 app row, 3 tile, 4 add, 5 filter, 6 user */
static wchar_t g_stSearch[64]; static HWND g_stEdit;
static RECT rVbar, rHead, rEdit, rAdd, rFilt, rApps, rTiles, rVb[7];
static HBITMAP g_avBmp, g_bgBmp; static int g_avW, g_avH, g_bgW, g_bgH; static wchar_t g_avPath[MAX_PATH], g_bgPath[MAX_PATH];
static int g_tcx[MAXL], g_tcy[MAXL], g_ttx[MAXL], g_tty[MAXL], g_cell, g_stAnimOn;
#define TGAP 4
static const wchar_t *FILT_NAMES[6] = { L"All apps", L"Recent", L"Most used", L"Favourite", L"System", L"Control panel" };

static void StSize(int *w, int *h) {
    RECT wa; GetWork(&wa);
    switch (g.stSize) { case 0: *w = ScaleI(380); *h = ScaleI(520); break; case 1: *w = ScaleI(520); *h = ScaleI(600); break;
                        case 2: *w = ScaleI(720); *h = ScaleI(640); break; default: *w = wa.right - wa.left; *h = wa.bottom - wa.top; }
    if (*w > wa.right - wa.left) *w = wa.right - wa.left; if (*h > wa.bottom - wa.top) *h = wa.bottom - wa.top;
}
static void StLayout(int w, int h) {
    int vb = ScaleI(52), x0 = vb, pad = 8, btn = ScaleI(44), i, y, ed = ScaleI(34), top, bl, br, bt, bb, bw;
    SetRect(&rVbar, 0, 0, vb, h);
    for (i = 0; i < 4; i++) SetRect(&rVb[i], 4, 6 + i * (btn + 2), vb - 4, 6 + i * (btn + 2) + btn);
    for (i = 0; i < 3; i++) SetRect(&rVb[6 - i], 4, h - 6 - btn - i * (btn + 2), vb - 4, h - 6 - i * (btn + 2));
    SetRect(&rHead, x0 + pad, pad, w - pad, pad + ScaleI(44));
    y = pad + ScaleI(44) + 6;
    SetRect(&rAdd, w - pad - ed, y, w - pad, y + ed);
    SetRect(&rFilt, rAdd.left - 4 - ScaleI(100), y, rAdd.left - 4, y + ed);
    SetRect(&rEdit, x0 + pad, y, rFilt.left - 4, y + ed);
    top = y + ed + 6; bl = x0 + pad; br = w - pad; bt = top; bb = h - pad; bw = br - bl;
    SetRectEmpty(&rApps); SetRectEmpty(&rTiles);
    if (g.showApps && g.showTiles) {
        if (bw >= ScaleI(470)) { int aw = bw * 45 / 100; SetRect(&rApps, bl, bt, bl + aw, bb); SetRect(&rTiles, bl + aw + 8, bt, br, bb); }
        else { int ah = (bb - bt) * 55 / 100; SetRect(&rApps, bl, bt, br, bt + ah); SetRect(&rTiles, bl, bt + ah + 6, br, bb); }
    } else if (g.showApps) SetRect(&rApps, bl, bt, br, bb); else if (g.showTiles) SetRect(&rTiles, bl, bt, br, bb);
}
static int RowH(void) { return ScaleI(38); }

/* filter + search -> g_stVis */
static int g_cmpMode;
static int CmpVis(const void *a, const void *b) {
    const AppEnt *x = &g_apps[*(const int *)a], *y = &g_apps[*(const int *)b];
    if (g_cmpMode == 1) return (int)(y->last - x->last);
    if (g_cmpMode == 2) return y->cnt - x->cnt;
    return _wcsicmp(x->name, y->name);
}
static void StBuildVisible(void) {
    int i, k; wchar_t lo[64], nm[64]; g_stNvis = 0;
    for (i = 0; i < 64 && g_stSearch[i]; i++) lo[i] = (wchar_t)towlower(g_stSearch[i]); lo[i < 63 ? i : 63] = 0;
    if (g_stFilter == 3) {                                   /* favourites keep their own order */
        for (k = 0; k < g_nL[L_FAV]; k++) for (i = 0; i < g_napps; i++) if (!_wcsicmp(g_apps[i].path, g_L[L_FAV][k].p)) { g_stVis[g_stNvis++] = i; break; }
    } else {
        for (i = 0; i < g_napps; i++) {
            const AppEnt *a = &g_apps[i];
            if (ListFind(L_HIDE, a->path) >= 0) continue;
            if (g_stFilter == 1 && a->last == 0) continue;
            if (g_stFilter == 2 && a->cnt == 0) continue;
            if (g_stFilter == 4 && !(a->sys && !a->ctl)) continue;
            if (g_stFilter == 5 && !a->ctl) continue;
            g_stVis[g_stNvis++] = i;
        }
        g_cmpMode = g_stFilter; qsort(g_stVis, g_stNvis, sizeof(int), CmpVis);
    }
    if (lo[0]) {
        int n = 0;
        for (i = 0; i < g_stNvis; i++) { int j; const wchar_t *s = g_apps[g_stVis[i]].name; for (j = 0; s[j] && j < 63; j++) nm[j] = (wchar_t)towlower(s[j]); nm[j] = 0; if (wcsstr(nm, lo)) g_stVis[n++] = g_stVis[i]; }
        g_stNvis = n;
    }
    if (g_stScroll > 0 && g_stScroll > g_stNvis * RowH()) g_stScroll = 0;
}
static void StReload(void) { ScanApps(); g_stDirty = 0; StBuildVisible(); }
static void StLoadImages(void) {
    if (wcscmp(g_avPath, S_pic) != 0) { wcscpy(g_avPath, S_pic); if (g_avBmp) { DeleteObject(g_avBmp); g_avBmp = NULL; } if (S_pic[0]) g_avBmp = LoadPng(S_pic, &g_avW, &g_avH); }
    if (wcscmp(g_bgPath, S_stbg) != 0) { wcscpy(g_bgPath, S_stbg); if (g_bgBmp) { DeleteObject(g_bgBmp); g_bgBmp = NULL; } if (S_stbg[0]) g_bgBmp = LoadPng(S_stbg, &g_bgW, &g_bgH); }
}
/* draw a bitmap into rect: mode 0 crop(fill) 1 fit 2 stretch 3 center 4 tile */
static void DrawBmp(HDC dc, HBITMAP bm, int bw, int bh, const RECT *r, int mode) {
    HDC m = CreateCompatibleDC(dc); HBITMAP o; int dw = r->right - r->left, dh = r->bottom - r->top, sx = 0, sy = 0, sw = bw, sh = bh, x = r->left, y = r->top, w2 = dw, h2 = dh;
    if (!m || !bm || bw < 1 || bh < 1) { if (m) DeleteDC(m); return; }
    o = (HBITMAP)SelectObject(m, bm); SetStretchBltMode(dc, HALFTONE); SetBrushOrgEx(dc, 0, 0, NULL);
    if (mode == 0) { if (bw * dh > bh * dw) { sw = bh * dw / dh; sx = (bw - sw) / 2; } else { sh = bw * dh / dw; sy = (bh - sh) / 2; } }
    else if (mode == 1) { if (bw * dh > bh * dw) { h2 = dw * bh / bw; y += (dh - h2) / 2; } else { w2 = dh * bw / bh; x += (dw - w2) / 2; } }
    else if (mode == 3) { w2 = bw; h2 = bh; x += (dw - bw) / 2; y += (dh - bh) / 2; }
    if (mode == 4) { int tx, ty; for (ty = r->top; ty < r->bottom; ty += bh) for (tx = r->left; tx < r->right; tx += bw) BitBlt(dc, tx, ty, bw, bh, m, 0, 0, SRCCOPY); }
    else StretchBlt(dc, x, y, w2, h2, m, sx, sy, sw, sh, SRCCOPY);
    SelectObject(m, o); DeleteDC(m);
}
static COLORREF TileColor(const LItem *t) {
    const wchar_t *p; unsigned hs = 0; COLORREF c;
    if (!g.stTileCol) return P.tile;
    if (t->col) c = t->col; else { for (p = t->p; *p; p++) hs = hs * 31 + *p; c = Blend(RGB(60 + hs % 140, 60 + (hs >> 8) % 140, 60 + (hs >> 16) % 140), P.acc, 25); }
    return Blend(c, P.panel, 100 - g.stTileAlpha);
}
static void StTilesLayout(void) {
    static BYTE occ[24][64]; int cols, i, k, rr, cc, w, h, ok; int cw = rTiles.right - rTiles.left;
    g_cell = ScaleI(g.stTilePx) / 2; if (g_cell < 20) g_cell = 20;
    cols = (cw + TGAP) / (g_cell + TGAP); if (cols < 1) cols = 1; if (cols > 24) cols = 24;
    memset(occ, 0, sizeof occ);
    for (i = 0; i < g_nL[L_TILE]; i++) {
        int px = 0, py = 0, found = 0; w = g_L[L_TILE][i].w; h = g_L[L_TILE][i].h; if (w > cols) w = cols;
        for (rr = 0; rr < 60 && !found; rr++) for (cc = 0; cc + w <= cols && !found; cc++) {
            ok = 1; for (k = 0; k < w * h && ok; k++) if (rr + k / w >= 64 || occ[cc + k % w][rr + k / w]) ok = 0;
            if (ok) { for (k = 0; k < w * h; k++) occ[cc + k % w][rr + k / w] = 1; px = cc; py = rr; found = 1; }
        }
        g_ttx[i] = rTiles.left + px * (g_cell + TGAP); g_tty[i] = rTiles.top + py * (g_cell + TGAP) + ScaleI(24);
        if (i == g_stDragTile) continue;
        if (!g.stTileAnim || g_tcx[i] == 0) { g_tcx[i] = g_ttx[i]; g_tcy[i] = g_tty[i]; }
    }
}
static void TileRect(int i, RECT *r) {
    int w = g_L[L_TILE][i].w, h = g_L[L_TILE][i].h;
    SetRect(r, g_tcx[i], g_tcy[i], g_tcx[i] + w * g_cell + (w - 1) * TGAP, g_tcy[i] + h * g_cell + (h - 1) * TGAP);
}

static void StPaint(HDC dc, int w, int h) {
    RECT all, r, q; int i, vis0; int rh = RowH(); COLORREF pt = P.panelTxt, pd = P.panelDim; wchar_t buf[80];
    static const int vk[7] = { UI_LIST, UI_TILES, UI_PROGRAMS, UI_SYSTEM, UI_RUN, UI_SETTINGS, UI_POWER };
    SetRect(&all, 0, 0, w, h); StLoadImages(); StLayout(w, h);
    if (g.stBgMode == 2 && g_bgBmp) DrawBmp(dc, g_bgBmp, g_bgW, g_bgH, &all, 0);
    else if (g.stBgMode == 1) FillRectC(dc, 0, 0, w, h, (COLORREF)g.stBgCol);
    else PaintSurface(dc, &all, P.panel, Blend(P.panel, K_, 8), g.effSt);
    if (g.stBgMode != 0) pt = Lum(g.stBgMode == 1 ? (COLORREF)g.stBgCol : P.panel) < 140 ? W_ : K_;
    if (g.stBarMode == 0) PaintSurface(dc, &rVbar, P.barA, P.barB, g.effSt);
    else if (g.stBarMode == 1) FillRectC(dc, 0, 0, rVbar.right, h, (COLORREF)g.stBarCol);
    else FillRectC(dc, 0, 0, rVbar.right, h, Blend(P.panel, K_, 25));
    SelectObject(dc, g_fTb);
    for (i = 0; i < 7; i++) {
        COLORREF c = g.stBarMode == 1 ? (Lum((COLORREF)g.stBarCol) < 140 ? W_ : K_) : P.txt; BOOL on = (i == 0 && g.showApps) || (i == 1 && g.showTiles);
        if (g_stHotK == 1 && g_stHotI == i) { RRect(dc, &rVb[i], ScaleI(8), P.btnHot, P.btnHot); }
        else if (on) { RRect(dc, &rVb[i], ScaleI(8), P.btn, P.btn); }
        DrawUiIcon(dc, vk[i], &rVb[i], c);
    }
    /* user */
    {   int d = rHead.bottom - rHead.top; RECT av; SetRect(&av, rHead.left, rHead.top, rHead.left + d, rHead.bottom);
        if (g_avBmp) { HRGN rg = CreateEllipticRgn(av.left, av.top, av.right, av.bottom); SelectClipRgn(dc, rg); DrawBmp(dc, g_avBmp, g_avW, g_avH, &av, 0); SelectClipRgn(dc, NULL); if (rg) DeleteObject(rg); }
        else { Disc(dc, (av.left + av.right) / 2, (av.top + av.bottom) / 2, d / 2 - 1, P.acc, P.acc); q = av; buf[0] = S_user[0] ? S_user[0] : L'U'; buf[1] = 0; SelectObject(dc, g_fTbB); Txt(dc, buf, &q, DT_SINGLELINE | DT_VCENTER | DT_CENTER, P.accTxt); SelectObject(dc, g_fTb); }
        q = rHead; q.left = av.right + 10; SelectObject(dc, g_fTbB); Txt(dc, S_user, &q, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS, pt); SelectObject(dc, g_fTb);
    }
    RRect(dc, &rAdd, ScaleI(8), P.acc, P.acc); DrawUiIcon(dc, UI_ADD, &rAdd, P.accTxt);
    RRect(dc, &rFilt, ScaleI(8), Blend(P.panel, pt, 12), Blend(P.panel, pt, 25)); q = rFilt; InflateRect(&q, -6, 0); Txt(dc, FILT_NAMES[g_stFilter], &q, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_END_ELLIPSIS, pt);
    /* apps */
    if (rApps.right > rApps.left) {
        HRGN rg = CreateRectRgn(rApps.left, rApps.top, rApps.right, rApps.bottom); SelectClipRgn(dc, rg);
        vis0 = g_stScroll / rh;
        for (i = vis0; i < g_stNvis; i++) {
            int y = rApps.top + i * rh - g_stScroll; const AppEnt *a = &g_apps[g_stVis[i]];
            if (y > rApps.bottom) break;
            SetRect(&r, rApps.left, y, rApps.right, y + rh);
            if (g_stHotK == 2 && g_stHotI == i) { RRect(dc, &r, ScaleI(6), Blend(P.panel, pt, 14), Blend(P.panel, pt, 14)); }
            DrawIc(dc, PathIcon(a->path), r.left + ScaleI(18), (r.top + r.bottom) / 2, ScaleI(26));
            q = r; q.left += ScaleI(38); Txt(dc, a->name, &q, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS, pt);
        }
        if (g_stNvis == 0) { q = rApps; q.top += 10; Txt(dc, L"No apps here. Tap + to add one.", &q, DT_CENTER | DT_WORDBREAK, pd); }
        SelectClipRgn(dc, NULL); if (rg) DeleteObject(rg);
    }
    /* tiles */
    if (rTiles.right > rTiles.left) {
        q = rTiles; q.bottom = q.top + ScaleI(22); SelectObject(dc, g_fTbB); Txt(dc, L"Pinned", &q, DT_SINGLELINE | DT_VCENTER, pt); SelectObject(dc, g_fTb);
        StTilesLayout();
        for (i = 0; i < g_nL[L_TILE]; i++) {
            int sz; TileRect(i, &r);
            RRect(dc, &r, ScaleI(g.pRound ? 14 : 4), TileColor(&g_L[L_TILE][i]), TileColor(&g_L[L_TILE][i]));
            if (g_stHotK == 3 && g_stHotI == i) { HBRUSH b = CreateSolidBrush(P.acc); FrameRect(dc, &r, b); DeleteObject(b); FillRectC(dc, r.right - 12, r.bottom - 12, r.right - 3, r.bottom - 3, Blend(P.acc, W_, 30)); }
            sz = MinI(r.right - r.left, r.bottom - r.top) / 2; if (sz > ScaleI(48)) sz = ScaleI(48);
            DrawIc(dc, PathIcon(g_L[L_TILE][i].p), (r.left + r.right) / 2, (r.top + r.bottom) / 2 - ScaleI(6), sz);
            if (r.right - r.left >= g_cell * 2) { BaseName(g_L[L_TILE][i].p, buf, 80); q = r; q.top = q.bottom - ScaleI(20); q.left += 4; q.right -= 4; Txt(dc, buf, &q, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_END_ELLIPSIS, Lum(TileColor(&g_L[L_TILE][i])) < 140 ? W_ : K_); }
        }
    }
    if (g.pShadow) EdgeShadow(dc, &all);
}

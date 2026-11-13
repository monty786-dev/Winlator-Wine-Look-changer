/* mpal.h - palette from theme + mode, surface painting, start-button and UI icon presets */
typedef struct {
    COLORREF barA, barB, line, btn, btnHot, btnAct, txt, dim, acc, accTxt, panel, panelTxt, panelDim, tile;
    int style;               /* 0 flat, 1 glass, 2 classic bevel */
    int dark;
    const wchar_t *startText;
} Pal;
static Pal P;

typedef struct { const wchar_t *key; COLORREF a, b, acc; int style; const wchar_t *st; } OsLook;
static const OsLook LOOKS[] = {
    { L"Windows 98",  RGB(192,192,192), RGB(192,192,192), RGB(0,0,128),     2, L"Start" },
    { L"Windows XP",  RGB(36,94,220),   RGB(20,60,180),   RGB(60,150,60),   1, L"start" },
    { L"Windows 7",   RGB(76,110,160),  RGB(26,42,76),    RGB(90,170,235),  1, L"" },
    { L"Windows 8",   RGB(22,34,50),    RGB(22,34,50),    RGB(70,134,198),  0, L"" },
    { L"Windows 10 Dark", RGB(18,18,18), RGB(18,18,18),   RGB(0,120,215),   0, L"" },
    { L"Windows 10",  RGB(22,22,26),    RGB(22,22,26),    RGB(0,120,215),   0, L"" },
    { L"Windows 11",  RGB(34,34,38),    RGB(34,34,38),    RGB(0,103,192),   0, L"" },
    { L"Ubuntu",      RGB(46,43,42),    RGB(34,32,31),    RGB(233,84,32),   0, L"" },
    { L"Arch",        RGB(21,23,28),    RGB(15,16,20),    RGB(23,147,209),  0, L"" },
    { L"Mint",        RGB(40,40,40),    RGB(28,28,28),    RGB(134,175,81),  1, L"" },
    { L"Xfce",        RGB(66,70,74),    RGB(40,44,48),    RGB(82,148,226),  1, L"" },
    { L"Cinnamon",    RGB(56,56,56),    RGB(36,36,36),    RGB(222,121,32),  1, L"" },
    { L"KDE",         RGB(46,50,54),    RGB(30,33,36),    RGB(61,174,233),  1, L"" },
    { L"Adwaita",     RGB(36,31,49),    RGB(36,31,49),    RGB(53,132,228),  0, L"" },
    { L"macOS",       RGB(70,70,76),    RGB(54,54,60),    RGB(0,122,255),   0, L"" },
};
#define NLOOKS ((int)(sizeof LOOKS / sizeof LOOKS[0]))

static void BuildPal(void) {
    const OsLook *lk = NULL; int i; const Theme *t = CurTheme();
    for (i = 0; i < NLOOKS; i++) if (wcsstr(t->name, LOOKS[i].key)) { lk = &LOOKS[i]; break; }
    P.barA = lk ? lk->a : t->tTop; P.barB = lk ? lk->b : t->tBot;
    P.acc = g.pAccOn ? (COLORREF)g.pAcc : (lk ? lk->acc : t->sHilite);
    P.style = (g.tbClassic && lk) ? lk->style : 0;
    P.startText = (g.tbClassic && lk) ? lk->st : L"";
    P.panel = (lk && Lum(lk->a) < 140) ? Blend(lk->a, W_, 8) : t->sFace;
    switch (g.pMode) {
    case 1: P.barA = RGB(28,28,32);  P.barB = RGB(20,20,24); P.panel = RGB(36,36,42); break;
    case 2: P.barA = RGB(236,236,240); P.barB = RGB(220,220,226); P.panel = RGB(248,248,250); break;
    case 3: P.barA = K_; P.barB = K_; P.panel = K_; break;
    case 4: P.acc = RGB(236,96,140); P.barA = RGB(58,30,42); P.barB = RGB(40,20,30); P.panel = RGB(68,38,52); break;
    case 5: P.acc = RGB(60,120,235); P.barA = RGB(16,30,66); P.barB = RGB(10,20,46); P.panel = RGB(24,40,84); break;
    case 6: P.barA = RGB(110,165,222); P.barB = RGB(60,110,170); P.panel = RGB(204,224,246); P.style = 1; break;
    }
    if (g.tbUseCol) { P.barA = (COLORREF)g.tbCol; P.barB = Blend((COLORREF)g.tbCol, K_, 22); }
    P.dark = Lum(P.barA) < 140;
    P.txt = P.dark ? W_ : K_;
    P.dim = Blend(P.txt, P.barA, 45);
    P.line = Blend(P.barA, P.txt, 22);
    P.btn = Blend(P.barA, P.txt, 8); P.btnHot = Blend(P.barA, P.txt, 20); P.btnAct = Blend(P.barA, P.txt, 32);
    P.accTxt = Lum(P.acc) < 150 ? W_ : K_;
    P.panelTxt = Lum(P.panel) < 140 ? W_ : K_;
    P.panelDim = Blend(P.panelTxt, P.panel, 45);
    P.tile = Blend(P.panel, P.acc, 55);
}

/* surface: gradient + optional glass shine/gloss, frosted lift, 3D bevel */
static void PaintSurface(HDC dc, const RECT *r, COLORREF a, COLORREF b, int effOn) {
    int h = r->bottom - r->top, y, eff = effOn ? g.pEffect : 0, glass = (eff == 1 || eff == 2 || P.style == 1);
    if (h < 1) return;
    for (y = 0; y < h; y++) {
        COLORREF c = Blend(a, b, h > 1 ? y * 100 / (h - 1) : 0);
        if (eff == 2) c = Blend(c, W_, g.pBlur / 8);
        if (glass) {
            int half = h / 2 > 0 ? h / 2 : 1;
            if (y < half) c = Blend(c, W_, g.pShine * (half - y) / half / 2);
            else c = Blend(c, K_, g.pGloss * (y - half) / half / 4);
            if (y == half) c = Blend(c, W_, g.pGloss / 3);
        }
        FillRectC(dc, r->left, r->top + y, r->right, r->top + y + 1, c);
    }
    if (eff == 3 || P.style == 2) {
        FillRectC(dc, r->left, r->top, r->right, r->top + 1, W_); FillRectC(dc, r->left, r->top, r->left + 1, r->bottom, W_);
        FillRectC(dc, r->left, r->bottom - 1, r->right, r->bottom, RGB(64,64,64)); FillRectC(dc, r->right - 1, r->top, r->right, r->bottom, RGB(64,64,64));
    }
}
static void PaintSunken(HDC dc, const RECT *r, COLORREF face) {
    FillRectC(dc, r->left, r->top, r->right, r->bottom, face);
    FillRectC(dc, r->left, r->top, r->right, r->top + 1, RGB(64,64,64)); FillRectC(dc, r->left, r->top, r->left + 1, r->bottom, RGB(64,64,64));
    FillRectC(dc, r->left, r->bottom - 1, r->right, r->bottom, W_); FillRectC(dc, r->right - 1, r->top, r->right, r->bottom, W_);
}
static void PaintRaised(HDC dc, const RECT *r, COLORREF face) {
    FillRectC(dc, r->left, r->top, r->right, r->bottom, face);
    FillRectC(dc, r->left, r->top, r->right - 1, r->top + 1, W_); FillRectC(dc, r->left, r->top, r->left + 1, r->bottom - 1, W_);
    FillRectC(dc, r->left, r->bottom - 1, r->right, r->bottom, RGB(64,64,64)); FillRectC(dc, r->right - 1, r->top, r->right, r->bottom, RGB(64,64,64));
}
static void RRect(HDC dc, const RECT *r, int rad, COLORREF fill, COLORREF line) {
    HPEN p = CreatePen(PS_SOLID, 1, line); HBRUSH b = CreateSolidBrush(fill);
    HPEN op = (HPEN)SelectObject(dc, p); HBRUSH ob = (HBRUSH)SelectObject(dc, b);
    RoundRect(dc, r->left, r->top, r->right, r->bottom, rad, rad);
    SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b); DeleteObject(p);
}
static void Disc(HDC dc, int cx, int cy, int r, COLORREF fill, COLORREF line) {
    HPEN p = CreatePen(PS_SOLID, 1, line); HBRUSH b = CreateSolidBrush(fill);
    HPEN op = (HPEN)SelectObject(dc, p); HBRUSH ob = (HBRUSH)SelectObject(dc, b);
    Ellipse(dc, cx - r, cy - r, cx + r + 1, cy + r + 1);
    SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b); DeleteObject(p);
}
static void RoundRgn(HWND w, int cw, int ch, int rad) {
    HRGN rg = CreateRoundRectRgn(0, 0, cw + 1, ch + 1, rad * 2, rad * 2);
    if (rg && !SetWindowRgn(w, rg, TRUE)) DeleteObject(rg);
}
static void EdgeShadow(HDC dc, const RECT *r) {
    int i; for (i = 0; i < 3; i++) {
        COLORREF c = Blend(P.panel, K_, 30 + i * 14); RECT f; f.left = r->left + i; f.top = r->top + i; f.right = r->right - i; f.bottom = r->bottom - i;
        { HBRUSH b = CreateSolidBrush(c); FrameRect(dc, &f, b); DeleteObject(b); }
    }
}

/* ---- Start button presets (18) ---- */
static void DrawStartIcon(HDC dc, const RECT *rc, int p, COLORREF col, COLORREF bg) {
    int cx = (rc->left + rc->right) / 2, cy = (rc->top + rc->bottom) / 2, s = (rc->bottom - rc->top < rc->right - rc->left ? rc->bottom - rc->top : rc->right - rc->left) / 2 - 3, i, j;
    HPEN pen = CreatePen(PS_SOLID, 2, col), op; HBRUSH br = CreateSolidBrush(col), nb = (HBRUSH)GetStockObject(NULL_BRUSH), ob; POINT pt[10];
    static const int SX[10] = { 0, 24, 95, 38, 59, 0, -59, -38, -95, -24 }, SY[10] = { -100, -32, -31, 12, 81, 40, 81, 12, -31, -32 };
    if (s < 5) s = 5;
    op = (HPEN)SelectObject(dc, pen); ob = (HBRUSH)SelectObject(dc, nb);
    switch (p) {
    case 0: case 3: { int g2 = (p == 0 ? 1 : 3), a = s - g2 / 2 - 1; SelectObject(dc, br);
        Rectangle(dc, cx - s, cy - s, cx - g2, cy - g2); Rectangle(dc, cx + g2, cy - s, cx + s, cy - g2);
        Rectangle(dc, cx - s, cy + g2, cx - g2, cy + s); Rectangle(dc, cx + g2, cy + g2, cx + s, cy + s); (void)a; break; }
    case 1: case 2: { COLORREF cs[4]; cs[0] = RGB(240,80,50); cs[1] = RGB(120,200,60); cs[2] = RGB(60,150,240); cs[3] = RGB(250,200,40);
        if (p == 2) { SelectObject(dc, ob); Disc(dc, cx, cy, s + 2, Blend(bg, RGB(80,150,240), 70), W_); SelectObject(dc, nb); }
        for (i = 0; i < 4; i++) { int x0 = cx + (i % 2 ? 1 : -s + (p == 2 ? 2 : 0)), y0 = cy + (i / 2 ? 1 : -s + (p == 2 ? 2 : 0)), w2 = (p == 2 ? s - 3 : s - 1);
            FillRectC(dc, x0, y0, x0 + w2, y0 + w2, cs[i]); } break; }
    case 4: SelectObject(dc, nb); Ellipse(dc, cx - s, cy - s, cx + s + 1, cy + s + 1);
        SelectObject(dc, br); Ellipse(dc, cx - 2, cy - s - 2, cx + 3, cy - s + 3); Ellipse(dc, cx - s - 2, cy + s / 2 - 2, cx - s + 3, cy + s / 2 + 3); Ellipse(dc, cx + s - 2, cy + s / 2 - 2, cx + s + 3, cy + s / 2 + 3); break;
    case 5: SelectObject(dc, br); pt[0].x = cx; pt[0].y = cy - s; pt[1].x = cx + s; pt[1].y = cy + s; pt[2].x = cx + s / 3; pt[2].y = cy + s - 1; pt[3].x = cx; pt[3].y = cy + s / 3;
        pt[4].x = cx - s / 3; pt[4].y = cy + s - 1; pt[5].x = cx - s; pt[5].y = cy + s; Polygon(dc, pt, 6); break;
    case 6: SelectObject(dc, br); Ellipse(dc, cx - s, cy - s / 2, cx + s + 1, cy + s); SelectObject(dc, nb); MoveToEx(dc, cx, cy - s, NULL); LineTo(dc, cx, cy + s / 2); break;
    case 7: SelectObject(dc, br); Ellipse(dc, cx - s + 2, cy - s / 2, cx + s - 1, cy + s); Ellipse(dc, cx - s, cy - s, cx - s / 3, cy - s / 4); Ellipse(dc, cx + s / 3, cy - s, cx + s + 1, cy - s / 4); break;
    case 8: SelectObject(dc, nb); Ellipse(dc, cx - s, cy - s, cx + s + 1, cy + s + 1); Ellipse(dc, cx - s / 2, cy - s / 2, cx + s / 2 + 1, cy + s / 2 + 1); Arc(dc, cx - s / 4, cy - s / 4, cx + s / 4, cy + s / 4, cx, cy - s / 4, cx - s / 4, cy); break;
    case 9: { HFONT f = CreateFontW(-s * 2, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Arial"), of; RECT r2 = *rc;
        SelectObject(dc, nb); Ellipse(dc, cx - s - 1, cy - s - 1, cx + s + 2, cy + s + 2);
        of = (HFONT)SelectObject(dc, f); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, col); DrawTextW(dc, L"K", 1, &r2, DT_SINGLELINE | DT_VCENTER | DT_CENTER); SelectObject(dc, of); DeleteObject(f); break; }
    case 10: SelectObject(dc, br); for (i = 0; i < 3; i++) for (j = 0; j < 3; j++) Ellipse(dc, cx - s + i * s - 1, cy - s + j * s - 1, cx - s + i * s + 3, cy - s + j * s + 3); break;
    case 11: for (i = -1; i <= 1; i++) { MoveToEx(dc, cx - s, cy + i * (s / 2 + 1), NULL); LineTo(dc, cx + s + 1, cy + i * (s / 2 + 1)); } break;
    case 12: pt[0].x = cx; pt[0].y = cy - s; pt[1].x = cx + s; pt[1].y = cy; pt[2].x = cx; pt[2].y = cy + s; pt[3].x = cx - s; pt[3].y = cy; SelectObject(dc, br); Polygon(dc, pt, 4); break;
    case 13: SelectObject(dc, br); for (i = 0; i < 10; i++) { pt[i].x = cx + SX[i] * (s + 2) / 100; pt[i].y = cy + SY[i] * (s + 2) / 100 + 1; } Polygon(dc, pt, 10); break;
    case 14: SelectObject(dc, br); Ellipse(dc, cx - s, cy - s, cx + 1, cy + 1); Ellipse(dc, cx, cy - s, cx + s + 1, cy + 1);
        pt[0].x = cx - s + 1; pt[0].y = cy - 1; pt[1].x = cx + s; pt[1].y = cy - 1; pt[2].x = cx; pt[2].y = cy + s; Polygon(dc, pt, 3); break;
    case 15: SelectObject(dc, br); Ellipse(dc, cx - s, cy - s, cx + s + 1, cy + s + 1); break;
    case 16: { HFONT f = CreateFontW(-s * 2 + 2, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Arial"), of; RECT r2 = *rc;
        of = (HFONT)SelectObject(dc, f); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, col); DrawTextW(dc, L"S", 1, &r2, DT_SINGLELINE | DT_VCENTER | DT_CENTER); SelectObject(dc, of); DeleteObject(f); break; }
    default: { HFONT f = CreateFontW(-s * 2, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Arial"), of; RECT r2 = *rc;
        of = (HFONT)SelectObject(dc, f); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, col); DrawTextW(dc, L"M", 1, &r2, DT_SINGLELINE | DT_VCENTER | DT_CENTER); SelectObject(dc, of); DeleteObject(f); break; }
    }
    SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(pen); DeleteObject(br);
}

/* ---- Vertical-bar / panel icons: 15 presets = 5 frames x 3 weights ---- */
enum { UI_POWER, UI_SETTINGS, UI_RUN, UI_PROGRAMS, UI_SYSTEM, UI_LIST, UI_TILES, UI_ADD, UI_SEARCH, UI_DESK, UI_USER };
static void DrawUiIcon(HDC dc, int kind, const RECT *rc, COLORREF col) {
    int cx = (rc->left + rc->right) / 2, cy = (rc->top + rc->bottom) / 2, f = g.pIcon % 5, wt = g.pIcon / 5 + 1, q = 6, i;
    int fh = (rc->bottom - rc->top < rc->right - rc->left ? rc->bottom - rc->top : rc->right - rc->left) / 2 - 1;
    COLORREF gc = g.pTint ? col : RGB(200,200,200), ink = gc; HPEN pen, op; HBRUSH br, nb = (HBRUSH)GetStockObject(NULL_BRUSH), ob; POINT p[4];
    if (fh > 14) fh = 14;
    if (f == 1 || f == 3) { HPEN fp = CreatePen(PS_SOLID, 1, gc); HPEN o2 = (HPEN)SelectObject(dc, fp); HBRUSH b2 = (HBRUSH)SelectObject(dc, nb);
        if (f == 1) Ellipse(dc, cx - fh, cy - fh, cx + fh + 1, cy + fh + 1); else RoundRect(dc, cx - fh, cy - fh, cx + fh + 1, cy + fh + 1, 8, 8);
        SelectObject(dc, b2); SelectObject(dc, o2); DeleteObject(fp); }
    else if (f == 2 || f == 4) { if (f == 2) Disc(dc, cx, cy, fh, gc, gc); else { RECT r2; r2.left = cx - fh; r2.top = cy - fh; r2.right = cx + fh + 1; r2.bottom = cy + fh + 1; RRect(dc, &r2, 8, gc, gc); }
        ink = Lum(gc) < 150 ? W_ : K_; }
    if (f) q = fh > 9 ? 5 : 4;
    pen = CreatePen(PS_SOLID, wt, ink); br = CreateSolidBrush(ink);
    op = (HPEN)SelectObject(dc, pen); ob = (HBRUSH)SelectObject(dc, g.pFill ? br : nb);
    switch (kind) {
    case UI_POWER: SelectObject(dc, nb); Arc(dc, cx - q, cy - q + 1, cx + q + 1, cy + q + 2, cx + q / 2, cy - q + 1, cx - q / 2, cy - q + 1);
        MoveToEx(dc, cx, cy - q - 1, NULL); LineTo(dc, cx, cy); break;
    case UI_SETTINGS: SelectObject(dc, nb); Ellipse(dc, cx - q + 1, cy - q + 1, cx + q, cy + q); Ellipse(dc, cx - 2, cy - 2, cx + 3, cy + 3);
        for (i = 0; i < 8; i++) { int ax = (i % 4 == 0) ? 0 : (i < 4 ? 1 : -1), ay = (i % 4 == 2) ? 0 : 0; (void)ay; (void)ax; }
        MoveToEx(dc, cx, cy - q - 2, NULL); LineTo(dc, cx, cy - q + 1); MoveToEx(dc, cx, cy + q + 2, NULL); LineTo(dc, cx, cy + q - 1);
        MoveToEx(dc, cx - q - 2, cy, NULL); LineTo(dc, cx - q + 1, cy); MoveToEx(dc, cx + q + 2, cy, NULL); LineTo(dc, cx + q - 1, cy); break;
    case UI_RUN: p[0].x = cx - q / 2; p[0].y = cy - q; p[1].x = cx + q; p[1].y = cy; p[2].x = cx - q / 2; p[2].y = cy + q; Polygon(dc, p, 3); break;
    case UI_PROGRAMS: for (i = 0; i < 9; i++) { int x0 = cx - q + (i % 3) * (q - 1), y0 = cy - q + (i / 3) * (q - 1); Rectangle(dc, x0, y0, x0 + 3, y0 + 3); } break;
    case UI_SYSTEM: Rectangle(dc, cx - q, cy - q + 1, cx + q + 1, cy + 2); MoveToEx(dc, cx, cy + 2, NULL); LineTo(dc, cx, cy + q); MoveToEx(dc, cx - q / 2, cy + q, NULL); LineTo(dc, cx + q / 2 + 1, cy + q); break;
    case UI_LIST: for (i = -1; i <= 1; i++) { Rectangle(dc, cx - q, cy + i * 5 - 1, cx - q + 3, cy + i * 5 + 2); MoveToEx(dc, cx - q + 5, cy + i * 5, NULL); LineTo(dc, cx + q + 1, cy + i * 5); } break;
    case UI_TILES: Rectangle(dc, cx - q, cy - q, cx - 1, cy - 1); Rectangle(dc, cx + 1, cy - q, cx + q + 1, cy - 1);
        Rectangle(dc, cx - q, cy + 1, cx - 1, cy + q + 1); Rectangle(dc, cx + 1, cy + 1, cx + q + 1, cy + q + 1); break;
    case UI_ADD: MoveToEx(dc, cx - q, cy, NULL); LineTo(dc, cx + q + 1, cy); MoveToEx(dc, cx, cy - q, NULL); LineTo(dc, cx, cy + q + 1); break;
    case UI_SEARCH: SelectObject(dc, nb); Ellipse(dc, cx - q, cy - q, cx + 2, cy + 2); MoveToEx(dc, cx + 1, cy + 1, NULL); LineTo(dc, cx + q + 1, cy + q + 1); break;
    case UI_DESK: Rectangle(dc, cx - q, cy - q + 1, cx + q + 1, cy + q); MoveToEx(dc, cx - q, cy - q + 4, NULL); LineTo(dc, cx + q + 1, cy - q + 4); break;
    default: SelectObject(dc, br); Ellipse(dc, cx - 3, cy - q, cx + 4, cy - q + 7); SelectObject(dc, nb); Arc(dc, cx - q, cy + 1, cx + q + 1, cy + q * 2 - 1, cx + q, cy + q, cx - q, cy + q); break;
    }
    SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(pen); DeleteObject(br);
}

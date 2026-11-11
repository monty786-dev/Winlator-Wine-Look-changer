/* ui1.h - UI palette, owner-drawn controls, Theme Lab property editor */
static const COLORREF PALETTE[8] = { RGB(0,120,215), RGB(232,17,35), RGB(16,137,62), RGB(255,140,0),
                                     RGB(136,23,152), RGB(0,153,188), RGB(60,60,60), RGB(233,30,99) };
static COLORREF UiBg(void)     { return g_dark ? RGB(30,30,30) : RGB(243,243,243); }
static COLORREF UiPanel(void)  { return g_dark ? RGB(48,48,52) : RGB(255,255,255); }
static COLORREF UiFace(void)   { return g_dark ? RGB(62,62,68) : RGB(226,226,230); }
static COLORREF UiText(void)   { return g_dark ? RGB(236,236,236) : RGB(28,28,28); }
static COLORREF UiSub(void)    { return g_dark ? RGB(150,150,155) : RGB(100,100,105); }
static COLORREF UiAccent(void) { return g_hasCol ? g_col : g_eff.sHilite; }

static int *TogPtr(int id) {
    switch (id) { case IDC_O_AUTO: return &g_auto; case IDC_O_SYS: return &g_sysApply; case IDC_O_BG: return &g_bg;
                  case IDC_O_TOP: return &g_mgrTop; case IDC_O_DARK: return &g_dark;
                  case IDC_O_PIN: return &g_showPin; case IDC_O_SHADE: return &g_showShade; case IDC_O_SNAP: return &g_showSnap; }
    return NULL;
}

static void DrawRound(HDC dc, const RECT *rc, int rad, COLORREF fill, COLORREF line) {
    HPEN p = CreatePen(PS_SOLID, 1, line); HBRUSH b = CreateSolidBrush(fill);
    HPEN op = (HPEN)SelectObject(dc, p); HBRUSH ob = (HBRUSH)SelectObject(dc, b);
    RoundRect(dc, rc->left, rc->top, rc->right, rc->bottom, rad, rad);
    SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b); DeleteObject(p);
}

/* Flat, rounded, theme-accent buttons (tabs, actions, switches) */
static void DrawUiButton(const DRAWITEMSTRUCT *d) {
    int id = (int)d->CtlID; RECT rc = d->rcItem, in; HDC dc = d->hDC; wchar_t tx[80];
    BOOL pr = (d->itemState & ODS_SELECTED) != 0; COLORREF face = UiFace(), txt = UiText(), acc = UiAccent();
    int *tg = TogPtr(id); BOOL tab = (id >= IDC_TAB0 && id <= IDC_TAB2);
    BOOL primary = (id == IDC_LSAVE);
    GetWindowTextW(d->hwndItem, tx, 80);
    FillRectC(dc, rc.left, rc.top, rc.right, rc.bottom, UiBg());
    in = rc; InflateRect(&in, -2, -2);
    SelectObject(dc, g_fUi); SetBkMode(dc, TRANSPARENT);
    if (tg) {
        RECT pill, lab = in; int on = *tg;
        DrawRound(dc, &in, 10, UiPanel(), g_dark ? RGB(70,70,76) : RGB(210,210,214));
        lab.left += 10; lab.right -= 62; SelectObject(dc, g_fSmall); SetTextColor(dc, txt);
        { RECT cr = lab; DrawTextW(dc, tx, -1, &cr, DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT);
          lab.top += (lab.bottom - lab.top - (cr.bottom - cr.top)) / 2; DrawTextW(dc, tx, -1, &lab, DT_WORDBREAK | DT_NOPREFIX); }
        pill.right = in.right - 12; pill.left = pill.right - 44; pill.top = (in.top + in.bottom) / 2 - 11; pill.bottom = pill.top + 22;
        DrawRound(dc, &pill, 22, on ? acc : (g_dark ? RGB(90,90,96) : RGB(180,180,186)), on ? acc : (g_dark ? RGB(90,90,96) : RGB(180,180,186)));
        { HBRUSH b = CreateSolidBrush(W_); HPEN p = CreatePen(PS_SOLID, 1, W_); HBRUSH ob = (HBRUSH)SelectObject(dc, b); HPEN op = (HPEN)SelectObject(dc, p);
          int kx = on ? pill.right - 20 : pill.left + 2; Ellipse(dc, kx, pill.top + 2, kx + 18, pill.top + 20);
          SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b); DeleteObject(p); }
        return;
    }
    if (tab && (id - IDC_TAB0) == g_tab) { face = acc; txt = Lum(acc) < 150 ? W_ : K_; }
    else if (primary) { face = acc; txt = Lum(acc) < 150 ? W_ : K_; }
    if (pr) face = Blend(face, g_dark ? W_ : K_, 18);
    DrawRound(dc, &in, 12, face, face);
    SetTextColor(dc, txt); DrawTextW(dc, tx, -1, &in, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}

/* Theme gallery: each row is a live render of that theme's title bar */
static void DrawGalleryItem(const DRAWITEMSTRUCT *d) {
    int idx = (int)d->itemID; RECT rc = d->rcItem; HDC dc = d->hDC; const Theme *t; PState s; HFONT f; HDC m; HBITMAP bm, ob;
    int w = rc.right - rc.left - 16, H; BOOL active;
    FillRectC(dc, rc.left, rc.top, rc.right, rc.bottom, UiBg());
    if (idx < 0 || idx >= g_nthemes) return;
    t = &g_themes[idx]; H = t->height; active = (idx == g_themeIdx);
    m = CreateCompatibleDC(dc); bm = CreateCompatibleBitmap(dc, w > 1 ? w : 1, H);
    if (m && bm) {
        ob = (HBITMAP)SelectObject(m, bm);
        memset(&s, 0, sizeof s); s.title = t->name; s.icon = LoadIconW(NULL, IDI_APPLICATION); s.active = TRUE; s.hot = -1; s.down = -1; s.pinned = (idx % 3 == 0); s.hide = g_hideG;
        f = MakeTitleFont(t); PaintSkin(m, w, H, t, &s, f); DeleteObject(f);
        BitBlt(dc, rc.left + 8, rc.top + 4, w, H, m, 0, 0, SRCCOPY);
        SelectObject(m, ob);
    }
    if (bm) DeleteObject(bm);
    if (m) DeleteDC(m);
    {   RECT fr; fr.left = rc.left + 6; fr.top = rc.top + 2; fr.right = rc.right - 6; fr.bottom = rc.top + 6 + H;
        HBRUSH b = CreateSolidBrush(active ? UiAccent() : UiFace()); int k;
        for (k = 0; k < (active ? 2 : 1); k++) { FrameRect(dc, &fr, b); InflateRect(&fr, -1, -1); } DeleteObject(b); }
    SelectObject(dc, g_fSmall); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, active ? UiAccent() : UiSub());
    { RECT tr; wchar_t tx[96]; tr.left = rc.left + 10; tr.top = rc.top + H + 7; tr.right = rc.right - 10; tr.bottom = rc.bottom;
      _snwprintf(tx, 95, L"%ls%ls%ls", t->custom ? L"Custom theme" : L"Built-in", active ? L"   -   ACTIVE" : L"", t->left ? L"   -   buttons on left" : L"");
      tx[95] = 0; DrawTextW(dc, tx, -1, &tr, DT_SINGLELINE | DT_NOPREFIX); }
}

/* ---- Theme Lab property table ---- */
enum { PT_INT, PT_ENUM, PT_COLOR, PT_FONT, PT_ICON, PT_PRESET };
typedef struct { const wchar_t *name; int type; size_t off; int a, b, c; const wchar_t *const *opts; } Prop;
static const wchar_t *O_SHAPE[] = { L"Flat", L"Rounded square", L"Circle", L"Glass", L"Classic 3D" };
static const wchar_t *O_GST[]   = { L"Thin lines", L"Bold lines", L"Colour dots", L"Boxed", L"Filled shapes" };
static const wchar_t *O_SIDE[]  = { L"Right", L"Left" };
static const wchar_t *O_ALIGN[] = { L"Left", L"Center" };
static const wchar_t *O_DBL[]   = { L"Maximize", L"Roll up" };
static const wchar_t *O_YN[]    = { L"No", L"Yes" };
static const wchar_t *FONTS[]   = { L"Segoe UI", L"Tahoma", L"Arial", L"Verdana", L"Trebuchet MS", L"DejaVu Sans",
                                    L"Ubuntu", L"Cantarell", L"Noto Sans", L"Courier New", L"Times New Roman", L"Helvetica" };
#define NFONTS 12
#define PI_(n,f,lo,hi,st) { n, PT_INT,  offsetof(Theme, f), lo, hi, st, NULL }
#define PE_(n,f,o,cnt)    { n, PT_ENUM, offsetof(Theme, f), cnt, 0, 0, o }
#define PC_(n,f)          { n, PT_COLOR, offsetof(Theme, f), 0, 0, 0, NULL }
#define PIC_(n,slot)      { n, PT_ICON, 0, slot, 0, 0, NULL }
#define PRE_(n,slot)      { n, PT_PRESET, 0, slot, 0, 0, NULL }
static const Prop PROPS[] = {
    PI_(L"Title bar height", height, 22, 44, 2), PI_(L"Corner radius", radius, 0, 14, 2),
    PE_(L"Button shape", shape, O_SHAPE, 5), PE_(L"Icon style", gstyle, O_GST, 5),
    PI_(L"Button width", bw, 16, 56, 2), PI_(L"Button height", bh, 14, 36, 2), PI_(L"Close button width", closeW, 16, 56, 2),
    PI_(L"Button spacing", gap, 0, 10, 1), PE_(L"Buttons side", left, O_SIDE, 2), PE_(L"Title alignment", center, O_ALIGN, 2),
    PE_(L"Double-click title", dbl, O_DBL, 2),
    { L"Title font", PT_FONT, 0, 0, 0, 0, NULL }, PI_(L"Title font size", fsize, 8, 16, 1),
    PE_(L"Bold title", bold, O_YN, 2), PE_(L"Title text shadow", shadow, O_YN, 2),
    PC_(L"Title top (active)", tTop), PC_(L"Title bottom (active)", tBot), PC_(L"Title text (active)", txtA),
    PC_(L"Title top (inactive)", tTopI), PC_(L"Title bottom (inactive)", tBotI), PC_(L"Title text (inactive)", txtI),
    PC_(L"Border (active)", border), PC_(L"Border (inactive)", borderI),
    PC_(L"Button face", bFace), PC_(L"Button hover", bHover), PC_(L"Button pressed", bPress), PC_(L"Button border", bBorder),
    PC_(L"Button icon colour", glyph), PC_(L"Close face", closeFace), PC_(L"Close hover", closeHover),
    PC_(L"Close icon colour", closeGlyph), PC_(L"Pin colour (active)", pin),
    PC_(L"Workspace colour", sWindow), PC_(L"Toolbar/menu/status colour", sFace), PC_(L"Text colour", sText),
    PC_(L"Highlight colour", sHilite), PC_(L"Highlight text", sHiliteText), PC_(L"3D shadow", sShadow), PC_(L"3D light", sLight),
    PC_(L"Scroll bar track", sScroll), PC_(L"App background", sWork),
    PI_(L"Scroll bar width", scrollW, 10, 24, 1), PI_(L"Menu bar height", menuH, 16, 32, 2), PI_(L"Border width", borderW, 1, 4, 1),
    PIC_(L"Icon: Close", BT_CLOSE), PIC_(L"Icon: Maximize", BT_MAX), PIC_(L"Icon: Restore", ICN_RESTORE), PIC_(L"Icon: Minimize", BT_MIN),
    PIC_(L"Icon: Snap left", BT_SNAPL), PIC_(L"Icon: Snap right", BT_SNAPR), PIC_(L"Icon: Pin", BT_PIN), PIC_(L"Icon: Roll up", BT_SHADE),
    PRE_(L"Preset: Close (25)", BT_CLOSE), PRE_(L"Preset: Maximize (25)", BT_MAX), PRE_(L"Preset: Restore (25)", ICN_RESTORE),
    PRE_(L"Preset: Minimize (25)", BT_MIN), PRE_(L"Preset: Roll up (25)", BT_SHADE), PRE_(L"Preset: Pin (25)", BT_PIN),
    PRE_(L"Preset: Snap left (25)", BT_SNAPL), PRE_(L"Preset: Snap right (25)", BT_SNAPR),
};
#define NPROPS ((int)(sizeof PROPS / sizeof PROPS[0]))

static void PropText(int i, wchar_t *b, int n, COLORREF *sw) {
    const Prop *p = &PROPS[i]; const BYTE *base = (const BYTE *)&g_lab; b[0] = 0; *sw = (COLORREF)-1;
    switch (p->type) {
    case PT_INT:  _snwprintf(b, n - 1, L"%d", *(const int *)(base + p->off)); break;
    case PT_ENUM: { int v = *(const int *)(base + p->off); if (v < 0 || v >= p->a) v = 0; wcsncpy(b, p->opts[v], n - 1); break; }
    case PT_COLOR: { COLORREF c = *(const COLORREF *)(base + p->off); *sw = c;
                     _snwprintf(b, n - 1, L"#%02X%02X%02X", GetRValue(c), GetGValue(c), GetBValue(c)); break; }
    case PT_FONT: wcsncpy(b, g_lab.font, n - 1); break;
    case PT_PRESET: if (g_lab.preset[p->a]) _snwprintf(b, n - 1, L"Preset %d", g_lab.preset[p->a]); else wcscpy(b, L"Default"); break;
    case PT_ICON: { const wchar_t *f = g_lab.icon[p->a]; const wchar_t *sl = wcsrchr(f, L'\\');
                    wcsncpy(b, f[0] ? (sl ? sl + 1 : f) : L"built-in", n - 1); break; }
    }
    b[n - 1] = 0;
}
static void DrawPropItem(const DRAWITEMSTRUCT *d) {
    int i = (int)d->itemID; RECT rc = d->rcItem, tr; HDC dc = d->hDC; wchar_t v[MAX_PATH]; COLORREF sw;
    FillRectC(dc, rc.left, rc.top, rc.right, rc.bottom, (i & 1) ? UiPanel() : Blend(UiPanel(), UiBg(), 50));
    if (i < 0 || i >= NPROPS) return;
    PropText(i, v, 60, &sw);
    SetBkMode(dc, TRANSPARENT); SelectObject(dc, g_fUi);
    SetTextColor(dc, UiText()); tr = rc; tr.left += 10; tr.right = rc.right - 150;
    DrawTextW(dc, PROPS[i].name, -1, &tr, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
    tr = rc; tr.right -= 10; tr.left = rc.right - 145;
    if (sw != (COLORREF)-1) {
        RECT sq; sq.right = rc.right - 10; sq.left = sq.right - 30; sq.top = (rc.top + rc.bottom) / 2 - 11; sq.bottom = sq.top + 22;
        DrawRound(dc, &sq, 6, sw, UiSub()); tr.right = sq.left - 8;
    }
    SetTextColor(dc, UiSub()); DrawTextW(dc, v, -1, &tr, DT_SINGLELINE | DT_VCENTER | DT_RIGHT | DT_NOPREFIX | DT_END_ELLIPSIS);
}

static int PopupChoose(POINT pt, const wchar_t *const *items, int n, int checked) {
    HMENU mn = CreatePopupMenu(); int i, cmd;
    if (!mn) return -1;
    for (i = 0; i < n; i++) AppendMenuW(mn, MF_STRING | (i == checked ? MF_CHECKED : 0), (UINT_PTR)(i + 1), items[i]);
    cmd = TrackPopupMenu(mn, TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, g_main, NULL);
    DestroyMenu(mn);
    return cmd > 0 ? cmd - 1 : -1;
}

/* ---- 5 x 5 preset picker: an owner-drawn popup grid (column = glyph design, row = frame) ---- */
static int g_pickMode, g_pickSlot, g_pickCur;     /* mode 0: one button, mode 1: all buttons (min/max/close trio) */
static void MeasureMenuPreset(MEASUREITEMSTRUCT *mi) { mi->itemWidth = g_pickMode ? 114 : 50; mi->itemHeight = 38; }
static void DrawMenuPreset(const DRAWITEMSTRUCT *d) {
    const Theme *t = g_pickMode ? CurTheme() : &g_lab; int pr = (int)d->itemData; HDC dc = d->hDC; RECT rc = d->rcItem;
    BOOL sel = (d->itemState & ODS_SELECTED) != 0; COLORREF bg = pr ? t->tBot : UiPanel(), fg; PState s;
    FillRectC(dc, rc.left, rc.top, rc.right, rc.bottom, bg);
    if (!pr) { SetBkMode(dc, TRANSPARENT); SetTextColor(dc, UiText()); SelectObject(dc, g_fSmall); DrawTextW(dc, L"Default", -1, &rc, DT_SINGLELINE | DT_VCENTER | DT_CENTER); }
    else {
        memset(&s, 0, sizeof s); s.hot = -1; s.down = -1; s.active = TRUE; fg = Lum(bg) < 150 ? W_ : K_;
        if (g_pickMode) {
            static const int ids[3] = { BT_MIN, BT_MAX, BT_CLOSE }; int k;
            for (k = 0; k < 3; k++) { RECT r = rc; r.left = rc.left + 4 + k * 35; r.right = r.left + 34; DrawPresetGlyph(dc, ids[k], &r, pr, fg, &s, t->pin); }
        } else {
            int id = g_pickSlot; RECT r = rc;
            if (id == ICN_RESTORE) { id = BT_MAX; s.maxed = TRUE; }
            s.pinned = (id == BT_PIN); r.left += 2; r.right -= 2; DrawPresetGlyph(dc, id, &r, pr, fg, &s, t->pin);
        }
    }
    if (sel || (pr && pr == g_pickCur)) {
        HBRUSH b = CreateSolidBrush(sel ? UiAccent() : W_); RECT f = rc;
        FrameRect(dc, &f, b); InflateRect(&f, -1, -1); FrameRect(dc, &f, b); DeleteObject(b);
    }
}
/* returns -1 cancel, 0 default, 1..25 preset */
static int PresetPick(POINT pt, int mode, int slot, int cur) {
    HMENU mn = CreatePopupMenu(); int v, f, cmd;
    if (!mn) return -1;
    g_pickMode = mode; g_pickSlot = slot; g_pickCur = cur;
    for (v = 0; v < 5; v++) for (f = 0; f < 5; f++) {
        int p = v * 5 + f + 1;
        AppendMenuW(mn, MF_OWNERDRAW | (f == 0 && v > 0 ? MF_MENUBREAK : 0), (UINT_PTR)p, (LPCWSTR)(ULONG_PTR)p);
    }
    AppendMenuW(mn, MF_OWNERDRAW | MF_MENUBREAK, (UINT_PTR)26, (LPCWSTR)(ULONG_PTR)0);
    cmd = TrackPopupMenu(mn, TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, g_main, NULL);
    DestroyMenu(mn);
    if (cmd == 26) return 0;
    return (cmd >= 1 && cmd <= 25) ? cmd : -1;
}

static BOOL PickColor(COLORREF *io) {
    static COLORREF cust[16]; static int pi; CHOOSECOLORW cc;
    memset(&cc, 0, sizeof cc); cc.lStructSize = sizeof cc; cc.hwndOwner = g_main; cc.rgbResult = *io; cc.lpCustColors = cust;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (ChooseColorW(&cc)) { *io = cc.rgbResult; return TRUE; }
    if (CommDlgExtendedError() != 0) { *io = PALETTE[pi++ & 7]; SetStatus(L"Colour dialog unavailable - using a preset colour."); return TRUE; }
    return FALSE;
}
static void EditProp(int i) {
    const Prop *p = &PROPS[i]; BYTE *base = (BYTE *)&g_lab; RECT ir; POINT pt; int r;
    SendMessageW(g_props, LB_GETITEMRECT, i, (LPARAM)&ir);
    pt.x = ir.left + 60; pt.y = ir.bottom; ClientToScreen(g_props, &pt);
    switch (p->type) {
    case PT_INT: {
        static wchar_t nb[48][8]; const wchar_t *it[48]; int n = 0, v, cur = -1, *pv = (int *)(base + p->off);
        for (v = p->a; v <= p->b && n < 48; v += p->c) { _snwprintf(nb[n], 7, L"%d", v); nb[n][7] = 0; it[n] = nb[n]; if (*pv == v) cur = n; n++; }
        r = PopupChoose(pt, it, n, cur); if (r >= 0) *pv = p->a + r * p->c; break; }
    case PT_ENUM: { int *pv = (int *)(base + p->off); r = PopupChoose(pt, p->opts, p->a, *pv); if (r >= 0) *pv = r; break; }
    case PT_COLOR: { COLORREF *pc = (COLORREF *)(base + p->off), c = *pc; if (PickColor(&c)) { if (!c) c = RGB(1,1,1); *pc = c; } break; }
    case PT_FONT: r = PopupChoose(pt, FONTS, NFONTS, -1); if (r >= 0) { wcscpy(g_lab.font, FONTS[r]); } break;
    case PT_PRESET: r = PresetPick(pt, 0, p->a, g_lab.preset[p->a]); if (r >= 0) g_lab.preset[p->a] = r; break;
    case PT_ICON: {
        static const wchar_t *it[2] = { L"Choose image (.ico .png .bmp)...", L"Clear (use built-in icon)" };
        r = PopupChoose(pt, it, 2, -1);
        if (r == 0) {
            wchar_t f[MAX_PATH] = L"", rel[MAX_PATH], full[MAX_PATH], *dot; OPENFILENAMEW o;
            memset(&o, 0, sizeof o); o.lStructSize = sizeof o; o.hwndOwner = g_main;
            o.lpstrFilter = L"Icons (*.ico;*.png;*.bmp)\0*.ico;*.png;*.bmp\0All files\0*.*\0";
            o.lpstrFile = f; o.nMaxFile = MAX_PATH; o.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
            if (GetOpenFileNameW(&o)) {
                dot = wcsrchr(f, L'.');
                _snwprintf(rel, MAX_PATH - 1, L"icons\\ico%lu_%d%ls", (unsigned long)GetTickCount(), p->a, dot ? dot : L""); rel[MAX_PATH - 1] = 0;
                wcscpy(full, g_themeDir); wcscat(full, rel);
                if (CopyFileW(f, full, FALSE)) wcscpy(g_lab.icon[p->a], rel); else wcsncpy(g_lab.icon[p->a], f, MAX_PATH - 1);
                ImgClear();
            }
        } else if (r == 1) g_lab.icon[p->a][0] = 0;
        break; }
    }
    InvalidateRect(g_props, NULL, FALSE);
    if (g_main) InvalidateRect(g_main, NULL, FALSE);
}

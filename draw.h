/* draw.h - title bar painting (shared by overlays, gallery and previews) */
typedef struct { const wchar_t *title; HICON icon; BOOL active, maxed, pinned, shaded; int hot, down, hide; } PState;

/* Buttons are placed from the anchored edge outward; hidden ones take no space. */
static void LayoutBtns(const Theme *t, int w, RECT *r, int *vis, int *tl, int *tr, int hide) {
    static const int seqR[BT_COUNT] = { BT_CLOSE, BT_MAX, BT_MIN, BT_PIN, BT_SHADE, BT_SNAPR, BT_SNAPL };
    static const int seqL[BT_COUNT] = { BT_CLOSE, BT_MIN, BT_MAX, BT_PIN, BT_SHADE, BT_SNAPL, BT_SNAPR };
    const int *seq = t->left ? seqL : seqR; int i, x, n = 0;
    for (i = 0; i < BT_COUNT; i++) { vis[i] = 0; SetRectEmpty(&r[i]); }
    x = t->left ? t->rmargin : w - t->rmargin;
    *tl = 8; *tr = w - 8;
    for (i = 0; i < BT_COUNT; i++) {
        int id = seq[i], bw = (id == BT_CLOSE) ? t->closeW : t->bw;
        if (hide & (1 << id)) continue;
        if (n == 3 || (n == 0 && i == 3)) { /* separator before the extra buttons */ }
        if (i == 3) x += t->left ? 6 : -6;
        if (t->left) {
            if (x + bw > w - 110) break;
            SetRect(&r[id], x, t->bt, x + bw, t->bt + t->bh); x += bw + t->gap; *tl = r[id].right + 8;
        } else {
            if (x - bw < 110) break;
            x -= bw; SetRect(&r[id], x, t->bt, x + bw, t->bt + t->bh); x -= t->gap; *tr = r[id].left - 8;
        }
        vis[id] = 1; n++;
    }
}

static void RoundBtn(HDC dc, const RECT *rc, int rad, COLORREF top, COLORREF bot, COLORREF bord) {
    HPEN p = CreatePen(PS_SOLID, 1, bord); HBRUSH b = CreateSolidBrush(bot);
    HPEN op = (HPEN)SelectObject(dc, p); HBRUSH ob = (HBRUSH)SelectObject(dc, b);
    RoundRect(dc, rc->left, rc->top, rc->right, rc->bottom, rad, rad);
    SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b); DeleteObject(p);
    if (top != bot) FillRectC(dc, rc->left + 2, rc->top + 1, rc->right - 2, (rc->top + rc->bottom) / 2, top);
}

static void DrawBevel(HDC dc, const RECT *rc, COLORREF face, int sunken) {
    COLORREF hi = W_, lo = RGB(64,64,64), mid = RGB(128,128,128), tl = sunken ? lo : hi, br = sunken ? hi : lo;
    FillRectC(dc, rc->left, rc->top, rc->right, rc->bottom, face);
    FillRectC(dc, rc->left, rc->top, rc->right - 1, rc->top + 1, tl);
    FillRectC(dc, rc->left, rc->top, rc->left + 1, rc->bottom - 1, tl);
    FillRectC(dc, rc->left, rc->bottom - 1, rc->right, rc->bottom, br);
    FillRectC(dc, rc->right - 1, rc->top, rc->right, rc->bottom, br);
    FillRectC(dc, rc->left + 1, rc->bottom - 2, rc->right - 1, rc->bottom - 1, sunken ? tl : mid);
    FillRectC(dc, rc->right - 2, rc->top + 1, rc->right - 1, rc->bottom - 1, sunken ? tl : mid);
}

static void DrawBtnFace(HDC dc, const Theme *t, int id, const RECT *rc, int st, BOOL active) {
    BOOL cl = (id == BT_CLOSE); int h = rc->bottom - rc->top; COLORREF face, bord;
    if (t->shape == 0) {
        if (st) FillRectC(dc, rc->left, rc->top, rc->right, rc->bottom,
                          cl ? (st == 2 ? Blend(t->closeHover, K_, 15) : t->closeHover) : (st == 2 ? t->bPress : t->bHover));
        else if (cl && t->closeFace)
            FillRectC(dc, rc->left, rc->top, rc->right, rc->bottom, active ? t->closeFace : Blend(t->closeFace, RGB(200,200,200), 40));
        return;
    }
    if (t->shape == 4) { DrawBevel(dc, rc, (cl && t->closeFace) ? t->closeFace : t->bFace, st == 2); return; }
    face = (cl && t->closeFace) ? t->closeFace : t->bFace;
    if (st == 1) face = cl ? t->closeHover : t->bHover;
    if (st == 2) face = cl ? Blend(t->closeHover, K_, 15) : t->bPress;
    if (!active) face = Blend(face, RGB(200,200,200), 35);
    bord = t->bBorder ? t->bBorder : Blend(face, K_, 28);
    if (t->shape == 2) {
        HPEN p = CreatePen(PS_SOLID, 1, bord); HBRUSH b = CreateSolidBrush(face);
        HPEN op = (HPEN)SelectObject(dc, p); HBRUSH ob = (HBRUSH)SelectObject(dc, b);
        Ellipse(dc, rc->left, rc->top, rc->right, rc->bottom);
        SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b); DeleteObject(p);
    } else {
        COLORREF top = (t->shape == 3) ? Blend(face, W_, 45) : Blend(face, W_, 12);
        RoundBtn(dc, rc, t->shape == 3 ? 5 : (h / 3 < 5 ? 5 : h / 3), top, face, bord);
    }
}

/* Built-in icon sets; every shape is drawn with GDI so no icon fonts are needed on Wine. */
static void DrawGlyph(HDC dc, int id, const RECT *rc, int style, COLORREF c, const PState *s, BOOL hot, COLORREF pinC) {
    int cx = (rc->left + rc->right) / 2, cy = (rc->top + rc->bottom) / 2, pw; BOOL fill;
    HPEN pen, op; HBRUSH hb, ob; COLORREF gc;
    if (style == 2) {
        COLORREF d; HPEN p; HBRUSH b;
        switch (id) { case BT_CLOSE: d = RGB(255,95,86); break; case BT_MAX: d = RGB(39,201,63); break;
                      case BT_MIN: d = RGB(255,189,46); break; case BT_PIN: d = s->pinned ? pinC : RGB(160,160,160); break;
                      case BT_SHADE: d = RGB(180,120,255); break; default: d = RGB(90,140,255); }
        if (hot) d = Blend(d, W_, 30);
        p = CreatePen(PS_SOLID, 1, Blend(d, K_, 30)); b = CreateSolidBrush(d);
        op = (HPEN)SelectObject(dc, p); ob = (HBRUSH)SelectObject(dc, b);
        Ellipse(dc, cx - 7, cy - 7, cx + 8, cy + 8);
        SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b); DeleteObject(p);
        return;
    }
    if (style == 3) {
        int hh = (rc->bottom - rc->top) / 2 - 1; HPEN p; if (hh > 9) hh = 9;
        p = CreatePen(PS_SOLID, 1, c);
        op = (HPEN)SelectObject(dc, p); ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
        RoundRect(dc, cx - hh, cy - hh, cx + hh + 1, cy + hh + 1, 4, 4);
        SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(p);
    }
    pw = (style == 1 || style == 4) ? 2 : 1; fill = (style == 4);
    gc = (id == BT_PIN && s->pinned) ? pinC : c;
    pen = CreatePen(PS_SOLID, pw, gc);
    hb = fill ? CreateSolidBrush(gc) : (HBRUSH)GetStockObject(NULL_BRUSH);
    op = (HPEN)SelectObject(dc, pen); ob = (HBRUSH)SelectObject(dc, hb);
    switch (id) {
    case BT_MIN:
        if (fill) Rectangle(dc, cx - 5, cy + 1, cx + 6, cy + 4);
        else { MoveToEx(dc, cx - 5, cy + 2, NULL); LineTo(dc, cx + 6, cy + 2); }
        break;
    case BT_MAX:
        if (!s->maxed) Rectangle(dc, cx - 5, cy - 5, cx + 6, cy + 6);
        else {
            MoveToEx(dc, cx - 3, cy - 3, NULL); LineTo(dc, cx - 3, cy - 6); LineTo(dc, cx + 6, cy - 6);
            LineTo(dc, cx + 6, cy + 3); LineTo(dc, cx + 3, cy + 3);
            Rectangle(dc, cx - 6, cy - 3, cx + 4, cy + 6);
        }
        break;
    case BT_CLOSE: {
        HPEN cp = CreatePen(PS_SOLID, fill ? 3 : pw, gc); HPEN old = (HPEN)SelectObject(dc, cp);
        MoveToEx(dc, cx - 5, cy - 5, NULL); LineTo(dc, cx + 6, cy + 6);
        MoveToEx(dc, cx + 5, cy - 5, NULL); LineTo(dc, cx - 6, cy + 6);
        SelectObject(dc, old); DeleteObject(cp); break; }
    case BT_SNAPL:
        Rectangle(dc, cx - 6, cy - 5, cx + 7, cy + 7); FillRectC(dc, cx - 5, cy - 4, cx, cy + 6, gc); break;
    case BT_SNAPR:
        Rectangle(dc, cx - 6, cy - 5, cx + 7, cy + 7); FillRectC(dc, cx + 1, cy - 4, cx + 6, cy + 6, gc); break;
    case BT_SHADE: {   /* roll-up: bar + arrow (arrow flips when rolled up) */
        POINT p[3]; HBRUSH sb = CreateSolidBrush(gc); HBRUSH o2 = (HBRUSH)SelectObject(dc, sb);
        MoveToEx(dc, cx - 5, cy - 5, NULL); LineTo(dc, cx + 6, cy - 5);
        if (!s->shaded) { p[0].x = cx - 5; p[0].y = cy + 4; p[1].x = cx + 5; p[1].y = cy + 4; p[2].x = cx; p[2].y = cy - 2; }
        else            { p[0].x = cx - 5; p[0].y = cy - 2; p[1].x = cx + 5; p[1].y = cy - 2; p[2].x = cx; p[2].y = cy + 4; }
        Polygon(dc, p, 3); SelectObject(dc, o2); DeleteObject(sb); break; }
    case BT_PIN: {     /* push-pin: head, bar, needle */
        HBRUSH pb = s->pinned ? CreateSolidBrush(pinC) : NULL; if (pb) SelectObject(dc, pb);
        Ellipse(dc, cx - 4, cy - 7, cx + 5, cy + 2);
        MoveToEx(dc, cx - 5, cy + 2, NULL); LineTo(dc, cx + 6, cy + 2);
        MoveToEx(dc, cx, cy + 2, NULL); LineTo(dc, cx, cy + 8);
        if (pb) { SelectObject(dc, hb); DeleteObject(pb); }
        break; }
    }
    SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(pen);
    if (fill) DeleteObject(hb);
}


/* ---- 25 icon presets per button: preset = variant(0..4) * 5 + frame(0..4) + 1 ----
   variants are 5 different glyph designs per button; frames are: none, ring, filled disc,
   rounded outline, filled rounded square (glyph colour inverts on filled frames).          */
static POINT Pt(int x, int y) { POINT p; p.x = x; p.y = y; return p; }
static void Ln(HDC dc, int x1, int y1, int x2, int y2) { MoveToEx(dc, x1, y1, NULL); LineTo(dc, x2, y2); }
static void FillR(HDC dc, int x1, int y1, int x2, int y2, COLORREF c) {
    int t; if (x1 > x2) { t = x1; x1 = x2; x2 = t; } if (y1 > y2) { t = y1; y1 = y2; y2 = t; }
    FillRectC(dc, x1, y1, x2 + 1, y2 + 1, c);
}
static void DrawShape(HDC dc, int id, int v, int cx, int cy, int q, COLORREF col, const PState *s, COLORREF pinC) {
    HPEN pen = CreatePen(PS_SOLID, 2, col), op; HBRUSH br = CreateSolidBrush(col), nb = (HBRUSH)GetStockObject(NULL_BRUSH), ob;
    POINT p[10]; int mx = (id == BT_SNAPR) ? -1 : 1, sg = s->shaded ? -1 : 1, i, h = q / 2;
    static const int SX[10] = { 0, 24, 95, 38, 59, 0, -59, -38, -95, -24 }, SY[10] = { -100, -32, -31, 12, 81, 40, 81, 12, -31, -32 };
    op = (HPEN)SelectObject(dc, pen); ob = (HBRUSH)SelectObject(dc, nb);
#define X_(d) (cx + mx * (d))
#define Y_(d) (cy + sg * (d))
    switch (id) {
    case BT_MIN:
        switch (v) {
        case 0: Ln(dc, cx - q, cy + h, cx + q + 1, cy + h); break;
        case 1: p[0] = Pt(cx - q, cy - h); p[1] = Pt(cx, cy + h); p[2] = Pt(cx + q, cy - h); Polyline(dc, p, 3); break;
        case 2: SelectObject(dc, br); p[0] = Pt(cx - q, cy - h); p[1] = Pt(cx + q, cy - h); p[2] = Pt(cx, cy + h + 1); Polygon(dc, p, 3); break;
        case 3: FillR(dc, cx - q, cy + h - 1, cx + q, cy + h + 1, col); break;
        default: Ln(dc, cx, cy - q, cx, cy + q - 1); p[0] = Pt(cx - h - 1, cy + h); p[1] = Pt(cx, cy + q); p[2] = Pt(cx + h + 1, cy + h); Polyline(dc, p, 3); break;
        } break;
    case BT_MAX:
        if (!s->maxed) switch (v) {
            case 0: Rectangle(dc, cx - q, cy - q, cx + q + 1, cy + q + 1); break;
            case 1: Rectangle(dc, cx - q, cy - q, cx + q + 1, cy + q + 1); FillR(dc, cx - q, cy - q, cx + q, cy - q + 2, col); break;
            case 2: p[0] = Pt(cx - q, cy + h); p[1] = Pt(cx, cy - h); p[2] = Pt(cx + q, cy + h); Polyline(dc, p, 3); break;
            case 3: p[0] = Pt(cx, cy - q); p[1] = Pt(cx + q, cy); p[2] = Pt(cx, cy + q); p[3] = Pt(cx - q, cy); Polygon(dc, p, 4); break;
            default: { int l = h + 1;
                p[0] = Pt(cx - q, cy - q + l); p[1] = Pt(cx - q, cy - q); p[2] = Pt(cx - q + l, cy - q); Polyline(dc, p, 3);
                p[0] = Pt(cx + q - l, cy - q); p[1] = Pt(cx + q, cy - q); p[2] = Pt(cx + q, cy - q + l); Polyline(dc, p, 3);
                p[0] = Pt(cx - q, cy + q - l); p[1] = Pt(cx - q, cy + q); p[2] = Pt(cx - q + l, cy + q); Polyline(dc, p, 3);
                p[0] = Pt(cx + q - l, cy + q); p[1] = Pt(cx + q, cy + q); p[2] = Pt(cx + q, cy + q - l); Polyline(dc, p, 3); } break;
        } else switch (v) {   /* restore */
            case 0: case 1:
                p[0] = Pt(cx - q + 3, cy - q + 3); p[1] = Pt(cx - q + 3, cy - q); p[2] = Pt(cx + q, cy - q); p[3] = Pt(cx + q, cy + q - 3); p[4] = Pt(cx + q - 3, cy + q - 3);
                Polyline(dc, p, 5); Rectangle(dc, cx - q, cy - q + 3, cx + q - 2, cy + q + 1);
                if (v == 1) FillR(dc, cx - q, cy - q + 3, cx + q - 3, cy - q + 4, col); break;
            case 2: p[0] = Pt(cx - q, cy - h); p[1] = Pt(cx, cy + h); p[2] = Pt(cx + q, cy - h); Polyline(dc, p, 3); break;
            case 3:
                p[0] = Pt(cx - q, cy - 1); p[1] = Pt(cx - 1, cy - 1); p[2] = Pt(cx - 1, cy - q); Polyline(dc, p, 3);
                p[0] = Pt(cx + 1, cy - q); p[1] = Pt(cx + 1, cy - 1); p[2] = Pt(cx + q, cy - 1); Polyline(dc, p, 3);
                p[0] = Pt(cx - q, cy + 1); p[1] = Pt(cx - 1, cy + 1); p[2] = Pt(cx - 1, cy + q); Polyline(dc, p, 3);
                p[0] = Pt(cx + q, cy + 1); p[1] = Pt(cx + 1, cy + 1); p[2] = Pt(cx + 1, cy + q); Polyline(dc, p, 3); break;
            default: Rectangle(dc, cx - q, cy - q, cx + q + 1, cy + q + 1); FillR(dc, cx - h, cy - h, cx + h, cy + h, col); break;
        } break;
    case BT_CLOSE:
        switch (v) {
        case 0: Ln(dc, cx - q, cy - q, cx + q + 1, cy + q + 1); Ln(dc, cx + q, cy - q, cx - q - 1, cy + q + 1); break;
        case 1: Ln(dc, cx - q, cy, cx + q + 1, cy); Ln(dc, cx, cy - q, cx, cy + q + 1); break;
        case 2: Ln(dc, cx - q + 1, cy - q + 1, cx + q, cy + q); Ln(dc, cx + q - 1, cy - q + 1, cx - q, cy + q);
                Ln(dc, cx - q, cy, cx + q + 1, cy); Ln(dc, cx, cy - q, cx, cy + q + 1); break;
        case 3: Ln(dc, cx - q, cy - q, cx - 1, cy - 1); Ln(dc, cx + q, cy - q, cx + 1, cy - 1);
                Ln(dc, cx - q, cy + q, cx - 1, cy + 1); Ln(dc, cx + q, cy + q, cx + 1, cy + 1); break;
        default: SelectObject(dc, br);
                Ellipse(dc, cx - q - 1, cy - q - 1, cx - q + 2, cy - q + 2); Ellipse(dc, cx + q - 1, cy - q - 1, cx + q + 2, cy - q + 2);
                Ellipse(dc, cx - q - 1, cy + q - 1, cx - q + 2, cy + q + 2); Ellipse(dc, cx + q - 1, cy + q - 1, cx + q + 2, cy + q + 2);
                Ellipse(dc, cx - 1, cy - 1, cx + 2, cy + 2); break;
        } break;
    case BT_SHADE:
        switch (v) {
        case 0: Ln(dc, cx - q, Y_(-q), cx + q + 1, Y_(-q)); SelectObject(dc, br);
                p[0] = Pt(cx - q, Y_(q - 1)); p[1] = Pt(cx + q, Y_(q - 1)); p[2] = Pt(cx, Y_(-h)); Polygon(dc, p, 3); break;
        case 1: p[0] = Pt(cx - q, Y_(h)); p[1] = Pt(cx, Y_(-h)); p[2] = Pt(cx + q, Y_(h)); Polyline(dc, p, 3); break;
        case 2: SelectObject(dc, br); p[0] = Pt(cx - q, Y_(h)); p[1] = Pt(cx + q, Y_(h)); p[2] = Pt(cx, Y_(-h - 1)); Polygon(dc, p, 3); break;
        case 3: p[0] = Pt(cx - q, Y_(0)); p[1] = Pt(cx, Y_(-q + 1)); p[2] = Pt(cx + q, Y_(0)); Polyline(dc, p, 3);
                p[0] = Pt(cx - q, Y_(q)); p[1] = Pt(cx, Y_(1)); p[2] = Pt(cx + q, Y_(q)); Polyline(dc, p, 3); break;
        default: if (!s->shaded) { Rectangle(dc, cx - q, cy - q, cx + q + 1, cy + q + 1); FillR(dc, cx - q, cy - q, cx + q, cy - q + 2, col); }
                 else Rectangle(dc, cx - q, cy - 2, cx + q + 1, cy + 3); break;
        } break;
    case BT_PIN:
        if (s->pinned) SelectObject(dc, br);
        switch (v) {
        case 0: Ellipse(dc, cx - q + 1, cy - q, cx + q, cy + 1); Ln(dc, cx - q, cy + 1, cx + q + 1, cy + 1); Ln(dc, cx, cy + 1, cx, cy + q + 2); break;
        case 1: Rectangle(dc, cx - q, cy - q, cx + q + 1, cy - q + 3);
                p[0] = Pt(cx - q + 2, cy - q + 3); p[1] = Pt(cx + q - 2, cy - q + 3); p[2] = Pt(cx + 1, cy + 1); p[3] = Pt(cx - 1, cy + 1); Polygon(dc, p, 4);
                Ln(dc, cx, cy + 1, cx, cy + q + 2); break;
        case 2: SelectObject(dc, br); Ellipse(dc, cx - q, cy - q, cx + q + 1, cy + q + 1); break;
        case 3: Ln(dc, cx - q, cy - q, cx + q + 1, cy - q); Ln(dc, cx, cy - q + 2, cx, cy + q + 1);
                p[0] = Pt(cx - q + 1, cy); p[1] = Pt(cx, cy - q + 3); p[2] = Pt(cx + q - 1, cy); Polyline(dc, p, 3); break;
        default: for (i = 0; i < 10; i++) p[i] = Pt(cx + SX[i] * (q + 2) / 100, cy + SY[i] * (q + 2) / 100 + 1); Polygon(dc, p, 10); break;
        } break;
    case BT_SNAPL: case BT_SNAPR:
        switch (v) {
        case 0: Rectangle(dc, cx - q - 1, cy - q, cx + q + 2, cy + q + 1); FillR(dc, X_(-q), cy - q + 1, X_(0), cy + q - 1, col); break;
        case 1: Ln(dc, X_(q + 1), cy, X_(-q), cy); p[0] = Pt(X_(-q + 3), cy - 3); p[1] = Pt(X_(-q), cy); p[2] = Pt(X_(-q + 3), cy + 3); Polyline(dc, p, 3); break;
        case 2: p[0] = Pt(X_(h), cy - q); p[1] = Pt(X_(-h), cy); p[2] = Pt(X_(h), cy + q); Polyline(dc, p, 3); break;
        case 3: Rectangle(dc, cx - q - 1, cy - q, cx + q + 2, cy + q + 1); FillR(dc, X_(-q), cy - q + 1, X_(-q + 2), cy + q - 1, col); break;
        default: SelectObject(dc, br); p[0] = Pt(X_(q), cy - q); p[1] = Pt(X_(-q), cy); p[2] = Pt(X_(q), cy + q); Polygon(dc, p, 3); break;
        } break;
    }
#undef X_
#undef Y_
    SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(pen); DeleteObject(br);
}

static void DrawPresetGlyph(HDC dc, int id, const RECT *rc, int preset, COLORREF col, const PState *s, COLORREF pinC) {
    int v = (preset - 1) / 5, f = (preset - 1) % 5, cx = (rc->left + rc->right) / 2, cy = (rc->top + rc->bottom) / 2;
    int w = rc->right - rc->left, h = rc->bottom - rc->top, rad = (h < w ? h : w) / 2 - 1, q;
    COLORREF gc = (id == BT_PIN && s->pinned) ? pinC : col, inner = gc;
    if (rad > 10) rad = 10;
    q = f ? (rad >= 9 ? 4 : 3) : 5;
    if (f == 1 || f == 3) {
        HPEN p = CreatePen(PS_SOLID, 1, gc); HPEN op = (HPEN)SelectObject(dc, p); HBRUSH ob = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));
        if (f == 1) Ellipse(dc, cx - rad, cy - rad, cx + rad + 1, cy + rad + 1); else RoundRect(dc, cx - rad, cy - rad, cx + rad + 1, cy + rad + 1, 6, 6);
        SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(p);
    } else if (f == 2 || f == 4) {
        HPEN p = CreatePen(PS_SOLID, 1, gc); HBRUSH b = CreateSolidBrush(gc);
        HPEN op = (HPEN)SelectObject(dc, p); HBRUSH ob = (HBRUSH)SelectObject(dc, b);
        if (f == 2) Ellipse(dc, cx - rad, cy - rad, cx + rad + 1, cy + rad + 1); else RoundRect(dc, cx - rad, cy - rad, cx + rad + 1, cy + rad + 1, 6, 6);
        SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b); DeleteObject(p);
        inner = Lum(gc) < 150 ? W_ : K_;
    }
    DrawShape(dc, id, v, cx, cy, q, inner, s, pinC);
}

static BOOL DrawCustomIcon(HDC dc, const Theme *t, int id, const RECT *rc, const PState *s) {
    int slot = id; Img *im;
    if (id == BT_MAX && s->maxed && t->icon[ICN_RESTORE][0]) slot = ICN_RESTORE;
    if (!t->icon[slot][0]) return FALSE;
    im = ImgGet(t->icon[slot]);
    if (!im) return FALSE;
    DrawImg(dc, im, rc);
    if (id == BT_PIN && s->pinned) {   /* amber badge shows the pinned state on custom icons */
        HBRUSH b = CreateSolidBrush(t->pin); HBRUSH ob = (HBRUSH)SelectObject(dc, b);
        HPEN op = (HPEN)SelectObject(dc, GetStockObject(NULL_PEN));
        Ellipse(dc, rc->right - 10, rc->top + 2, rc->right - 3, rc->top + 9);
        SelectObject(dc, op); SelectObject(dc, ob); DeleteObject(b);
    }
    return TRUE;
}

static void PaintSkin(HDC dc, int w, int h, const Theme *t, const PState *s, HFONT font) {
    COLORREF top = s->active ? t->tTop : t->tTopI, bot = s->active ? t->tBot : t->tBotI;
    COLORREF fg = s->active ? t->txtA : t->txtI, bord = s->active ? t->border : t->borderI;
    RECT br[BT_COUNT]; int vis[BT_COUNT], tl, tr, y, x, id, tx; HFONT of; RECT trc; UINT fl;
    if (t->shape == 4) { for (x = 0; x < w; x += 3) FillRectC(dc, x, 0, x + 3 > w ? w : x + 3, h, Blend(top, bot, w > 1 ? x * 100 / (w - 1) : 0)); }
    else for (y = 0; y < h; y++) FillRectC(dc, 0, y, w, y + 1, top == bot ? top : Blend(top, bot, h > 1 ? y * 100 / (h - 1) : 0));
    FillRectC(dc, 0, 0, w, 1, bord); FillRectC(dc, 0, 0, 1, h, bord); FillRectC(dc, w - 1, 0, w, h, bord);
    if (t->shape == 4) { FillRectC(dc, 1, 1, w - 1, 2, RGB(192,192,192)); FillRectC(dc, 1, 1, 2, h, RGB(192,192,192)); FillRectC(dc, w - 2, 1, w - 1, h, RGB(128,128,128)); }
    LayoutBtns(t, w, br, vis, &tl, &tr, s->hide);
    tx = tl;
    if (s->icon) { DrawIconEx(dc, tx, (h - 16) / 2, s->icon, 16, 16, 0, NULL, DI_NORMAL); tx += 22; }
    of = (HFONT)SelectObject(dc, font); SetBkMode(dc, TRANSPARENT);
    trc.left = tx; trc.top = 0; trc.right = tr; trc.bottom = h;
    fl = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX | (t->center ? DT_CENTER : DT_LEFT);
    if (trc.right > trc.left && s->title) {
        if (t->shadow) { RECT sr = trc; OffsetRect(&sr, 1, 1); SetTextColor(dc, Blend(bot, K_, 60)); DrawTextW(dc, s->title, -1, &sr, fl); }
        SetTextColor(dc, fg); DrawTextW(dc, s->title, -1, &trc, fl);
    }
    SelectObject(dc, of);
    for (id = 0; id < BT_COUNT; id++) {
        int st; COLORREF gc;
        if (!vis[id]) continue;
        st = (s->hot == id) ? ((s->down == id) ? 2 : 1) : 0;
        if (t->gstyle != 2) DrawBtnFace(dc, t, id, &br[id], st, s->active);
        gc = t->glyph;
        if (id == BT_CLOSE && (st || (t->closeFace && t->closeFace != t->bFace))) gc = t->closeGlyph;
        if (!DrawCustomIcon(dc, t, id, &br[id], s)) {
            int slot = (id == BT_MAX && s->maxed) ? ICN_RESTORE : id, pr = t->preset[slot];
            if (!pr && slot == ICN_RESTORE) pr = t->preset[BT_MAX];
            if (pr > 0) DrawPresetGlyph(dc, id, &br[id], pr, gc, s, t->pin);
            else DrawGlyph(dc, id, &br[id], t->gstyle, gc, s, st != 0, t->pin);
        }
    }
}

/* ---- live preview: title bar + menu bar + toolbar + workspace + scroll bar + status bar ---- */
static void Bevel3(HDC dc, int l, int t, int r, int b, COLORREF face, COLORREF lt, COLORREF sh) {
    FillRectC(dc, l, t, r, b, face);
    FillRectC(dc, l, t, r - 1, t + 1, lt); FillRectC(dc, l, t, l + 1, b - 1, lt);
    FillRectC(dc, l, b - 1, r, b, sh); FillRectC(dc, r - 1, t, r, b, sh);
}
static void Arrow(HDC dc, int cx, int cy, int up, COLORREF c) {
    POINT p[3]; HBRUSH b = CreateSolidBrush(c); HPEN pn = CreatePen(PS_SOLID, 1, c);
    HBRUSH ob = (HBRUSH)SelectObject(dc, b); HPEN op = (HPEN)SelectObject(dc, pn);
    if (up) { p[0].x = cx - 3; p[0].y = cy + 2; p[1].x = cx + 4; p[1].y = cy + 2; p[2].x = cx; p[2].y = cy - 2; }
    else    { p[0].x = cx - 3; p[0].y = cy - 2; p[1].x = cx + 4; p[1].y = cy - 2; p[2].x = cx; p[2].y = cy + 2; }
    Polygon(dc, p, 3);
    SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b); DeleteObject(pn);
}

static void PaintThemePreview(HDC dc, RECT r, const Theme *t) {
    int w = r.right - r.left, h = r.bottom - r.top, mx = 8, my = 6, ww = w - 2 * mx, H = t->height, wh = h - 2 * my;
    HDC m = CreateCompatibleDC(dc), m2 = CreateCompatibleDC(dc);
    HBITMAP bm = CreateCompatibleBitmap(dc, w > 0 ? w : 1, h > 0 ? h : 1), bm2 = CreateCompatibleBitmap(dc, ww > 0 ? ww : 1, H);
    HBITMAP ob = NULL, ob2 = NULL; PState s; HFONT f; int y0, mh, sbw, wy0, wy1, i, th = 22, sth = 16;
    static const wchar_t *items[4] = { L"File", L"Edit", L"View", L"Help" };
    if (!m || !m2 || !bm || !bm2 || ww < 140 || wh < H + 70) goto done;
    ob = (HBITMAP)SelectObject(m, bm); ob2 = (HBITMAP)SelectObject(m2, bm2);
    FillRectC(m, 0, 0, w, h, g_dark ? RGB(18,18,18) : RGB(190,196,208));
    memset(&s, 0, sizeof s); s.title = L"Notepad - Preview"; s.icon = LoadIconW(NULL, IDI_APPLICATION);
    s.active = TRUE; s.hot = -1; s.down = -1; s.pinned = TRUE; s.hide = g_hideG;
    f = MakeTitleFont(t); PaintSkin(m2, ww, H, t, &s, f); DeleteObject(f);
    BitBlt(m, mx, my, ww, H, m2, 0, 0, SRCCOPY);
    y0 = my + H; mh = t->menuH; if (mh > 24) mh = 24;
    FillRectC(m, mx, y0, mx + ww, y0 + mh, t->sFace);                         /* menu bar */
    SetBkMode(m, TRANSPARENT); SetTextColor(m, t->sText); SelectObject(m, g_fSmall);
    for (i = 0; i < 4; i++) { RECT tr; tr.left = mx + 8 + i * 40; tr.top = y0; tr.right = tr.left + 40; tr.bottom = y0 + mh;
                              DrawTextW(m, items[i], -1, &tr, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX); }
    FillRectC(m, mx, y0 + mh, mx + ww, y0 + mh + th, t->sFace);               /* toolbar */
    for (i = 0; i < 4; i++) { int bx = mx + 6 + i * 24;
        Bevel3(m, bx, y0 + mh + 3, bx + 20, y0 + mh + th - 2, t->sFace, t->sLight, t->sShadow);
        FillRectC(m, bx + 7, y0 + mh + 8, bx + 13, y0 + mh + 14, t->sText); }
    wy0 = y0 + mh + th; wy1 = my + wh - sth;                                    /* workspace */
    FillRectC(m, mx, wy0, mx + ww, wy1, t->sWindow);
    for (i = 0; i < 3; i++) FillRectC(m, mx + 10, wy0 + 8 + i * 12, mx + 10 + ww / 2 - i * 30, wy0 + 12 + i * 12, Blend(t->sWindow, t->sText, 60));
    FillRectC(m, mx + 10, wy0 + 44, mx + 90, wy0 + 58, t->sHilite);
    FillRectC(m, mx + 14, wy0 + 49, mx + 70, wy0 + 53, t->sHiliteText);
    sbw = t->scrollW < 10 ? 10 : (t->scrollW > 22 ? 22 : t->scrollW);          /* scroll bar */
    FillRectC(m, mx + ww - sbw - 1, wy0, mx + ww - 1, wy1, t->sScroll);
    Bevel3(m, mx + ww - sbw - 1, wy0, mx + ww - 1, wy0 + sbw, t->sFace, t->sLight, t->sShadow);
    Bevel3(m, mx + ww - sbw - 1, wy1 - sbw, mx + ww - 1, wy1, t->sFace, t->sLight, t->sShadow);
    Arrow(m, mx + ww - sbw / 2 - 1, wy0 + sbw / 2, 1, t->sText); Arrow(m, mx + ww - sbw / 2 - 1, wy1 - sbw / 2, 0, t->sText);
    if (wy1 - wy0 > 2 * sbw + 16) Bevel3(m, mx + ww - sbw - 1, wy0 + sbw + 4, mx + ww - 1, wy0 + sbw + 4 + (wy1 - wy0) / 3, t->sFace, t->sLight, t->sShadow);
    FillRectC(m, mx, wy1, mx + ww, my + wh, t->sFace);                         /* status bar */
    FillRectC(m, mx, wy1, mx + ww, wy1 + 1, t->sShadow);
    { RECT tr; tr.left = mx + 8; tr.top = wy1; tr.right = mx + ww - 20; tr.bottom = my + wh;
      SetTextColor(m, t->sText); DrawTextW(m, L"Ready", -1, &tr, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX); }
    for (i = 0; i < t->borderW; i++) {                                         /* frame */
        FillRectC(m, mx + i, y0, mx + i + 1, my + wh, t->border); FillRectC(m, mx + ww - 1 - i, y0, mx + ww - i, my + wh, t->border);
        FillRectC(m, mx, my + wh - 1 - i, mx + ww, my + wh - i, t->border); }
    BitBlt(dc, r.left, r.top, w, h, m, 0, 0, SRCCOPY);
done:
    if (ob) SelectObject(m, ob);
    if (ob2) SelectObject(m2, ob2);
    if (bm) DeleteObject(bm);
    if (bm2) DeleteObject(bm2);
    if (m) DeleteDC(m);
    if (m2) DeleteDC(m2);
}

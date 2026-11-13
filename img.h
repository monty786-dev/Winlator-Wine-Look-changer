/* img.h - custom button icons from .ico / .png / .bmp (PNG via GDI+ loaded at run time) */
typedef struct { wchar_t path[MAX_PATH]; HICON ico; HBITMAP bmp; int w, h, kind; } Img; /* kind 1 ico, 2 png, 3 bmp */
static Img g_imgs[MAXIMG];
static int g_nimg;

static void ImgClear(void) {
    int i;
    for (i = 0; i < g_nimg; i++) {
        if (g_imgs[i].ico) DestroyIcon(g_imgs[i].ico);
        if (g_imgs[i].bmp) DeleteObject(g_imgs[i].bmp);
    }
    g_nimg = 0;
}

typedef int (WINAPI *PFN_Startup)(ULONG_PTR *, const void *, void *);
typedef int (WINAPI *PFN_FromFile)(const WCHAR *, void **);
typedef int (WINAPI *PFN_Dim)(void *, UINT *);
typedef int (WINAPI *PFN_Lock)(void *, const void *, UINT, INT, void *);
typedef int (WINAPI *PFN_Unlock)(void *, void *);
typedef int (WINAPI *PFN_Dispose)(void *);
typedef struct { UINT ver; void *cb; BOOL sbt, sec; } GdipInput;
typedef struct { INT x, y, w, h; } GdipRect;
typedef struct { UINT w, h; INT stride; INT fmt; void *scan0; UINT_PTR res; } GdipData;

static HBITMAP LoadPng(const wchar_t *path, int *pw, int *ph) {
    static HMODULE gp; static int init; static ULONG_PTR tok;
    static PFN_FromFile fFile; static PFN_Dim fW, fH; static PFN_Lock fLock; static PFN_Unlock fUnlock; static PFN_Dispose fDisp;
    void *bmp = NULL; UINT w = 0, h = 0; GdipRect r; GdipData bd; BITMAPINFO bi; void *bits = NULL; HBITMAP hb; UINT y;
    if (!init) {
        PFN_Startup st; GdipInput in;
        init = 1; gp = LoadLibraryW(L"gdiplus.dll");
        if (gp) {
            st = (PFN_Startup)GetProcAddress(gp, "GdiplusStartup");
            fFile = (PFN_FromFile)GetProcAddress(gp, "GdipCreateBitmapFromFile");
            fW = (PFN_Dim)GetProcAddress(gp, "GdipGetImageWidth"); fH = (PFN_Dim)GetProcAddress(gp, "GdipGetImageHeight");
            fLock = (PFN_Lock)GetProcAddress(gp, "GdipBitmapLockBits"); fUnlock = (PFN_Unlock)GetProcAddress(gp, "GdipBitmapUnlockBits");
            fDisp = (PFN_Dispose)GetProcAddress(gp, "GdipDisposeImage");
            in.ver = 1; in.cb = NULL; in.sbt = FALSE; in.sec = FALSE;
            if (!st || !fFile || !fW || !fH || !fLock || !fUnlock || !fDisp || st(&tok, &in, NULL) != 0) gp = NULL;
        }
    }
    if (!gp) return NULL;
    if (fFile(path, &bmp) != 0 || !bmp) return NULL;
    fW(bmp, &w); fH(bmp, &h);
    if (!w || !h || w > 1024 || h > 1024) { fDisp(bmp); return NULL; }
    r.x = 0; r.y = 0; r.w = (INT)w; r.h = (INT)h;
    memset(&bd, 0, sizeof bd);
    if (fLock(bmp, &r, 1, 0xE200B /* 32bpp PARGB */, &bd) != 0) { fDisp(bmp); return NULL; }
    memset(&bi, 0, sizeof bi);
    bi.bmiHeader.biSize = sizeof bi.bmiHeader; bi.bmiHeader.biWidth = (LONG)w; bi.bmiHeader.biHeight = -(LONG)h;
    bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
    hb = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (hb && bits) for (y = 0; y < h; y++) memcpy((BYTE *)bits + (size_t)y * w * 4, (BYTE *)bd.scan0 + (ptrdiff_t)y * bd.stride, (size_t)w * 4);
    fUnlock(bmp, &bd); fDisp(bmp);
    *pw = (int)w; *ph = (int)h;
    return hb;
}

static Img *ImgGet(const wchar_t *rel) {
    wchar_t apath[MAX_PATH], *dot; int i; Img *im;
    if (!rel[0]) return NULL;
    if (rel[1] == L':' || rel[0] == L'\\' || rel[0] == L'/') wcsncpy(apath, rel, MAX_PATH - 1);
    else { wcscpy(apath, g_themeDir); wcsncat(apath, rel, MAX_PATH - wcslen(apath) - 1); }
    apath[MAX_PATH - 1] = 0;
    for (i = 0; i < g_nimg; i++) if (!wcscmp(g_imgs[i].path, apath)) return g_imgs[i].kind ? &g_imgs[i] : NULL;
    if (g_nimg >= MAXIMG) ImgClear();
    im = &g_imgs[g_nimg++]; memset(im, 0, sizeof *im); wcscpy(im->path, apath);
    dot = wcsrchr(apath, L'.');
    if (dot && !_wcsicmp(dot, L".ico")) {
        im->ico = (HICON)LoadImageW(NULL, apath, IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
        if (im->ico) im->kind = 1;
    } else if (dot && !_wcsicmp(dot, L".png")) {
        im->bmp = LoadPng(apath, &im->w, &im->h);
        if (im->bmp) im->kind = 2;
    } else {
        im->bmp = (HBITMAP)LoadImageW(NULL, apath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        if (im->bmp) { BITMAP b; GetObject(im->bmp, sizeof b, &b); im->w = b.bmWidth; im->h = b.bmHeight; im->kind = 3; }
    }
    return im->kind ? im : NULL;
}

static void DrawImg(HDC dc, const Img *im, const RECT *rc) {
    int s = (rc->bottom - rc->top < rc->right - rc->left ? rc->bottom - rc->top : rc->right - rc->left) - 6;
    int dw, dh, x, y;
    if (s > 20) s = 20; if (s < 8) s = 8;
    dw = dh = s;
    if (im->kind != 1 && im->w > 0 && im->h > 0) { if (im->w > im->h) dh = s * im->h / im->w; else dw = s * im->w / im->h; }
    x = (rc->left + rc->right) / 2 - dw / 2; y = (rc->top + rc->bottom) / 2 - dh / 2;
    if (im->kind == 1) DrawIconEx(dc, x, y, im->ico, dw, dh, 0, NULL, DI_NORMAL);
    else {
        HDC m = CreateCompatibleDC(dc);
        if (m) {
            HBITMAP o = (HBITMAP)SelectObject(m, im->bmp);
            if (im->kind == 2) {
                BLENDFUNCTION bf; bf.BlendOp = AC_SRC_OVER; bf.BlendFlags = 0; bf.SourceConstantAlpha = 255; bf.AlphaFormat = AC_SRC_ALPHA;
                AlphaBlend(dc, x, y, dw, dh, m, 0, 0, im->w, im->h, bf);
            } else TransparentBlt(dc, x, y, dw, dh, m, 0, 0, im->w, im->h, RGB(255,0,255));
            SelectObject(m, o); DeleteDC(m);
        }
    }
}

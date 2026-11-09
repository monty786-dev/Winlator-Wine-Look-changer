/* theme.h - built-in themes, fixup, load/save, system colours */

static void ThemeFixup(Theme *t) {
    int dk;
    if (!t->font[0]) wcscpy(t->font, L"Segoe UI");
    if (t->fsize < 7 || t->fsize > 20) t->fsize = 9;
    if (t->height < 20 || t->height > 48) t->height = 30;
    if (t->bw < 12) t->bw = 40;
    if (t->bh < 10) t->bh = t->height;
    if (t->bh > t->height) t->bh = t->height;
    if (t->closeW < 12) t->closeW = t->bw;
    if (t->shape < 0 || t->shape > 4) t->shape = 0;
    if (t->gstyle < 0 || t->gstyle > 4) t->gstyle = 0;
    if (!t->tTopI) t->tTopI = Blend(t->tTop, RGB(205,205,205), 55);
    if (!t->tBotI) t->tBotI = Blend(t->tBot, RGB(205,205,205), 55);
    if (!t->txtI)  t->txtI  = Blend(t->txtA, RGB(128,128,128), 55);
    if (!t->borderI) t->borderI = Blend(t->border, RGB(190,190,190), 55);
    if (!t->glyph) t->glyph = t->txtA;
    if (!t->closeGlyph) t->closeGlyph = W_;
    if (!t->pin) t->pin = RGB(255,176,0);
    if (t->shape != 0 && !t->bFace) t->bFace = Blend(t->tBot, W_, 30);
    if (t->shape != 0 && !t->closeFace) t->closeFace = RGB(205,70,60);
    dk = Lum(t->tBot) < 150;
    if (!t->bHover) t->bHover = t->shape == 0 ? (dk ? Blend(t->tBot, W_, 22) : Blend(t->tBot, K_, 10))
                                              : Blend(t->bFace, W_, 28);
    if (!t->bPress) t->bPress = t->shape == 0 ? (dk ? Blend(t->tBot, W_, 38) : Blend(t->tBot, K_, 20))
                                              : Blend(t->bFace, K_, 18);
    if (!t->closeHover) t->closeHover = t->shape == 0 ? RGB(232,17,35) : Blend(t->closeFace, W_, 25);
    if (!t->sFace) t->sFace = RGB(240,240,240);
    if (!t->sWindow) t->sWindow = W_;
    if (!t->sHilite) t->sHilite = RGB(0,120,215);
    if (!t->sHiliteText) t->sHiliteText = W_;
    if (!t->sShadow) t->sShadow = Blend(t->sFace, K_, 35);
    if (!t->sLight)  t->sLight  = Blend(t->sFace, W_, 60);
    if (!t->sScroll) t->sScroll = Blend(t->sFace, t->sWindow, 50);
    if (!t->sWork)   t->sWork   = Blend(t->sFace, K_, 12);
    if (t->scrollW < 8 || t->scrollW > 30) t->scrollW = 17;
    if (t->menuH < 14 || t->menuH > 40) t->menuH = 20;
    if (t->borderW < 1 || t->borderW > 6) t->borderW = 1;
}

static int AddTheme(const Theme *t) {
    if (g_nthemes >= MAXTHEME) return -1;
    g_themes[g_nthemes] = *t;
    ThemeFixup(&g_themes[g_nthemes]);
    return g_nthemes++;
}

static void InitThemes(void) {
    Theme t;
    g_nthemes = 0;
#define DEF(...) memset(&t, 0, sizeof t); t = (Theme){ __VA_ARGS__ }; AddTheme(&t);

    DEF(.name=L"Windows 10", .height=31,.bw=46,.bh=31,.closeW=46,.fsize=9,.font=L"Segoe UI",
        .tTop=W_,.tBot=W_,.tTopI=W_,.tBotI=W_,.txtA=K_,.txtI=RGB(128,128,128),
        .border=RGB(0,120,215),.borderI=RGB(170,170,170),
        .sWindow=W_,.sFace=RGB(240,240,240),.sText=K_,.sHilite=RGB(0,120,215),.sShadow=RGB(160,160,160),
        .sLight=W_,.sScroll=RGB(240,240,240),.sWork=RGB(171,171,171),.scrollW=17,.menuH=20)

    DEF(.name=L"Windows 10 Dark", .height=31,.bw=46,.bh=31,.closeW=46,.fsize=9,.font=L"Segoe UI",
        .tTop=RGB(32,32,32),.tBot=RGB(32,32,32),.tTopI=RGB(43,43,43),.tBotI=RGB(43,43,43),
        .txtA=W_,.txtI=RGB(150,150,150),.border=RGB(60,60,60),.borderI=RGB(50,50,50),
        .glyph=W_,.bHover=RGB(62,62,62),
        .sWindow=RGB(30,30,30),.sFace=RGB(45,45,48),.sText=RGB(235,235,235),.sHilite=RGB(0,120,215),
        .sShadow=RGB(20,20,20),.sLight=RGB(80,80,80),.sScroll=RGB(37,37,38),.sWork=RGB(24,24,24),.scrollW=17)

    DEF(.name=L"Windows 11", .height=32,.radius=8,.bw=46,.bh=32,.closeW=46,.fsize=9,.font=L"Segoe UI",
        .tTop=RGB(243,243,243),.tBot=RGB(243,243,243),.tTopI=RGB(250,250,250),.tBotI=RGB(250,250,250),
        .txtA=K_,.txtI=RGB(140,140,140),.border=RGB(205,205,205),.borderI=RGB(225,225,225),
        .bHover=RGB(229,229,229),
        .sWindow=W_,.sFace=RGB(243,243,243),.sText=K_,.sHilite=RGB(0,103,192),.sShadow=RGB(190,190,190),
        .sLight=W_,.sScroll=RGB(249,249,249),.sWork=RGB(200,200,200),.scrollW=12,.menuH=22)

    DEF(.name=L"Windows 7 Aero", .shape=3,.height=30,.radius=7,.bw=28,.bh=18,.bt=1,.closeW=47,.rmargin=6,
        .fsize=9,.font=L"Segoe UI",
        .tTop=RGB(185,209,234),.tBot=RGB(125,160,205),.tTopI=RGB(215,228,242),.tBotI=RGB(185,205,225),
        .txtA=K_,.txtI=RGB(90,90,90),.border=RGB(40,60,90),
        .bFace=RGB(168,193,224),.bHover=RGB(190,212,238),.bPress=RGB(140,170,210),.bBorder=RGB(50,70,100),
        .glyph=RGB(30,40,60),.closeFace=RGB(196,64,38),.closeHover=RGB(225,100,75),.closeGlyph=W_,
        .sWindow=W_,.sFace=RGB(240,240,240),.sText=K_,.sHilite=RGB(51,153,255),.sShadow=RGB(160,160,160),
        .sLight=W_,.sScroll=RGB(233,236,240),.sWork=RGB(171,171,171),.scrollW=17)

    DEF(.name=L"Windows 8", .height=31,.bw=34,.bh=22,.closeW=47,.fsize=9,.font=L"Segoe UI",
        .tTop=RGB(70,134,198),.tBot=RGB(70,134,198),.tTopI=RGB(190,200,215),.tBotI=RGB(190,200,215),
        .txtA=W_,.txtI=RGB(220,230,240),.border=RGB(70,134,198),.borderI=RGB(190,200,215),
        .glyph=W_,.bHover=RGB(100,160,215),.closeFace=RGB(199,80,80),.closeHover=RGB(224,67,67),
        .sWindow=W_,.sFace=RGB(240,240,240),.sText=K_,.sHilite=RGB(70,134,198),.sShadow=RGB(160,160,160),
        .sLight=W_,.sScroll=RGB(240,240,240),.sWork=RGB(171,171,171),.scrollW=17)

    DEF(.name=L"Windows XP Luna", .shape=1,.height=30,.radius=8,.bw=21,.bh=21,.bt=5,.closeW=21,.gap=2,.rmargin=5,
        .bold=1,.shadow=1,.fsize=10,.font=L"Trebuchet MS",
        .tTop=RGB(40,120,250),.tBot=RGB(0,70,215),.tTopI=RGB(150,178,238),.tBotI=RGB(122,150,223),
        .txtA=W_,.txtI=RGB(216,228,248),.border=RGB(0,50,170),
        .bFace=RGB(36,102,238),.bHover=RGB(90,150,250),.bPress=RGB(20,70,190),.bBorder=W_,.glyph=W_,
        .closeFace=RGB(206,84,52),.closeHover=RGB(230,120,85),.closeGlyph=W_,
        .sWindow=W_,.sFace=RGB(236,233,216),.sText=K_,.sHilite=RGB(49,106,197),.sShadow=RGB(172,168,153),
        .sLight=W_,.sScroll=RGB(244,243,238),.sWork=RGB(128,128,128),.scrollW=17)

    DEF(.name=L"Windows 98 Classic", .shape=4,.gstyle=4,.height=24,.bw=16,.bh=14,.bt=5,.closeW=16,.gap=2,.rmargin=3,
        .bold=1,.fsize=8,.font=L"Tahoma",
        .tTop=RGB(0,0,128),.tBot=RGB(16,132,208),.tTopI=RGB(128,128,128),.tBotI=RGB(181,181,181),
        .txtA=W_,.txtI=RGB(192,192,192),.border=RGB(64,64,64),.borderI=RGB(64,64,64),
        .bFace=RGB(192,192,192),.bHover=RGB(192,192,192),.bPress=RGB(192,192,192),.glyph=RGB(0,0,1),
        .closeFace=RGB(192,192,192),.closeHover=RGB(192,192,192),.closeGlyph=RGB(0,0,1),
        .sWindow=W_,.sFace=RGB(192,192,192),.sText=K_,.sHilite=RGB(0,0,128),.sShadow=RGB(128,128,128),
        .sLight=W_,.sScroll=RGB(224,224,224),.sWork=RGB(128,128,128),.scrollW=16,.menuH=19)

    DEF(.name=L"Ubuntu Yaru", .shape=2,.height=34,.radius=8,.bw=24,.bh=24,.bt=5,.closeW=24,.gap=6,.rmargin=10,
        .center=1,.bold=1,.fsize=9,.font=L"Ubuntu",
        .tTop=RGB(66,63,59),.tBot=RGB(51,49,46),.tTopI=RGB(82,80,76),.tBotI=RGB(70,68,65),
        .txtA=RGB(223,219,210),.txtI=RGB(150,147,140),.border=RGB(33,31,29),
        .bFace=RGB(80,77,72),.bHover=RGB(100,96,90),.glyph=RGB(223,219,210),
        .closeFace=RGB(233,84,32),.closeHover=RGB(240,110,60),.closeGlyph=W_,
        .sWindow=W_,.sFace=RGB(246,245,244),.sText=RGB(60,60,60),.sHilite=RGB(233,84,32),
        .sShadow=RGB(190,188,184),.sLight=W_,.sScroll=RGB(235,233,231),.sWork=RGB(76,40,60),.scrollW=14,.menuH=24)

    DEF(.name=L"Xfce Greybird", .shape=1,.height=28,.radius=4,.bw=22,.bh=22,.bt=3,.closeW=22,.gap=4,.rmargin=6,
        .center=1,.bold=1,.fsize=9,.font=L"DejaVu Sans",
        .tTop=RGB(236,235,234),.tBot=RGB(206,204,201),.tTopI=RGB(245,244,243),.tBotI=RGB(228,227,225),
        .txtA=RGB(60,60,60),.txtI=RGB(140,140,140),.border=RGB(150,148,144),
        .bFace=RGB(236,235,233),.bHover=RGB(252,252,252),.bPress=RGB(210,208,205),.bBorder=RGB(160,158,154),
        .glyph=RGB(70,70,70),.closeFace=RGB(236,235,233),.closeHover=RGB(220,90,70),.closeGlyph=W_,
        .sWindow=W_,.sFace=RGB(237,236,235),.sText=RGB(60,60,60),.sHilite=RGB(82,148,226),
        .sShadow=RGB(170,168,164),.sLight=W_,.sScroll=RGB(228,227,225),.sWork=RGB(110,120,135),.scrollW=14,.menuH=22)

    DEF(.name=L"Linux Mint-Y Dark", .shape=2,.height=30,.radius=6,.bw=22,.bh=22,.bt=4,.closeW=22,.gap=5,.rmargin=8,
        .center=1,.fsize=9,.font=L"Noto Sans",
        .tTop=RGB(48,48,48),.tBot=RGB(40,40,40),.tTopI=RGB(62,62,62),.tBotI=RGB(55,55,55),
        .txtA=RGB(230,230,230),.txtI=RGB(140,140,140),.border=RGB(30,30,30),
        .bFace=RGB(70,70,70),.bHover=RGB(134,175,81),.glyph=RGB(230,230,230),
        .closeFace=RGB(70,70,70),.closeHover=RGB(230,80,70),.closeGlyph=W_,
        .sWindow=W_,.sFace=RGB(245,245,245),.sText=RGB(50,50,50),.sHilite=RGB(134,175,81),
        .sShadow=RGB(185,185,185),.sLight=W_,.sScroll=RGB(238,238,238),.sWork=RGB(60,70,60),.scrollW=14,.menuH=24)

    DEF(.name=L"GNOME Adwaita", .shape=2,.height=36,.radius=8,.bw=24,.bh=24,.bt=6,.closeW=24,.gap=6,.rmargin=10,
        .center=1,.bold=1,.fsize=10,.font=L"Cantarell",
        .tTop=RGB(235,235,235),.tBot=RGB(235,235,235),.tTopI=RGB(246,245,244),.tBotI=RGB(246,245,244),
        .txtA=RGB(46,52,54),.txtI=RGB(140,140,140),.border=RGB(205,199,194),
        .bFace=RGB(222,221,218),.bHover=RGB(205,203,198),.bBorder=RGB(205,199,194),.glyph=RGB(46,52,54),
        .closeFace=RGB(222,221,218),.closeHover=RGB(237,51,59),.closeGlyph=W_,
        .sWindow=W_,.sFace=RGB(250,250,250),.sText=RGB(46,52,54),.sHilite=RGB(53,132,228),
        .sShadow=RGB(200,198,194),.sLight=W_,.sScroll=RGB(243,242,240),.sWork=RGB(94,92,100),.scrollW=12,.menuH=24)

    DEF(.name=L"KDE Breeze", .height=30,.bw=28,.bh=30,.closeW=28,.fsize=9,.font=L"Noto Sans",
        .tTop=RGB(227,229,231),.tBot=RGB(227,229,231),.tTopI=RGB(239,240,241),.tBotI=RGB(239,240,241),
        .txtA=RGB(35,38,39),.txtI=RGB(140,140,140),.border=RGB(188,190,192),
        .bHover=RGB(207,212,216),.closeHover=RGB(218,68,83),.glyph=RGB(35,38,39),
        .sWindow=RGB(252,252,252),.sFace=RGB(239,240,241),.sText=RGB(35,38,39),.sHilite=RGB(61,174,233),
        .sShadow=RGB(188,190,192),.sLight=W_,.sScroll=RGB(239,240,241),.sWork=RGB(120,130,140),.scrollW=14,.menuH=22)

    DEF(.name=L"Nord (Arch)", .height=30,.bw=40,.bh=30,.closeW=40,.fsize=9,.font=L"DejaVu Sans",
        .tTop=RGB(46,52,64),.tBot=RGB(46,52,64),.tTopI=RGB(59,66,82),.tBotI=RGB(59,66,82),
        .txtA=RGB(216,222,233),.txtI=RGB(130,138,155),.border=RGB(76,86,106),
        .bHover=RGB(67,76,94),.closeHover=RGB(191,97,106),.glyph=RGB(216,222,233),
        .sWindow=RGB(46,52,64),.sFace=RGB(59,66,82),.sText=RGB(216,222,233),.sHilite=RGB(136,192,208),
        .sHiliteText=RGB(46,52,64),.sShadow=RGB(36,41,51),.sLight=RGB(76,86,106),.sScroll=RGB(46,52,64),
        .sWork=RGB(36,41,51),.scrollW=12,.menuH=22)

    DEF(.name=L"macOS Light", .shape=2,.gstyle=2,.height=28,.radius=10,.bw=22,.bh=22,.bt=3,.closeW=22,.gap=4,
        .rmargin=10,.left=1,.center=1,.fsize=9,.font=L"Helvetica",
        .tTop=RGB(236,236,236),.tBot=RGB(214,214,214),.tTopI=RGB(246,246,246),.tBotI=RGB(236,236,236),
        .txtA=RGB(76,76,76),.txtI=RGB(160,160,160),.border=RGB(190,190,190),
        .sWindow=W_,.sFace=RGB(246,246,246),.sText=RGB(40,40,40),.sHilite=RGB(0,122,255),
        .sShadow=RGB(205,205,205),.sLight=W_,.sScroll=RGB(250,250,250),.sWork=RGB(150,160,180),.scrollW=12,.menuH=22)
#undef DEF
    g_nbuiltin = g_nthemes;
}

static int FindTheme(const wchar_t *name) {
    int i; for (i = 0; i < g_nthemes; i++) if (!_wcsicmp(g_themes[i].name, name)) return i;
    return -1;
}

/* ---- .wlct theme files (INI format) ---- */
static void SanitizeName(const wchar_t *in, wchar_t *out, int n) {
    int i = 0; for (; in[i] && i < n - 1; i++) out[i] = iswalnum(in[i]) ? in[i] : L'_'; out[i] = 0;
    if (!out[0]) wcscpy(out, L"theme");
}
static void ThemeSave(const Theme *t, const wchar_t *file) {
    wchar_t b[32], k[16]; int i;
    WritePrivateProfileStringW(L"theme", L"name", t->name, file);
    WritePrivateProfileStringW(L"theme", L"font", t->font, file);
#define X(f) _snwprintf(b, 31, L"%lu", (unsigned long)t->f); b[31] = 0; \
             WritePrivateProfileStringW(L"theme", L"" #f, b, file);
    THEME_FIELDS(X)
#undef X
    for (i = 0; i < NICON; i++) {
        _snwprintf(k, 15, L"icon%d", i); k[15] = 0;
        WritePrivateProfileStringW(L"theme", k, t->icon[i][0] ? t->icon[i] : NULL, file);
    }
}
static BOOL ThemeLoad(Theme *t, const wchar_t *file) {
    wchar_t k[16]; int i;
    memset(t, 0, sizeof *t);
    GetPrivateProfileStringW(L"theme", L"name", L"", t->name, 48, file);
    if (!t->name[0]) return FALSE;
    GetPrivateProfileStringW(L"theme", L"font", L"Segoe UI", t->font, 32, file);
#define X(f) t->f = GetPrivateProfileIntW(L"theme", L"" #f, 0, file);
    THEME_FIELDS(X)
#undef X
    for (i = 0; i < NICON; i++) {
        _snwprintf(k, 15, L"icon%d", i); k[15] = 0;
        GetPrivateProfileStringW(L"theme", k, L"", t->icon[i], MAX_PATH, file);
    }
    t->custom = 1;
    ThemeFixup(t);
    return TRUE;
}
static void ThemeFilePath(const Theme *t, wchar_t *out) {
    wcscpy(out, g_themeDir); wcscat(out, t->file);
}
/* save as a new custom theme (unique name), returns index */
static int SaveNewCustom(Theme *t) {
    wchar_t base[48], path[MAX_PATH]; int n = 1, idx;
    wcsncpy(base, t->name, 47); base[47] = 0;
    if (!base[0]) wcscpy(base, L"Custom");
    while (FindTheme(t->name) >= 0) { _snwprintf(t->name, 47, L"%ls %d", base, ++n); t->name[47] = 0; }
    { wchar_t s[48]; SanitizeName(t->name, s, 48); _snwprintf(t->file, 63, L"%ls.wlct", s); t->file[63] = 0; }
    t->custom = 1;
    ThemeFilePath(t, path);
    ThemeSave(t, path);
    idx = AddTheme(t);
    return idx;
}
static void LoadCustomThemes(void) {
    WIN32_FIND_DATAW fd; wchar_t pat[MAX_PATH], path[MAX_PATH]; HANDLE h; Theme t;
    wcscpy(pat, g_themeDir); wcscat(pat, L"*.wlct");
    h = FindFirstFileW(pat, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        wcscpy(path, g_themeDir); wcscat(path, fd.cFileName);
        if (ThemeLoad(&t, path) && FindTheme(t.name) < 0) {
            wcsncpy(t.file, fd.cFileName, 63); t.file[63] = 0; AddTheme(&t);
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

/* ---- effective theme: accent colour and icon-style overrides ---- */
static void RebuildEff(void) {
    Theme *t; COLORREF c;
    if (g_themeIdx < 0 || g_themeIdx >= g_nthemes) g_themeIdx = 0;
    g_eff = g_themes[g_themeIdx]; t = &g_eff;
    if (g_iconOver >= 0 && g_iconOver <= 4) t->gstyle = g_iconOver;
    if (g_hasCol) {
        int dk, grad = (t->tTop != t->tBot); c = g_col; dk = Lum(c) < 150;
        t->tBot = c; t->tTop = grad ? Blend(c, W_, 25) : c;
        t->tTopI = Blend(t->tTop, RGB(205,205,205), 60); t->tBotI = Blend(c, RGB(205,205,205), 60);
        t->txtA = dk ? W_ : K_; t->txtI = dk ? Blend(c, W_, 70) : Blend(c, K_, 50);
        t->border = Blend(c, K_, 30); t->sHilite = c; t->sHiliteText = dk ? W_ : K_;
        if (t->shape == 0) {
            t->glyph = t->txtA;
            t->bHover = dk ? Blend(c, W_, 22) : Blend(c, K_, 10);
            t->bPress = dk ? Blend(c, W_, 38) : Blend(c, K_, 20);
        }
    }
}

/* ---- system colours / metrics: this is how toolbar, menu bar, status bar, scroll bars,
        workspace and borders of classic Win32/Wine controls follow the theme ---- */
static const int SYSIDX[] = { COLOR_SCROLLBAR, COLOR_ACTIVECAPTION, COLOR_INACTIVECAPTION, COLOR_MENU,
    COLOR_WINDOW, COLOR_WINDOWFRAME, COLOR_MENUTEXT, COLOR_WINDOWTEXT, COLOR_CAPTIONTEXT, COLOR_ACTIVEBORDER,
    COLOR_INACTIVEBORDER, COLOR_APPWORKSPACE, COLOR_HIGHLIGHT, COLOR_HIGHLIGHTTEXT, COLOR_BTNFACE,
    COLOR_BTNSHADOW, COLOR_GRAYTEXT, COLOR_BTNTEXT, COLOR_INACTIVECAPTIONTEXT, COLOR_BTNHIGHLIGHT,
    COLOR_3DDKSHADOW, COLOR_3DLIGHT, COLOR_INFOTEXT, COLOR_INFOBK, COLOR_HOTLIGHT,
    COLOR_GRADIENTACTIVECAPTION, COLOR_GRADIENTINACTIVECAPTION, COLOR_MENUHILIGHT, COLOR_MENUBAR };
static const wchar_t *SYSKEY[] = { L"Scrollbar", L"ActiveTitle", L"InactiveTitle", L"Menu", L"Window",
    L"WindowFrame", L"MenuText", L"WindowText", L"TitleText", L"ActiveBorder", L"InactiveBorder",
    L"AppWorkspace", L"Hilight", L"HilightText", L"ButtonFace", L"ButtonShadow", L"GrayText", L"ButtonText",
    L"InactiveTitleText", L"ButtonHilight", L"ButtonDkShadow", L"ButtonLight", L"InfoText", L"InfoWindow",
    L"HotTrackingColor", L"GradientActiveTitle", L"GradientInactiveTitle", L"MenuHilight", L"MenuBar" };
#define NSYS ((int)(sizeof SYSIDX / sizeof SYSIDX[0]))

static void SysValues(const Theme *t, COLORREF *v) {
    v[0] = t->sScroll; v[1] = t->tTop; v[2] = t->tTopI; v[3] = t->sFace; v[4] = t->sWindow;
    v[5] = t->border; v[6] = t->sText; v[7] = t->sText; v[8] = t->txtA; v[9] = t->border;
    v[10] = t->borderI; v[11] = t->sWork; v[12] = t->sHilite; v[13] = t->sHiliteText; v[14] = t->sFace;
    v[15] = t->sShadow; v[16] = Blend(t->sText, t->sFace, 55); v[17] = t->sText; v[18] = t->txtI;
    v[19] = t->sLight; v[20] = Blend(t->sShadow, K_, 40); v[21] = Blend(t->sFace, W_, 40);
    v[22] = K_; v[23] = RGB(255,255,225); v[24] = t->sHilite; v[25] = t->tBot; v[26] = t->tBotI;
    v[27] = t->sHilite; v[28] = t->sFace;
}
static void PersistColors(const COLORREF *v) {
    HKEY k; int i; wchar_t s[32];
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Colors", 0, NULL, 0, KEY_WRITE, NULL, &k, NULL) != ERROR_SUCCESS) return;
    for (i = 0; i < NSYS; i++) {
        _snwprintf(s, 31, L"%d %d %d", GetRValue(v[i]), GetGValue(v[i]), GetBValue(v[i])); s[31] = 0;
        RegSetValueExW(k, SYSKEY[i], 0, REG_SZ, (const BYTE *)s, (DWORD)((wcslen(s) + 1) * sizeof(wchar_t)));
    }
    RegCloseKey(k);
}
static BOOL GetNcm(NONCLIENTMETRICSW *n) {
    memset(n, 0, sizeof *n); n->cbSize = sizeof *n;
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, n->cbSize, n, 0)) return TRUE;
    n->cbSize = offsetof(NONCLIENTMETRICSW, iPaddedBorderWidth);
    return SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, n->cbSize, n, 0);
}
static void SetLf(LOGFONTW *lf, const Theme *t) {
    lf->lfHeight = -MulDiv(t->fsize, 96, 72); lf->lfWeight = FW_NORMAL;
    wcsncpy(lf->lfFaceName, t->font, LF_FACESIZE - 1); lf->lfFaceName[LF_FACESIZE - 1] = 0;
}
static void BackupSystemLook(void) {
    NONCLIENTMETRICSW n; int i; wchar_t k[16];
    if (GetPrivateProfileIntW(L"backup", L"done", 0, g_ini)) return;
    for (i = 0; i < NSYS; i++) {
        wchar_t b[16]; _snwprintf(k, 15, L"c%d", i); _snwprintf(b, 15, L"%lu", (unsigned long)GetSysColor(SYSIDX[i]));
        k[15] = b[15] = 0; WritePrivateProfileStringW(L"backup", k, b, g_ini);
    }
    if (GetNcm(&n)) WritePrivateProfileStructW(L"backup", L"ncm", &n, n.cbSize, g_ini);
    WritePrivateProfileStringW(L"backup", L"done", L"1", g_ini);
}
static void ApplySystemLook(const Theme *t) {
    COLORREF v[NSYS]; NONCLIENTMETRICSW n; int idx[NSYS], i;
    BackupSystemLook();
    SysValues(t, v);
    for (i = 0; i < NSYS; i++) idx[i] = SYSIDX[i];
    SetSysColors(NSYS, idx, v);
    PersistColors(v);
    if (GetNcm(&n)) {
        n.iScrollWidth = n.iScrollHeight = t->scrollW; n.iMenuHeight = t->menuH; n.iMenuWidth = t->menuH;
        n.iBorderWidth = t->borderW; n.iCaptionHeight = t->height; n.iCaptionWidth = t->height;
        SetLf(&n.lfMenuFont, t); SetLf(&n.lfStatusFont, t); SetLf(&n.lfMessageFont, t); SetLf(&n.lfCaptionFont, t);
        n.lfCaptionFont.lfWeight = t->bold ? FW_BOLD : FW_NORMAL;
        SystemParametersInfoW(SPI_SETNONCLIENTMETRICS, n.cbSize, &n, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    }
}
static BOOL RestoreSystemLook(void) {
    COLORREF v[NSYS]; int idx[NSYS], i; wchar_t k[16]; NONCLIENTMETRICSW n;
    if (!GetPrivateProfileIntW(L"backup", L"done", 0, g_ini)) return FALSE;
    for (i = 0; i < NSYS; i++) {
        _snwprintf(k, 15, L"c%d", i); k[15] = 0;
        v[i] = (COLORREF)GetPrivateProfileIntW(L"backup", k, 0, g_ini); idx[i] = SYSIDX[i];
    }
    SetSysColors(NSYS, idx, v); PersistColors(v);
    memset(&n, 0, sizeof n); n.cbSize = sizeof n;
    if (GetPrivateProfileStructW(L"backup", L"ncm", &n, n.cbSize, g_ini) ||
        (n.cbSize = offsetof(NONCLIENTMETRICSW, iPaddedBorderWidth),
         GetPrivateProfileStructW(L"backup", L"ncm", &n, n.cbSize, g_ini)))
        SystemParametersInfoW(SPI_SETNONCLIENTMETRICS, n.cbSize, &n, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    return TRUE;
}

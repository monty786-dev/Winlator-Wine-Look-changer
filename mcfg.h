/* mcfg.h - Monty settings: table-driven ints, strings, lists, icon cache, app scan */
#include <time.h>
#define CFG_INTS(X) \
 X(tbPos,0,0,3) X(tbAlign,0,0,2) X(tbHeight,44,24,90) X(tbAutoW,0,0,1) X(tbIcon,24,12,48) X(tbTitles,1,0,1) \
 X(tbRound,0,0,1) X(tbDetach,0,0,40) X(tbUseCol,0,0,1) X(tbCol,0x302820,0,0xFFFFFF) X(tbOpacity,100,30,100) \
 X(tbUnder,1,0,1) X(tbUnderH,3,1,8) X(tbUnderAdapt,1,0,1) X(tbDivider,1,0,3) X(tbDay,0,0,1) X(tbClock,1,0,1) \
 X(tbDesk,1,0,1) X(tbTray,1,0,1) X(tbPreview,1,0,1) X(pvW,220,120,480) X(pvH,150,80,360) X(tbClassic,0,0,1) \
 X(hideWine,1,0,1) X(fsMode,1,0,3) X(fsApplies,2,0,2) X(autoStart,1,0,1) X(startDelay,0,0,60) \
 X(rvOn,1,0,1) X(rvAlways,0,0,1) X(rvCol,0xE0A030,0,0xFFFFFF) X(rvPos,0,0,1) X(rvAlign,1,0,2) X(rvL,0,0,1) \
 X(rvOpacity,70,10,100) X(rvH,4,2,24) X(rvW,160,20,1600) X(rvSecs,4,1,30) X(rvAnim,1,0,3) \
 X(stSize,0,0,3) X(stIcon,0,0,17) X(stUseCol,0,0,1) X(stCol,0xE0A030,0,0xFFFFFF) X(stBarMode,0,0,2) X(stBarCol,0x2A2A2A,0,0xFFFFFF) \
 X(stBgMode,0,0,2) X(stBgCol,0x302820,0,0xFFFFFF) X(stOpacity,95,30,100) X(stTileCol,1,0,1) X(stTileAlpha,90,30,100) \
 X(stTileAnim,1,0,1) X(stTilePx,72,40,160) X(showApps,1,0,1) X(showTiles,1,0,1) \
 X(curOn,0,0,1) X(curHide,0,0,1) X(curCol,0xFFFFFF,0,0xFFFFFF) X(curBorder,0x000000,0,0xFFFFFF) X(curSize,32,16,64) \
 X(kbOn,0,0,1) X(kbClick,1,0,1) X(kbDelay,150,0,2000) \
 X(wlOn,1,0,1) X(wlAlpha,100,30,100) X(wlAll,1,0,1) \
 X(pEffect,0,0,3) X(pBlur,40,0,100) X(pAccOn,0,0,1) X(pAcc,0xD77800,0,0xFFFFFF) X(pShine,30,0,100) X(pGloss,30,0,100) \
 X(pShadow,1,0,1) X(pRound,0,0,1) X(effTb,1,0,1) X(effSt,1,0,1) X(effPv,1,0,1) X(effWin,1,0,1) X(effInst,1,0,1) \
 X(pIcon,0,0,14) X(pMode,0,0,6) X(pTint,1,0,1) X(pFill,1,0,1) X(fontSize,12,8,24) X(uiScale,100,70,170) \
 X(dkOn,0,0,1) X(dkGrid,84,56,160) X(dkSnap,1,0,1) X(dkCorner,0,0,3) X(dkIcons,1,0,1) X(dkClock,1,0,1) X(dkApps,0,0,1) \
 X(dkWallMode,0,0,4) X(dkLock,0,0,1) X(dkBgCol,0x504030,0,0xFFFFFF)

typedef struct {
#define X(n,d,lo,hi) int n;
    CFG_INTS(X)
#undef X
} Cfg;
static Cfg g;
static wchar_t g_mini[MAX_PATH];
static wchar_t S_stbg[MAX_PATH];
static wchar_t S_theme[48] = L"Windows 10", S_user[48] = L"User", S_pic[MAX_PATH], S_font[32] = L"Segoe UI", S_wall[MAX_PATH],
               S_kbCmd[MAX_PATH], S_cur[8][MAX_PATH];

typedef struct { wchar_t p[MAX_PATH]; int w, h; COLORREF col; } LItem;
enum { L_FAV, L_TILE, L_TB, L_TRAY, L_HIDE, L_RVX, L_KBX, L_COUNT };
#define MAXL 64
static LItem g_L[L_COUNT][MAXL];
static int g_nL[L_COUNT];
static const wchar_t *LNAME[L_COUNT] = { L"fav", L"tile", L"taskbar", L"tray", L"hide", L"revealx", L"kbdx" };

static int ScaleI(int v) { return v * g.uiScale / 100; }
static void CfgDefaults(void) {
#define X(n,d,lo,hi) g.n = d;
    CFG_INTS(X)
#undef X
}
static void CfgClamp(void) {
#define X(n,d,lo,hi) if (g.n < lo) g.n = lo; if (g.n > hi) g.n = hi;
    CFG_INTS(X)
#undef X
}
static int  MRd(const wchar_t *sec, const wchar_t *k, int d) { return (int)GetPrivateProfileIntW(sec, k, d, g_mini); }
static void MWr(const wchar_t *sec, const wchar_t *k, int v) { wchar_t b[24]; _snwprintf(b, 23, L"%d", v); b[23] = 0; WritePrivateProfileStringW(sec, k, b, g_mini); }
static void MWrS(const wchar_t *k, const wchar_t *v) { WritePrivateProfileStringW(L"str", k, v, g_mini); }

static void LoadLists(void) {
    int l, i; wchar_t k[12], v[MAX_PATH + 40], *a, *b;
    for (l = 0; l < L_COUNT; l++) {
        g_nL[l] = 0;
        for (i = 0; i < MAXL; i++) {
            LItem *it = &g_L[l][g_nL[l]];
            _snwprintf(k, 11, L"%d", i); k[11] = 0;
            GetPrivateProfileStringW(LNAME[l], k, L"", v, MAX_PATH + 40, g_mini);
            if (!v[0]) continue;
            it->w = 2; it->h = 2; it->col = 0;
            a = wcschr(v, L'|'); if (a) *a++ = 0;
            wcsncpy(it->p, v, MAX_PATH - 1); it->p[MAX_PATH - 1] = 0;
            if (a) { it->w = _wtoi(a); b = wcschr(a, L'|'); if (b) { it->h = _wtoi(++b); b = wcschr(b, L'|'); if (b) it->col = (COLORREF)_wtoi(b + 1); } }
            if (it->w < 1 || it->w > 4) it->w = 2;
            if (it->h < 1 || it->h > 4) it->h = 2;
            g_nL[l]++;
        }
    }
}
static void SaveList(int l) {
    int i; wchar_t k[12], v[MAX_PATH + 40];
    WritePrivateProfileStringW(LNAME[l], NULL, NULL, g_mini);
    for (i = 0; i < g_nL[l]; i++) {
        _snwprintf(k, 11, L"%d", i); k[11] = 0;
        _snwprintf(v, MAX_PATH + 39, L"%ls|%d|%d|%lu", g_L[l][i].p, g_L[l][i].w, g_L[l][i].h, (unsigned long)g_L[l][i].col); v[MAX_PATH + 39] = 0;
        WritePrivateProfileStringW(LNAME[l], k, v, g_mini);
    }
}
static int ListFind(int l, const wchar_t *p) { int i; for (i = 0; i < g_nL[l]; i++) if (!_wcsicmp(g_L[l][i].p, p)) return i; return -1; }
static BOOL ListAdd(int l, const wchar_t *p) {
    LItem *it;
    if (g_nL[l] >= MAXL || ListFind(l, p) >= 0) return FALSE;
    it = &g_L[l][g_nL[l]++]; memset(it, 0, sizeof *it);
    wcsncpy(it->p, p, MAX_PATH - 1); it->w = 2; it->h = 2; SaveList(l); return TRUE;
}
static void ListRemove(int l, int i) {
    int k; if (i < 0 || i >= g_nL[l]) return;
    for (k = i; k < g_nL[l] - 1; k++) g_L[l][k] = g_L[l][k + 1];
    g_nL[l]--; SaveList(l);
}

static void LoadCfg(void) {
    int i; wchar_t k[8];
    CfgDefaults();
#define X(n,d,lo,hi) g.n = MRd(L"cfg", L"" #n, d);
    CFG_INTS(X)
#undef X
    CfgClamp();
    GetPrivateProfileStringW(L"str", L"theme", L"Windows 10", S_theme, 48, g_mini);
    GetPrivateProfileStringW(L"str", L"user", L"User", S_user, 48, g_mini);
    GetPrivateProfileStringW(L"str", L"pic", L"", S_pic, MAX_PATH, g_mini);
    GetPrivateProfileStringW(L"str", L"font", L"Segoe UI", S_font, 32, g_mini);
    GetPrivateProfileStringW(L"str", L"wall", L"", S_wall, MAX_PATH, g_mini);
    GetPrivateProfileStringW(L"str", L"kbcmd", L"", S_kbCmd, MAX_PATH, g_mini);
    GetPrivateProfileStringW(L"str", L"stbg", L"", S_stbg, MAX_PATH, g_mini);
    for (i = 0; i < 8; i++) { _snwprintf(k, 7, L"cur%d", i); k[7] = 0; GetPrivateProfileStringW(L"str", k, L"", S_cur[i], MAX_PATH, g_mini); }
    LoadLists();
}
static void SaveCfg(void) {
    int i; wchar_t k[8];
#define X(n,d,lo,hi) MWr(L"cfg", L"" #n, g.n);
    CFG_INTS(X)
#undef X
    MWrS(L"theme", S_theme); MWrS(L"user", S_user); MWrS(L"pic", S_pic); MWrS(L"font", S_font); MWrS(L"wall", S_wall); MWrS(L"kbcmd", S_kbCmd); MWrS(L"stbg", S_stbg);
    for (i = 0; i < 8; i++) { _snwprintf(k, 7, L"cur%d", i); k[7] = 0; MWrS(k, S_cur[i]); }
    for (i = 0; i < L_COUNT; i++) SaveList(i);
}
static BOOL CfgBackup(void) { wchar_t b[MAX_PATH]; SaveCfg(); wcscpy(b, g_mini); wcscat(b, L".bak"); return CopyFileW(g_mini, b, FALSE); }
static BOOL CfgRestore(void) { wchar_t b[MAX_PATH]; wcscpy(b, g_mini); wcscat(b, L".bak"); if (!CopyFileW(b, g_mini, FALSE)) return FALSE; LoadCfg(); return TRUE; }
static void CfgReset(void) { DeleteFileW(g_mini); LoadCfg(); }

/* ---- icons of files / shortcuts (cached) ---- */
typedef struct { wchar_t p[MAX_PATH]; HICON ic; } IcEnt;
static IcEnt g_ic[200]; static int g_nic;
static HICON PathIcon(const wchar_t *path) {
    int i; SHFILEINFOW fi; HICON ic = NULL;
    for (i = 0; i < g_nic; i++) if (!_wcsicmp(g_ic[i].p, path)) return g_ic[i].ic;
    memset(&fi, 0, sizeof fi);
    if (SHGetFileInfoW(path, 0, &fi, sizeof fi, SHGFI_ICON | SHGFI_LARGEICON)) ic = fi.hIcon;
    if (!ic) ic = LoadIconW(NULL, IDI_APPLICATION);
    if (g_nic >= 200) { for (i = 0; i < g_nic; i++) if (g_ic[i].ic) DestroyIcon(g_ic[i].ic); g_nic = 0; }
    wcsncpy(g_ic[g_nic].p, path, MAX_PATH - 1); g_ic[g_nic].p[MAX_PATH - 1] = 0; g_ic[g_nic].ic = ic; g_nic++;
    return ic;
}
static void BaseName(const wchar_t *path, wchar_t *out, int n) {
    const wchar_t *s = wcsrchr(path, L'\\'), *e; int len;
    s = s ? s + 1 : path; e = wcsrchr(s, L'.'); len = e ? (int)(e - s) : (int)wcslen(s);
    if (len >= n) len = n - 1; wcsncpy(out, s, len); out[len] = 0;
}
static void BumpUsage(const wchar_t *path) {
    wchar_t v[48], o[48]; int cnt = 0;
    GetPrivateProfileStringW(L"usage", path, L"", o, 48, g_mini);
    if (o[0]) cnt = _wtoi(o);
    _snwprintf(v, 47, L"%d|%ld", cnt + 1, (long)time(NULL)); v[47] = 0;
    WritePrivateProfileStringW(L"usage", path, v, g_mini);
}
static void LaunchPath(const wchar_t *path) {
    wchar_t dir[MAX_PATH], *s; HINSTANCE r;
    wcsncpy(dir, path, MAX_PATH - 1); dir[MAX_PATH - 1] = 0; s = wcsrchr(dir, L'\\'); if (s) *s = 0; else dir[0] = 0;
    r = ShellExecuteW(NULL, L"open", path, NULL, dir[0] ? dir : NULL, SW_SHOWNORMAL);
    if ((INT_PTR)r > 32) BumpUsage(path);
}

/* ---- installed app scan (Start Menu .lnk files + system tools) ---- */
typedef struct { wchar_t name[64]; wchar_t path[MAX_PATH]; BYTE sys, ctl; int cnt; long last; } AppEnt;
#define MAXAPPS 600
static AppEnt g_apps[MAXAPPS]; static int g_napps;
static void AddApp(const wchar_t *path, BYTE sys, BYTE ctl) {
    AppEnt *a; wchar_t o[48]; int i;
    if (g_napps >= MAXAPPS) return;
    for (i = 0; i < g_napps; i++) if (!_wcsicmp(g_apps[i].path, path)) return;
    a = &g_apps[g_napps++]; memset(a, 0, sizeof *a);
    wcsncpy(a->path, path, MAX_PATH - 1); BaseName(path, a->name, 64); a->sys = sys; a->ctl = ctl;
    GetPrivateProfileStringW(L"usage", path, L"", o, 48, g_mini);
    if (o[0]) { wchar_t *bar = wcschr(o, L'|'); a->cnt = _wtoi(o); if (bar) a->last = _wtol(bar + 1); }
}
static void ScanDir(const wchar_t *dir, int depth) {
    WIN32_FIND_DATAW fd; wchar_t pat[MAX_PATH], p[MAX_PATH]; HANDLE h;
    if (depth > 3) return;
    _snwprintf(pat, MAX_PATH - 1, L"%ls\\*", dir); pat[MAX_PATH - 1] = 0;
    h = FindFirstFileW(pat, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        size_t n;
        if (fd.cFileName[0] == L'.') continue;
        _snwprintf(p, MAX_PATH - 1, L"%ls\\%ls", dir, fd.cFileName); p[MAX_PATH - 1] = 0;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ScanDir(p, depth + 1);
        else { n = wcslen(fd.cFileName); if (n > 4 && (!_wcsicmp(fd.cFileName + n - 4, L".lnk") || !_wcsicmp(fd.cFileName + n - 4, L".exe"))) AddApp(p, 0, 0); }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}
static int CmpApp(const void *a, const void *b) { return _wcsicmp(((const AppEnt *)a)->name, ((const AppEnt *)b)->name); }
static void ScanApps(void) {
    wchar_t d[MAX_PATH], w[MAX_PATH], p[MAX_PATH]; DWORD n; int i;
    static const wchar_t *sysx[] = { L"notepad.exe", L"wordpad.exe", L"calc.exe", L"cmd.exe", L"explorer.exe", L"taskmgr.exe", L"mspaint.exe", L"write.exe" };
    static const wchar_t *ctlx[] = { L"control.exe", L"winecfg.exe", L"regedit.exe", L"msiexec.exe", L"uninstaller.exe", L"wineboot.exe" };
    g_napps = 0;
    n = GetEnvironmentVariableW(L"ProgramData", d, MAX_PATH);
    if (n && n < MAX_PATH - 40) { wcscat(d, L"\\Microsoft\\Windows\\Start Menu\\Programs"); ScanDir(d, 0); }
    n = GetEnvironmentVariableW(L"APPDATA", d, MAX_PATH);
    if (n && n < MAX_PATH - 40) { wcscat(d, L"\\Microsoft\\Windows\\Start Menu\\Programs"); ScanDir(d, 0); }
    n = GetEnvironmentVariableW(L"USERPROFILE", d, MAX_PATH);
    if (n && n < MAX_PATH - 40) { wcscat(d, L"\\Desktop"); ScanDir(d, 2); }
    GetWindowsDirectoryW(w, MAX_PATH);
    for (i = 0; i < 8; i++) { _snwprintf(p, MAX_PATH - 1, L"%ls\\system32\\%ls", w, sysx[i]); p[MAX_PATH - 1] = 0; if (GetFileAttributesW(p) != INVALID_FILE_ATTRIBUTES) AddApp(p, 1, 0); }
    for (i = 0; i < 6; i++) { _snwprintf(p, MAX_PATH - 1, L"%ls\\system32\\%ls", w, ctlx[i]); p[MAX_PATH - 1] = 0; if (GetFileAttributesW(p) != INVALID_FILE_ATTRIBUTES) AddApp(p, 1, 1); }
    for (i = 0; i < g_nL[L_FAV]; i++) AddApp(g_L[L_FAV][i].p, 0, 0);
    qsort(g_apps, g_napps, sizeof(AppEnt), CmpApp);
}

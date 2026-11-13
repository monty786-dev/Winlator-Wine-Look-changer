/* mmisc.h - custom cursor, keyboard assist signal, window-look-changer bridge, autostart */

/* ---------- custom cursor ---------- */
static HBITMAP MkDib(HDC dc, int w, int h, void **bits) {
    BITMAPINFO bi; memset(&bi, 0, sizeof bi);
    bi.bmiHeader.biSize = sizeof bi.bmiHeader; bi.bmiHeader.biWidth = w; bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
    return CreateDIBSection(dc, &bi, DIB_RGB_COLORS, bits, NULL, 0);
}
static void ArrowPts(POINT *p, int s) {
    static const int X[7] = { 0, 0, 22, 36, 50, 37, 65 }, Y[7] = { 0, 80, 62, 95, 90, 58, 58 }; int i;
    p[0].x = X[0]; p[0].y = Y[0]; p[1].x = X[1]; p[1].y = Y[1]; p[2].x = X[2]; p[2].y = Y[2]; p[3].x = X[3]; p[3].y = Y[3];
    p[4].x = X[4]; p[4].y = Y[4]; p[5].x = X[5]; p[5].y = Y[5]; p[6].x = X[6]; p[6].y = Y[6];
    for (i = 0; i < 7; i++) { p[i].x = p[i].x * s / 100 + 1; p[i].y = p[i].y * s / 100 + 1; }
}
/* arrow cursor drawn with GDI: colour bitmap + 1-bit mask */
static HCURSOR MakeArrow(int sz, COLORREF fill, COLORREF border, BOOL blank) {
    HDC sdc = GetDC(NULL), cdc = CreateCompatibleDC(sdc), mdc = CreateCompatibleDC(sdc); void *bits = NULL; HBITMAP col = MkDib(sdc, sz, sz, &bits), mask = CreateBitmap(sz, sz, 1, 1, NULL);
    HCURSOR cur = NULL; POINT p[7];
    if (cdc && mdc && col && mask) {
        HBITMAP o1 = (HBITMAP)SelectObject(cdc, col), o2 = (HBITMAP)SelectObject(mdc, mask); ICONINFO ii;
        PatBlt(mdc, 0, 0, sz, sz, WHITENESS); FillRectC(cdc, 0, 0, sz, sz, K_);
        if (!blank) {
            HPEN pm = CreatePen(PS_SOLID, 1, K_), pc = CreatePen(PS_SOLID, 1, border); HBRUSH bm = CreateSolidBrush(K_), bc = CreateSolidBrush(fill);
            ArrowPts(p, sz);
            SelectObject(mdc, pm); SelectObject(mdc, bm); Polygon(mdc, p, 7);
            SelectObject(cdc, pc); SelectObject(cdc, bc); Polygon(cdc, p, 7);
            DeleteObject(pm); DeleteObject(pc); DeleteObject(bm); DeleteObject(bc);
        }
        SelectObject(cdc, o1); SelectObject(mdc, o2);
        memset(&ii, 0, sizeof ii); ii.fIcon = FALSE; ii.xHotspot = 0; ii.yHotspot = 0; ii.hbmMask = mask; ii.hbmColor = col;
        cur = (HCURSOR)CreateIconIndirect(&ii);
    }
    if (col) DeleteObject(col); if (mask) DeleteObject(mask); if (cdc) DeleteDC(cdc); if (mdc) DeleteDC(mdc); ReleaseDC(NULL, sdc);
    return cur;
}
static HCURSOR CursorFromFile(const wchar_t *f, int sz) {
    const wchar_t *dot = wcsrchr(f, L'.'); HCURSOR c = NULL;
    if (!f[0] || !dot) return NULL;
    if (!_wcsicmp(dot, L".cur") || !_wcsicmp(dot, L".ani")) return LoadCursorFromFileW(f);
    if (!_wcsicmp(dot, L".ico")) {
        HICON ic = (HICON)LoadImageW(NULL, f, IMAGE_ICON, sz, sz, LR_LOADFROMFILE); ICONINFO ii;
        if (ic && GetIconInfo(ic, &ii)) { ii.fIcon = FALSE; ii.xHotspot = 0; ii.yHotspot = 0; c = (HCURSOR)CreateIconIndirect(&ii); DeleteObject(ii.hbmColor); DeleteObject(ii.hbmMask); }
        if (ic) DestroyIcon(ic);
        return c;
    }
    {   int w = 0, h = 0; HBITMAP src = LoadPng(f, &w, &h);                      /* png / jpg / gif through GDI+ */
        if (src && w > 0 && h > 0) {
            HDC sdc = GetDC(NULL), a = CreateCompatibleDC(sdc), b = CreateCompatibleDC(sdc); void *bits = NULL; HBITMAP dst = MkDib(sdc, sz, sz, &bits), mask = CreateBitmap(sz, sz, 1, 1, NULL);
            if (a && b && dst && mask) {
                HBITMAP o1 = (HBITMAP)SelectObject(a, src), o2 = (HBITMAP)SelectObject(b, dst); BLENDFUNCTION bf; ICONINFO ii; HDC md = CreateCompatibleDC(sdc); HBITMAP o3 = (HBITMAP)SelectObject(md, mask);
                bf.BlendOp = AC_SRC_OVER; bf.BlendFlags = 0; bf.SourceConstantAlpha = 255; bf.AlphaFormat = AC_SRC_ALPHA;
                memset(bits, 0, (size_t)sz * sz * 4); AlphaBlend(b, 0, 0, sz, sz, a, 0, 0, w, h, bf);
                PatBlt(md, 0, 0, sz, sz, BLACKNESS);                       /* all opaque: alpha channel carries the shape */
                SelectObject(md, o3); DeleteDC(md); SelectObject(a, o1); SelectObject(b, o2);
                memset(&ii, 0, sizeof ii); ii.fIcon = FALSE; ii.hbmMask = mask; ii.hbmColor = dst; c = (HCURSOR)CreateIconIndirect(&ii);
            }
            if (dst) DeleteObject(dst); if (mask) DeleteObject(mask); if (a) DeleteDC(a); if (b) DeleteDC(b); ReleaseDC(NULL, sdc);
        }
        if (src) DeleteObject(src);
    }
    return c;
}
static int g_curFail;
static void ApplyCursor(void) {
    static const DWORD IDS[8] = { OCR_NORMAL, OCR_APPSTARTING, OCR_WAIT, OCR_HAND, OCR_IBEAM, OCR_CROSS, OCR_SIZEALL, OCR_NO };
    int i; g_curFail = 0;
    if (!g.curOn && !g.curHide) { SystemParametersInfoW(SPI_SETCURSORS, 0, NULL, SPIF_SENDCHANGE); return; }
    for (i = 0; i < 8; i++) {
        HCURSOR c = NULL;
        if (g.curHide) c = MakeArrow(32, K_, K_, TRUE);
        else { c = CursorFromFile(S_cur[i], g.curSize); if (!c && i == 0) c = MakeArrow(g.curSize, (COLORREF)g.curCol, (COLORREF)g.curBorder, FALSE); }
        if (c && !SetSystemCursor(c, IDS[i])) { g_curFail = 1; DestroyCursor(c); }
    }
    if (g_curFail) SetStatus(L"This Wine build refused SetSystemCursor - custom cursor not applied.");
}

/* ---------- keyboard assist: signals the Android side (needs a receiver in Winlator / a script) ---------- */
static int g_kbWas; static DWORD g_kbLastClick, g_kbSince;
static void KbdSignal(int show) {
    wchar_t p[MAX_PATH]; HANDLE f, ev; DWORD wr; char msg[32];
    wcscpy(p, g_dir); wcscat(p, show ? L"kbd.show" : L"kbd.hide");
    f = CreateFileW(p, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f != INVALID_HANDLE_VALUE) { _snprintf(msg, 31, "%s %lu\n", show ? "show" : "hide", (unsigned long)GetTickCount()); WriteFile(f, msg, (DWORD)strlen(msg), &wr, NULL); CloseHandle(f); }
    wcscpy(p, g_dir); wcscat(p, show ? L"kbd.hide" : L"kbd.show"); DeleteFileW(p);
    ev = CreateEventW(NULL, FALSE, FALSE, show ? L"MontyKbdShow" : L"MontyKbdHide"); if (ev) { SetEvent(ev); CloseHandle(ev); }
    if (S_kbCmd[0]) {
        wchar_t cmd[MAX_PATH + 16]; STARTUPINFOW si; PROCESS_INFORMATION pi; memset(&si, 0, sizeof si); si.cb = sizeof si; si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
        _snwprintf(cmd, MAX_PATH + 15, L"%ls %ls", S_kbCmd, show ? L"show" : L"hide"); cmd[MAX_PATH + 15] = 0;
        if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) { CloseHandle(pi.hThread); CloseHandle(pi.hProcess); }
    }
}
static void KbdTick(void) {
    HWND fg; GUITHREADINFO gi; DWORD tid; BOOL input = FALSE, now_in; DWORD now = GetTickCount(); wchar_t exe[40];
    if (!g.kbOn) { g_kbWas = 0; return; }
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) g_kbLastClick = now;
    fg = GetForegroundWindow(); if (!fg) return;
    tid = GetWindowThreadProcessId(fg, NULL); memset(&gi, 0, sizeof gi); gi.cbSize = sizeof gi;
    if (GetGUIThreadInfo(tid, &gi)) {
        input = (gi.hwndCaret != NULL);
        if (!input && gi.hwndFocus) {
            wchar_t cls[40]; GetClassNameW(gi.hwndFocus, cls, 40);
            input = !_wcsicmp(cls, L"Edit") || wcsstr(cls, L"RichEdit") || !_wcsicmp(cls, L"Scintilla") || !_wcsicmp(cls, L"TEdit") || !_wcsicmp(cls, L"TMemo");
        }
    }
    MWinExe(fg, exe, 40); if (ListHasExe(L_KBX, exe)) input = FALSE;
    now_in = input;
    if (now_in && !g_kbWas) {
        if (g_kbSince == 0) g_kbSince = now;
        if ((long)(now - g_kbSince) >= g.kbDelay && (!g.kbClick || (long)(now - g_kbLastClick) < 1500)) { KbdSignal(1); g_kbWas = 1; }
    } else if (!now_in) { g_kbSince = 0; if (g_kbWas) { KbdSignal(0); g_kbWas = 0; } }
}

/* ---------- Window Look Changer bridge (the v2 skin app) ---------- */
static void SkinBridge(void) {
    HWND w = FindWindowW(L"WLCMain", NULL);
    WritePrivateProfileStringW(L"cfg", L"theme", S_theme, g_ini);
    WrI(L"alpha", g.wlAlpha * 255 / 100); WrI(L"sys", g.wlAll); WrI(L"auto", g.wlOn);
    if (g.wlOn) {
        if (!w) {
            wchar_t p[MAX_PATH]; wcscpy(p, g_dir); wcscat(p, L"WindowLookChanger.exe");
            if (GetFileAttributesW(p) != INVALID_FILE_ATTRIBUTES) ShellExecuteW(NULL, L"open", p, L"/bg", g_dir, SW_HIDE);
        } else PostMessageW(w, WM_APP + 3, 0, 0);
    } else if (w) PostMessageW(w, WM_QUITAPP, 0, 0);
}
static void OpenSkinUi(void) {
    HWND w = FindWindowW(L"WLCMain", NULL);
    if (w) PostMessageW(w, WM_SHOWAPP, 0, 0);
    else { wchar_t p[MAX_PATH]; wcscpy(p, g_dir); wcscat(p, L"WindowLookChanger.exe"); ShellExecuteW(NULL, L"open", p, NULL, g_dir, SW_SHOWNORMAL); }
}

/* ---------- autostart ---------- */
static void SetAutostart(int on) {
    HKEY k; wchar_t me[MAX_PATH], cmd[MAX_PATH + 8];
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, KEY_WRITE, NULL, &k, NULL) != ERROR_SUCCESS) return;
    if (on) {
        GetModuleFileNameW(NULL, me, MAX_PATH); _snwprintf(cmd, MAX_PATH + 7, L"\"%ls\" /bg", me); cmd[MAX_PATH + 7] = 0;
        RegSetValueExW(k, L"MontyShell", 0, REG_SZ, (const BYTE *)cmd, (DWORD)((wcslen(cmd) + 1) * sizeof(wchar_t)));
    } else RegDeleteValueW(k, L"MontyShell");
    RegCloseKey(k);
}

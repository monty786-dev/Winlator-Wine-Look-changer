# Builds for 64-bit and 32-bit Wine with mingw-w64:
#   WindowLookChanger*.exe  (title-bar skinner)   MontyShell*.exe (taskbar/start/desktop/settings)   MontySetup*.exe (installer with both embedded)
CFLAGS  = -O2 -Wall -Wno-unused-function -Wno-unused-variable -Wno-misleading-indentation -Wno-unused-but-set-variable
LIBS    = -luser32 -lgdi32 -lcomdlg32 -lshell32 -ladvapi32 -lmsimg32 -lkernel32
LDFLAGS = -mwindows -static -static-libgcc -s
SKIN_H  = common.h theme.h img.h draw.h skin.h app.h ui1.h ui2.h
SHELL_H = common.h theme.h img.h draw.h mcfg.h mpal.h mtb1.h mtb2.h mst1.h mst2.h mdesk.h mmisc.h mset.h

all: MontySetup64.exe MontySetup32.exe

WindowLookChanger64.exe: wlc.c $(SKIN_H)
	x86_64-w64-mingw32-gcc $(CFLAGS) wlc.c -o $@ $(LDFLAGS) $(LIBS)
WindowLookChanger32.exe: wlc.c $(SKIN_H)
	i686-w64-mingw32-gcc $(CFLAGS) wlc.c -o $@ $(LDFLAGS) $(LIBS)
MontyShell64.exe: monty.c $(SHELL_H)
	x86_64-w64-mingw32-gcc $(CFLAGS) monty.c -o $@ $(LDFLAGS) $(LIBS)
MontyShell32.exe: monty.c $(SHELL_H)
	i686-w64-mingw32-gcc $(CFLAGS) monty.c -o $@ $(LDFLAGS) $(LIBS)

MontySetup64.exe: msetup.c MontyShell64.exe WindowLookChanger64.exe
	echo '1 RCDATA "MontyShell64.exe"' > res64.rc
	echo '2 RCDATA "WindowLookChanger64.exe"' >> res64.rc
	x86_64-w64-mingw32-windres res64.rc -O coff -o res64.o
	x86_64-w64-mingw32-gcc $(CFLAGS) msetup.c res64.o -o $@ $(LDFLAGS) -luser32 -lgdi32 -lshell32 -ladvapi32 -lkernel32
MontySetup32.exe: msetup.c MontyShell32.exe WindowLookChanger32.exe
	echo '1 RCDATA "MontyShell32.exe"' > res32.rc
	echo '2 RCDATA "WindowLookChanger32.exe"' >> res32.rc
	i686-w64-mingw32-windres res32.rc -O coff -o res32.o
	i686-w64-mingw32-gcc $(CFLAGS) msetup.c res32.o -o $@ $(LDFLAGS) -luser32 -lgdi32 -lshell32 -ladvapi32 -lkernel32

clean:
	rm -f *.exe *.o res*.rc

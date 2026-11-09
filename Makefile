# Builds the app and the single-file Setup for 64-bit and 32-bit Wine (mingw-w64).
CFLAGS  = -O2 -Wall -Wno-unused-function -Wno-unused-variable -Wno-misleading-indentation
LIBS    = -luser32 -lgdi32 -lcomdlg32 -lshell32 -ladvapi32 -lmsimg32 -lkernel32
LDFLAGS = -mwindows -static -static-libgcc -s
HDRS    = common.h theme.h img.h draw.h skin.h app.h ui1.h ui2.h

all: WindowLookChangerSetup64.exe WindowLookChangerSetup32.exe

WindowLookChanger64.exe: wlc.c $(HDRS)
	x86_64-w64-mingw32-gcc $(CFLAGS) wlc.c -o $@ $(LDFLAGS) $(LIBS)
WindowLookChanger32.exe: wlc.c $(HDRS)
	i686-w64-mingw32-gcc $(CFLAGS) wlc.c -o $@ $(LDFLAGS) $(LIBS)

# Setup embeds the matching app exe as resource #1 (always named WindowLookChanger.exe inside)
WindowLookChangerSetup64.exe: setup.c WindowLookChanger64.exe
	cp WindowLookChanger64.exe WindowLookChanger.exe
	echo '1 RCDATA "WindowLookChanger.exe"' > res64.rc
	x86_64-w64-mingw32-windres res64.rc -O coff -o res64.o
	x86_64-w64-mingw32-gcc $(CFLAGS) setup.c res64.o -o $@ $(LDFLAGS) -ladvapi32 -lshell32 -luser32 -lgdi32 -lkernel32
WindowLookChangerSetup32.exe: setup.c WindowLookChanger32.exe
	cp WindowLookChanger32.exe WindowLookChanger.exe
	echo '1 RCDATA "WindowLookChanger.exe"' > res32.rc
	i686-w64-mingw32-windres res32.rc -O coff -o res32.o
	i686-w64-mingw32-gcc $(CFLAGS) setup.c res32.o -o $@ $(LDFLAGS) -ladvapi32 -lshell32 -luser32 -lgdi32 -lkernel32

clean:
	rm -f *.exe *.o res*.rc

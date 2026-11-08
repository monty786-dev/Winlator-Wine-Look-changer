SRC    = wlc.c
CFLAGS = -O2 -Wall -Wextra -Wno-unused-parameter -Wno-misleading-indentation
LIBS   = -luser32 -lgdi32 -lcomdlg32 -lshell32 -lkernel32
LDFLAGS= -mwindows -static -static-libgcc -s

all: WindowLookChanger64.exe WindowLookChanger32.exe

WindowLookChanger64.exe: $(SRC)
	x86_64-w64-mingw32-gcc $(CFLAGS) $(SRC) -o $@ $(LDFLAGS) $(LIBS)

WindowLookChanger32.exe: $(SRC)
	i686-w64-mingw32-gcc $(CFLAGS) $(SRC) -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -f *.exe

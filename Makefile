CC=cl
LINK=link
TARGET=transmit.exe
OBJ=main.obj

# Debug and release folders
DBG=dbg
REL=rel

CFLAGS_COMMON=/Zi /nologo /W4 /WX
CFLAGS=$(CFLAGS_COMMON) /O2 /MD
CFLAGSD=$(CFLAGS_COMMON) /Od /MDd /RTC1 /RTCc

LFLAGS_COMMON=/nologo /debug /release mswsock.lib ws2_32.lib
LFLAGS=$(LFLAGS_COMMON)
LFLAGSD=$(LFLAGS_COMMON)

all: debug release

debug: $(DBG)\$(TARGET)
release: $(REL)\$(TARGET)

$(DBG)\$(TARGET): $(DBG)\$(OBJ)
	$(LINK) $(LFLAGSD) /out:$@ $**

$(REL)\$(TARGET): $(REL)\$(OBJ)
	$(LINK) $(LFLAGS) /out:$@ $**

{}.c{$(DBG)\}.obj:
	$(CC) /c $(CFLAGSD) $< /Fo$@

{}.c{$(REL)\}.obj:
	$(CC) /c $(CFLAGS) $< /Fo$@

clean:
	-del $(DBG)\*.obj $(REL)\*.obj
	-del $(DBG)\$(TARGET) $(REL)\$(TARGET)

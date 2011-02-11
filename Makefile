CC=cl
LINK=link
TARGET=transmit.exe
OBJ=main.obj

# Debug and release folders
DBG=dbg
REL=rel

CFLAGS_COMMON=/Zi
CFLAGS=$(CFLAGS_COMMON) /O2
CFLAGSD=$(CFLAGS_COMMON) /Od /RTC1 /RTCc

LFLAGS_COMMON=/debug /release
LFLAGS=$(LFLAGS_COMMON)
LFLAGSD=$(LFLAGS_COMMON)

all: debug release

debug: $(DBG)\$(TARGET)
release: $(REL)\$(TARGET)

$(DBG)\$(TARGET): $(DBG)\$(OBJ)
	$(LINK) $(LFLAGSD) /out:$@
$(REL)\$(TARGET): $(REL)\$(OBJ)
	$(LINK) $(LFLAGS) /out:$@

{}.c{$(DBG)\}.obj:
	$(CC) /c $(CFLAGSD) $<

{}.c{$(REL)\}.obj:
	$(CC) /c $(CFLAGS) $<

clean:
	-del *.obj $(TARGET)

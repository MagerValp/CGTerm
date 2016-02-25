PREFIX=/usr/local
EXESUFFIX=
SOCKETLIBS=

# for cygwin
#EXESUFFIX=.exe

# for solaris:
#SOCKETLIBS = -lsocket -lnsl

CC=gcc
CFLAGS=-O3 -Wall `sdl-config --cflags` -DPREFIX=\"$(PREFIX)\"
LDFLAGS=`sdl-config --libs` $(SOCKETLIBS)


OBJS= \
	kernal.o \
	gfx.o \
	net.o \
	config.o \
	keyboard.o \
	menu.o \
	font.o \
	timer.o \
	crc.o \
	sound.o \
	macro.o \
	ui.o

TERMOBJS= \
	xfer.o \
	xmodem.o \
	punter.o \
	diskimage.o \
	dir.o \
	fileselector.o \
	ui_term.o

CHATOBJS= \
	chat.o \
	status.o \
	ui_chat.o

EDITOBJS= \
	ui_edit.o \
	fileselector.o \
	dir.o \
	diskimage.o


%.o: %c
	$(CC) $(CFLAGS) -c $<


.PHONY: install installdirs clean


all: cgterm cgchat cgedit testkbd

cgterm: cgterm.o $(OBJS) $(TERMOBJS)
	$(CC) -o cgterm $^ $(LDFLAGS)

cgchat: cgchat.o $(OBJS) $(CHATOBJS)
	$(CC) -o cgchat $^ $(LDFLAGS)

cgedit: cgedit.o $(OBJS) $(EDITOBJS)
	$(CC) -o cgedit $^ $(LDFLAGS)

testkbd: testkbd.o
	$(CC) -o testkbd $^ $(LDFLAGS)

testimage: testimage.c diskimage.c dir.c
	$(CC) -g -O3 -Wall -o testimage $^

install: all installdirs
	strip cgterm$(EXESUFFIX)
	strip cgchat$(EXESUFFIX)
	strip cgedit$(EXESUFFIX)
	cp cgterm$(EXESUFFIX) $(PREFIX)/bin/
	cp cgchat$(EXESUFFIX) $(PREFIX)/bin/
	cp cgedit$(EXESUFFIX) $(PREFIX)/bin/
	cp *.bmp *.kbd *.wav $(PREFIX)/share/cgterm/

installdirs: $(PREFIX)/bin $(PREFIX)/share $(PREFIX)/share/cgterm $(PREFIX)/etc

$(PREFIX)/bin:
	mkdir $(PREFIX)/bin > /dev/null 2>&1
$(PREFIX)/share:
	mkdir $(PREFIX)/share > /dev/null 2>&1
$(PREFIX)/share/cgterm:
	mkdir $(PREFIX)/share/cgterm > /dev/null 2>&1
$(PREFIX)/etc:
	mkdir $(PREFIX)/etc > /dev/null 2>&1

clean:
	rm -f cgterm$(EXESUFFIX) cgchat$(EXESUFFIX) cgedit$(EXESUFFIX) testkbd$(EXESUFFIX) *.o *~

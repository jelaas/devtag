CC:=gcc
CFLAGS+=-Wall -Os -g
all:	devtag
devtag:		devtag.o jelopt.o libdevtag.a devtag-allinone.c devtag-allinone.h
	$(CC) $(LDFLAGS) -o devtag devtag.o jelopt.o libdevtag.a
libdevtag.a:	usb.o dev.o lookup.o
	ar cr libdevtag.a usb.o dev.o lookup.o
devtag-allinone.c:	usb.c dev.c lookup.c
	echo "#define DEVTAG_ALLINONE" > devtag-allinone.c
	cat usb.c dev.c lookup.c >> devtag-allinone.c
devtag-allinone.h:	devtag.h libdevtag.h
	cat devtag.h libdevtag.h > devtag-allinone.h
clean:	
	rm -f *.o devtag libdevtag.a devtag-allinone.c devtag-allinone.h
install-lib:	libdevtag.a
	mkdir -p $(DESTDIR)/usr/lib
	mkdir -p $(DESTDIR)/usr/include
	cp libdevtag.a $(DESTDIR)/usr/lib
	cp devtag.h $(DESTDIR)/usr/include
	cp libdevtag.h $(DESTDIR)/usr/include
install-bin:	devtag
	mkdir -p $(DESTDIR)/bin
	cp devtag $(DESTDIR)/bin
	ln -sf devtag $(DESTDIR)/bin/devtagns
install:	install-lib install-bin

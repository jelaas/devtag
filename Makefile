CC:=gcc
CFLAGS+=-Wall -Os -g
all:	devtag
devtag:		devtag.o jelopt.o libdevtag.a
	$(CC) $(LDFLAGS) -o devtag devtag.o jelopt.o libdevtag.a
libdevtag.a:	usb.o dev.o lookup.o
	ar cr libdevtag.a usb.o dev.o lookup.o
clean:	
	rm -f *.o devtag libdevtag.a
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

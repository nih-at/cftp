## Process this file with automake to create Makefile.in

AUTOMAKE_OPTIONS=	no-dependencies foreign

MAKEINFO = makeinfo --no-validate

LDADD=	@LIBOBJS@

bin_PROGRAMS=	cftp
info_TEXINFOS=	cftp.texi
cftp_TEXINFOS=	functions.texi
noinst_PROGRAMS=mkbind
cftp_SOURCES=	directory.c display.c readdir.c loop.c functions.c \
		ftp.c sockets.c main.c options.c keys.c tty.c rc.c \
		tag.c util.c version.c bindings.c fntable.c \
		fn_basic.c fn_scroll.c fn_select.c fn_options.c \
		fn_tag.c
noinst_HEADERS= directory.h display.h readdir.h loop.h functions.h tag.h\
		ftp.h sockets.h options.h keys.h tty.h rc.h fntable.h util.h
mkbind_SOURCES=	mkbind.c keys.c rc.c

cftp_DEPENDENCIES=    @LIBOBJS@
mkbind_DEPENDENCIES=    @LIBOBJS@

EXTRA_DIST=	bindings.desc fntable.c.proto fntable.h.proto \
		$(FUNCS) \
		mkfntexi.m4 mkfntab-c.m4 mkfntab-h.m4 endall.m4

FUNCS=	fn_basic.fn fn_scroll.fn fn_select.fn fn_tag.fn \
	fn_options.fn 

# extra rules

version.c: configure.in
	echo "char version[] = \"$(PACKAGE) (version $(VERSION))\";" \
		> version.c

functions.texi : $(FUNCS) mkfntexi.m4
	m4 mkfntexi.m4 $(FUNCS) endall.m4 >> functions.texi-unsort
	sed -e '/^>>>>>>>>/,$$ d' functions.texi-unsort > functions.texi
	sed -e '1,/^>>>>>>>>/ d' -e '/^<<<<<<<</,$$ d' functions.texi-unsort \
		| sort -t: +0 -1 >> functions.texi
	sed -e '1,/^<<<<<<<</ d' functions.texi-unsort >> functions.texi
	rm functions.texi-unsort

fntable.c : $(FUNCS) fntable.c.proto mkfntab-c.m4
	sed -e '/^----/,$$ d' fntable.c.proto > fntable.c
	m4 mkfntab-c.m4 $(FUNCS) endall.m4 >> fntable.c
	sed -e '1,/^----/ d' fntable.c.proto >> fntable.c

fntable.h : $(FUNCS) fntable.h.proto mkfntab-h.m4
	sed -e '/^----/,$$ d' fntable.h.proto > fntable.h
	m4 mkfntab-h.m4 $(FUNCS) endall.m4 >> fntable.h
	sed -e '1,/^----/ d' fntable.h.proto >> fntable.h

bindings.c : mkbind bindings.desc fntable.c
	./mkbind

getdomainname.o: getdomainname.c
	$(CC) $(CFLAGS) -DDOMAINNAME=\"@DOMAINNAME@\" -c getdomainname.c


# web rules

%.fn %.c %.h : %.cw 
	ctangle $<

#$(LIBSRCS): additional.cw
#	ctangle $<


# dependencies

main.o: main.c directory.h display.h loop.h ftp.h functions.h
directory.o: ftp.h
display.o: display.c directory.h ftp.h config.h tty.h keys.h
readdir.o: readdir.c directory.h ftp.h display.h
loop.o: loop.c directory.h display.h functions.h tty.h
functions.o: functions.c directory.h display.h tag.h options.h
ftp.o: ftp.c directory.h display.h readdir.h sockets.h util.h
fn_basic.o: fn_basic.c directory.h functions.h display.h ftp.h util.h
fn_scroll.o: fn_scroll.c directory.h functions.h display.h
fn_select.o: fn_select.c directory.h functions.h display.h ftp.h options.h \
	util.h
fn_options.o: fn_options.c directory.h functions.h display.h options.h
fn_tag.o: fn_tag.c directory.h functions.h display.h util.h tag.h options.h \
	ftp.h
fntable.o: fntable.c fntable.h
keys.o: keys.c keys.h
tty.o: tty.c tty.h keys.h
rc.o: rc.c rc.h

mkbind.o: mkbind.c keys.h rc.h

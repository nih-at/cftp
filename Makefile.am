## Process this file with automake to create Makefile.in

AUTOMAKE_OPTIONS=	foreign

MAKEINFO = makeinfo --no-validate

LDADD=	@LIBOBJS@

bin_PROGRAMS=	cftp
info_TEXINFOS=	cftp.texi
cftp_TEXINFOS=	functions.texi options.texi
noinst_PROGRAMS=mkbind
cftp_SOURCES=	directory.c display.c readdir.c loop.c functions.c \
		ftp.c sockets.c main.c options.c keys.c tty.c rc.c readrc.c \
		list.c tag.c util.c version.c bindings.c fntable.c \
		fn_basic.c fn_scroll.c fn_select.c fn_tag.c fn_bind.c
noinst_HEADERS= directory.h display.h readdir.h loop.h functions.h tag.h \
		ftp.h sockets.h options.h keys.h tty.h rc.h fntable.h util.h \
		getopt.h
mkbind_SOURCES=	mkbind.c keys.c rc.c fn_bind.c functions.c

cftp_DEPENDENCIES=    @LIBOBJS@
mkbind_DEPENDENCIES=    @LIBOBJS@

EXTRA_DIST=	bindings.desc fntable.c.proto fntable.h.proto \
		mkfntexi.m4 mkfntab-c.m4 mkfntab-h.m4 fntable.fn \
		mkoptexi.m4 mkoptab-c.m4 mkoptab-h.m4 options.op

# extra rules

version.c: configure.in
	echo "char version[] = \"$(PACKAGE) (version $(VERSION))\";" \
		> version.c

functions.texi : fntable.fn mkfntexi.m4
	m4 mkfntexi.m4 fntable.fn >> functions.texi-unsort
	sed -e '/^>>>>>>>>/,$$ d' functions.texi-unsort > functions.texi
	sed -e '1,/^>>>>>>>>/ d' -e '/^<<<<<<<</,$$ d' functions.texi-unsort \
		| sort -t: +0 -1 >> functions.texi
	sed -e '1,/^<<<<<<<</ d' functions.texi-unsort >> functions.texi
	rm functions.texi-unsort

fntable.c : fntable.fn fntable.c.proto mkfntab-c.m4
	sed -e '/^----/,$$ d' fntable.c.proto > fntable.c
	m4 mkfntab-c.m4 fntable.fn >> fntable.c
	sed -e '1,/^----/ d' fntable.c.proto >> fntable.c

fntable.h : fntable.fn fntable.h.proto mkfntab-h.m4
	sed -e '/^----/,$$ d' fntable.h.proto > fntable.h
	m4 mkfntab-h.m4 fntable.fn >> fntable.h
	sed -e '1,/^----/ d' fntable.h.proto >> fntable.h

fntable.o: fntable.c fntable.h

bindings.c : mkbind bindings.desc fntable.c
	./mkbind

options.texi: options.op mkoptexi.m4
	m4 mkoptexi.m4 options.op > options.texi

options.c: options.op mkoptab-c.m4
	m4 mkoptab-c.m4 options.op > options.c

options.h: options.op mkoptab-h.m4
	m4 mkoptab-h.m4 options.op > options.h

getdomainname.o: getdomainname.c
	$(CC) $(CFLAGS) -DDOMAINNAME=\"@DOMAINNAME@\" -c getdomainname.c

## Process this file with automake to create Makefile.in

MAKEINFO = makeinfo --no-validate

LDADD=	@LIBOBJS@

bin_PROGRAMS=	cftp
info_TEXINFOS=	cftp.texi
cftp_TEXINFOS=	functions.texi options.texi
noinst_PROGRAMS=mkbind
cftp_SOURCES=	directory.c display.c readdir.c loop.c functions.c \
		ftp.c sockets.c main.c options.c keys.c tty.c rc.c readrc.c \
		list.c tag.c util.c version.c bindings.c status.c signals.c \
		fntable.c fn_basic.c fn_scroll.c fn_select.c fn_tag.c fn_bind.c
noinst_HEADERS= directory.h display.h readdir.h loop.h functions.h tag.h \
		ftp.h sockets.h options.h keys.h tty.h rc.h fntable.h util.h \
		getopt.h list.h bindings.h status.h signals.h \
		fnmatch_repl.h
mkbind_SOURCES=	mkbind.c keys.c rc.c fn_bind.c functions.c

cftp_DEPENDENCIES=    @LIBOBJS@
mkbind_DEPENDENCIES=    @LIBOBJS@

EXTRA_DIST=	bindings.desc \
		mkfntexi.m4 mkfntab-c.m4 mkfntab-h.m4 fntable.fn \
		mkoptexi.m4 mkoptab-c.m4 mkoptab-h.m4 options.op

# extra rules

BUILT_SOURCES=	options.h options.c fntable.h fntable.c bindings.c

functions.texi : fntable.fn mkfntexi.m4
	m4 $(srcdir)/mkfntexi.m4 $(srcdir)/fntable.fn >> functions.texi-unsort
	sed -e '/^<-------/,$$ d' $(srcdir)/functions.texi-unsort \
		> functions.texi
	sed -e '1,/^<-------/ d' -e '/^------->/,$$ d' functions.texi-unsort \
		| sort -t: +0 -1 >> functions.texi
	sed -e '1,/^------->/ d' functions.texi-unsort >> functions.texi
	rm functions.texi-unsort

fntable.c : fntable.fn mkfntab-c.m4
	m4 $(srcdir)/mkfntab-c.m4 $(srcdir)/fntable.fn > fntable.c

fntable.h : fntable.fn mkfntab-h.m4
	m4 $(srcdir)/mkfntab-h.m4 $(srcdir)/fntable.fn > fntable.h

bindings.c : mkbind bindings.desc fntable.c
	./mkbind $(srcdir)

options.texi: options.op mkoptexi.m4
	m4 $(srcdir)/mkoptexi.m4 $(srcdir)/options.op > options.texi

options.c: options.op $(srcdir)/mkoptab-c.m4
	m4 $(srcdir)/mkoptab-c.m4 $(srcdir)/options.op > options.c

options.h: options.op mkoptab-h.m4
	m4 $(srcdir)/mkoptab-h.m4 $(srcdir)/options.op > options.h

getdomainname.o: getdomainname.c
	$(CC) $(CFLAGS) -DDOMAINNAME=\"@DOMAINNAME@\" -c getdomainname.c

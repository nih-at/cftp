## Process this file with automake to create Makefile.in

MAKEINFO = makeinfo --no-validate

LDADD=	@LIBOBJS@

bin_PROGRAMS=	cftp
info_TEXINFOS=	cftp.texi
cftp_TEXINFOS=	functions.texi options.texi
noinst_PROGRAMS=mkbind

cftp_SOURCES=	bindings.c directory.c display.c fn_basic.c fn_bind.c \
	fn_scroll.c fn_select.c fn_tag.c fntable.c ftp.c functions.c \
	keys.c list.c loop.c main.c options.c rc.c readdir.c readrc.c \
	signals.c sockets.c status.c tag.c tty.c url.c util.c version.c

mkbind_SOURCES=	fn_bind.c functions.c mkbind.c keys.c rc.c

noinst_HEADERS=	bindings.h directory.h display.h fnmatch_repl.h fntable.h \
	ftp.h functions.h getopt.h keys.h list.h loop.h options.h rc.h \
	readdir.h signals.h sockets.h status.h tag.h tty.h url.h util.h

cftp_DEPENDENCIES=    @LIBOBJS@
mkbind_DEPENDENCIES=    @LIBOBJS@

EXTRA_DIST=	bindings.desc \
		mkfntexi.m4 mkfntab-c.m4 mkfntab-h.m4 fntable.fn \
		mkoptexi.m4 mkoptab-c.m4 mkoptab-h.m4 options.op

BUILT_SOURCES=	bindings.c fntable.c fntable.h options.c options.h

# extra rules

bindings.c : mkbind bindings.desc fntable.c
	./mkbind $(srcdir)

fntable.c : fntable.fn mkfntab-c.m4
	m4 $(srcdir)/mkfntab-c.m4 $(srcdir)/fntable.fn > fntable.c

fntable.h : fntable.fn mkfntab-h.m4
	m4 $(srcdir)/mkfntab-h.m4 $(srcdir)/fntable.fn > fntable.h

functions.texi : fntable.fn mkfntexi.m4
	m4 $(srcdir)/mkfntexi.m4 $(srcdir)/fntable.fn >> functions.texi-unsort
	sed -e '/^<-------/,$$ d' $(srcdir)/functions.texi-unsort \
		> functions.texi
	sed -e '1,/^<-------/ d' -e '/^------->/,$$ d' functions.texi-unsort \
		| sort -t: +0 -1 >> functions.texi
	sed -e '1,/^------->/ d' functions.texi-unsort >> functions.texi
	rm functions.texi-unsort

options.c: options.op $(srcdir)/mkoptab-c.m4
	m4 $(srcdir)/mkoptab-c.m4 $(srcdir)/options.op > options.c

options.h: options.op mkoptab-h.m4
	m4 $(srcdir)/mkoptab-h.m4 $(srcdir)/options.op > options.h

options.texi: options.op mkoptexi.m4
	m4 $(srcdir)/mkoptexi.m4 $(srcdir)/options.op > options.texi

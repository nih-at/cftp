## Process this file with automake to create Makefile.in

AUTOMAKE_OPTIONS=	no-dependencies foreign

LDADD=	@LIBOBJS@

bin_PROGRAMS=	cftp
noinst_PROGRAMS=mkbind
cftp_SOURCES=	directory.c display.c readdir.c loop.c functions.c \
		ftp.c sockets.c main.c options.c keys.c tty.c rc.c \
		tag.c util.c version.c bindings.c fntable.c \
		fn_basic.c fn_prefix.c fn_scroll.c fn_select.c fn_options.c \
		fn_tag.c
noinst_HEADERS= directory.h display.h readdir.h loop.h functions.h tag.h\
		ftp.h sockets.h options.h keys.h tty.h rc.h fntable.h
mkbind_SOURCES=	mkbind.c keys.c rc.c

cftp_DEPENDENCIES=    @LIBOBJS@
mkbind_DEPENDENCIES=    @LIBOBJS@

EXTRA_DIST=	bindings.desc fntable.c.proto fntable.h.proto \
		$(FUNCS)

FUNCS=	fn_basic.fn fn_prefix.fn fn_scroll.fn fn_select.fn fn_options.fn \
	fn_tag.fn

# extra rules

version.c: configure.in
	echo "char version[] = \"$(PACKAGE) (version $(VERSION))\";" \
		> version.c

fntable.c : $(FUNCS) fntable.c.proto
	sed -e '/^----/,$$ d' fntable.c.proto > fntable.c
	sed -n -e '1,$$ s/^[ \t]*\({.*\)/  \1,/' \
		-e '1,$$ s-^; \(.*\)-/* \1 */-' \
		-e '/^  {/ p' -e '/\/\*/ p' $(FUNCS) >> fntable.c
	sed -e '1,/^----/ d' fntable.c.proto >> fntable.c

fntable.h : $(FUNCS) fntable.h.proto
	sed -e '/^----/,$$ d' fntable.h.proto > fntable.h
	sed -n \
	 -e '1,$$ s/^[ \t]*{[ \t]*\([a-zA-Z][a-zA-Z0-9_]*\).*/void \1(void);/' \
	 -e '1,$$ s-^; \(.*\)-/* \1 */-' \
	 -e '/^void / p' -e '/^\/\*/ p' $(FUNCS) >> fntable.h
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
fn_basic.o: fn_basic.c directory.h functions.h display.h
fn_prefix.o: fn_prefix.c directory.h functions.h
fn_scroll.o: fn_scroll.c directory.h functions.h display.h
fn_select.o: fn_select.c directory.h functions.h display.h ftp.h options.h \
	util.h
fn_options.o: fn_options.c directory.h functions.h display.h options.h
fn_tag.o: fn_tag.c directory.h functions.h display.h util.h tag.h options.h
fntable.o: fntable.c fntable.h
keys.o: keys.c keys.h
tty.o: tty.c tty.h keys.h
rc.o: rc.c rc.h

mkbind.o: mkbind.c keys.h rc.h

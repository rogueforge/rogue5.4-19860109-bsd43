#
# Makefile for rogue
# @(#)Makefile	X.XX (Berkeley) XX/XX/XX
#
HDRS=	rogue.h extern.h
DOBJS=	vers.o extern.o armor.o chase.o command.o daemon.o \
	daemons.o fight.o init.o io.o list.o main.o misc.o \
	monsters.o move.o new_level.o options.o pack.o passages.o potions.o \
	rings.o rip.o rooms.o save.o scrolls.o sticks.o things.o \
	weapons.o wizard.o
OBJS=	$(DOBJS) mach_dep.o
POBJS=	vers.po extern.po armor.po chase.po command.po daemon.po \
	daemons.po fight.po init.po io.po list.po main.po misc.po \
	monsters.po move.po new_level.po options.po pack.po passages.po \
	potions.po rings.po rip.po rooms.po save.po scrolls.po sticks.po \
	things.po weapons.po wizard.po mach_dep.po
CFILES=	vers.c extern.c armor.c chase.c command.c daemon.c \
	daemons.c fight.c init.c io.c list.c main.c misc.c \
	monsters.c move.c new_level.c options.c pack.c passages.c potions.c \
	rings.c rip.c rooms.c save.c scrolls.c sticks.c things.c \
	weapons.c wizard.c mach_dep.c
MISC=	Makefile

#DEFS=		-DMASTER -DDUMP -DALLSCORES
CFLAGS=		-O $(DEFS)
PROFLAGS=	-p -O
#LDFLAGS=-i	# For PDP-11's
LDFLAGS=	# For VAXen
CRLIB=		-lcurses
SCOREFILE=	/usr/games/lib/rogue_roll
SF=		-DSCOREFILE='"$(SCOREFILE)"'
NAMELIST=	/vmunix
NL=		-DNAMELIST='"$(NAMELIST)"'
#MACHDEP=	-DLOADAV -DMAXLOAD=40 -DCHECKTIME=4
BINARY=		distr.out

LD=	ld
VGRIND=	/usr/ucb/vgrind
# for sites without sccs front end, GET= get
GET=	sccs get
#GET-	get
CC=	cc

.SUFFIXES: .po

.DEFAULT:
	$(GET) $@

.c.po:
	@echo $(CC) -c $(PROFLAGS) $*.c
	@$(CC) -E $(PROFLAGS) $*.c | xstr -c -
	@$(CC) -c $(PROFLAGS) x.c
	@mv x.o $*.po

.c.o:
	@echo $(CC) -c $(CFLAGS) $*.c
	@$(CC) -E $(CFLAGS) $*.c | xstr -c -
	@$(CC) -c $(CFLAGS) x.c
	@mv x.o $*.o

a.out: $(HDRS) $(OBJS) xs.o
	-rm -f a.out
	@rm -f x.c
	$(CC) $(LDFLAGS) xs.o $(OBJS) $(CRLIB) -ltermlib
	size a.out
#	@echo -n 'You still have to remove '		# 11/70's
#	@size a.out | sed 's/+.*/ 1024 63 * - p/' | dc	# 11/70's

p.out:	$(HDRS) $(POBJS)
	@rm -f x.c
	$(CC) $(PROFLAGS) $(LDFLAGS) $(POBJS) $(PCRLIB) -ltermlib -o p.out
	size p.out

vers.o:
	$(CC) -c $(CFLAGS) vers.c

mach_dep.o: mach_dep.c
	$(CC) -c $(CFLAGS) $(SF) $(NL) $(MACHDEP) mach_dep.c

xs.o: strings
	xstr
	$(CC) -c $(CFLAGS) xs.c

rogue: newvers a.out
	cp a.out rogue
	strip rogue


install: rogue
	cp rogue         $(DESTDIR)/usr/games/rogue
	chown games      $(DESTDIR)/usr/games/rogue
	chgrp bin        $(DESTDIR)/usr/games/rogue
	chmod 4755       $(DESTDIR)/usr/games/rogue
	cat /dev/null >> $(DESTDIR)/usr/games/lib/rogue_roll
	chown games      $(DESTDIR)/usr/games/lib/rogue_roll
	chgrp bin        $(DESTDIR)/usr/games/lib/rogue_roll
	chmod 644        $(DESTDIR)/usr/games/lib/rogue_roll

distr: rmmach distmod.o mach_dep.o
	@rm -f x.c
	$(CC) -s $(LDFLAGS) -o $(BINARY) distmod.o mach_dep.o $(CRLIB) -ltermlib
	size $(BINARY)

rmmach:
	rm -f mach_dep.o

distmod.o: $(DOBJS) xs.o
	@rm -f x.c
	$(LD) -r -o distmod.o xs.o $(DOBJS)

main.obj: distmod.o
	cp distmod.o main.obj

newvers:
	$(GET) -e vers.c
	sccs delta -y vers.c

p.out:	$(HDRS) $(POBJS) xs.po
	@rm -f x.c
	$(CC) $(PROFLAGS) $(LDFLAGS) -o p.out xs.po $(POBJS) $(CRLIB) -ltermlib
	size p.out

vers.po:
	@echo $(CC) -c $(PROFLAGS) vers.c
	@ln vers.c x.c
	@$(CC) -c $(PROFLAGS) $(SF) x.c
	@mv x.o vers.po
	@rm -f x.c

mach_dep.po:
	$(CC) -c $(PROFLAGS) $(SF) $(NL) $(MACHDEP) mach_dep.c
	@mv mach_dep.o mach_dep.po

xs.po: strings
	xstr
	@echo $(CC) -c $(PROFLAGS) xs.c
	@mv xs.c x.c
	@$(CC) -c $(PROFLAGS) x.c
	@mv x.o xs.po

tags: $(HDRS) $(CFILES)
	ctags -u $?
	ed - tags < :ctfix
	sort tags -o tags

lint:
	lint -hxbc $(DEFS) $(MACHDEP) $(SF) $(NL) $(CFILES) -lcurses > linterrs

clean:
	rm -f $(POBJS) $(OBJS) core a.out p.out $(BINARY) rogue strings rogue.tar vgrind.* x.c x.o xs.c xs.o linterrs distmod.o main.obj xs.po

xtar: $(HDRS) $(CFILES) $(MISC)
	rm -f rogue.tar
	tar cf rogue.tar $? :ctfix
	touch xtar

vgrind:
	@csh $(VGRIND) -t -h "Rogue Version 5.4" $(HDRS) *.c > vgrind.out
	@ctags -v $(HDRS) *.c > index
	@csh $(VGRIND) -t -x index > vgrind.out.tbl

wc:
	@echo "  bytes  words  lines  pages file"
	@wc -cwlp $(HDRS) $(CFILES)

cfiles: $(CFILES)

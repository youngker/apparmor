# 	$Id$
# ----------------------------------------------------------------------
#    Copyright (c) 1999, 2000, 2001, 2002, 2004, 2005, 2006, 2007
#    NOVELL (All rights reserved)
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, contact Novell, Inc.
# ----------------------------------------------------------------------
NAME=apparmor-parser
all:
COMMONDIR=../common/

include common/Make.rules

COMMONDIR_EXISTS=$(strip $(shell [ -d ${COMMONDIR} ] && echo true))
ifeq ($(COMMONDIR_EXISTS), true)
common/Make.rules: $(COMMONDIR)/Make.rules
	ln -sf $(COMMONDIR) .
endif

DESTDIR=/
APPARMOR_BIN_PREFIX=${DESTDIR}/lib/apparmor
CONFDIR=/etc/apparmor
INSTALL_CONFDIR=${DESTDIR}${CONFDIR}
LOCALEDIR=/usr/share/locale
MANPAGES=apparmor.d.5 apparmor.7 apparmor_parser.8 subdomain.conf.5 apparmor.vim.5

YACC	:= /usr/bin/bison
YFLAGS	:= -d
LEX	:= /usr/bin/flex
LEXFLAGS = -B -v
WARNINGS = -Wall -Wstrict-prototypes
EXTRA_WARNINGS = -Wsign-compare -Wmissing-field-initializers -Wnested-externs -Wformat-security -Wunused-parameter
WARNINGS += $(shell for warning in ${EXTRA_WARNINGS} ; do \
			if ${CC} $${warning} -S -o /dev/null -xc /dev/null >/dev/null 2>&1; then \
				echo "$${warning}"; \
			fi ; \
		done)
CFLAGS	= -O2 -pipe

ifdef DEBUG
CFLAGS = -g
endif
EXTRA_CFLAGS = ${CFLAGS} ${WARNINGS}

#LEXLIB	:= -lfl

# override this on the make command to point to where the immunix.h file is
# (yeah this is lame, but since we are tied to the kernel so tightly...)
#INCLUDEDIR = /usr/src/linux/include
INCLUDEDIR =

ifdef INCLUDEDIR
	CFLAGS += -I$(INCLUDEDIR)
endif

# Internationalization support. Define a package and a LOCALEDIR
EXTRA_CFLAGS+=-DPACKAGE=\"${NAME}\" -DLOCALEDIR=\"${LOCALEDIR}\"

# Compile-time configuration of the location of the config file
EXTRA_CFLAGS+=-DSUBDOMAIN_CONFDIR=\"${CONFDIR}\"

SRCS = parser_include.c parser_interface.c parser_lex.c parser_main.c \
       parser_misc.c parser_merge.c parser_symtab.c parser_yacc.c \
       parser_regex.c parser_variable.c parser_policy.c
HDRS = parser.h parser_include.h immunix.h
TOOLS = apparmor_parser

OBJECTS = parser_lex.o parser_yacc.o parser_main.o parser_interface.o \
	  parser_include.o parser_merge.o parser_symtab.o parser_misc.o \
	  parser_regex.o parser_variable.o parser_policy.o

AAREDIR= libapparmor_re
AAREOBJECTS = ${AAREDIR}/libapparmor_re.a

PCREDIR= pcre
PCREOBJECTS = ${PCREDIR}/pcre.o

LEX_C_FILES	= parser_lex.c
YACC_C_FILES	= parser_yacc.c parser_yacc.h

TESTS = tst_regex tst_misc tst_symtab tst_variable
TEST_FLAGS = -Wl,--warn-unresolved-symbols
DISABLED_TESTS =

TEST_OBJECTS = $(filter-out parser_lex.o, \
	       $(filter-out parser_yacc.o, \
	       $(filter-out parser_main.o, ${OBJECTS} ${PCREOBJECTS})))

ifndef VERBOSE
  VERBOSE = 0
endif
ifeq ($(VERBOSE),1)
  BUILD_OUTPUT =
  Q =
else
  BUILD_OUTPUT = > /dev/null 2>&1
  Q = @
endif
export Q VERBOSE BUILD_OUTPUT

po/${NAME}.pot: ${SRCS} ${HDRS}
	make -C po ${NAME}.pot NAME=${NAME} SOURCES="${SRCS} ${HDRS}"

techdoc.pdf: techdoc.tex
	while pdflatex $< ${BUILD_OUTPUT} || exit 1 ; \
		grep -q "Label(s) may have changed" techdoc.log; \
	do :; done

techdoc/index.html: techdoc.pdf
	latex2html -show_section_numbers -split 0 -noinfo -nonavigation -noaddress techdoc.tex ${BUILD_OUTPUT}

techdoc.txt: techdoc/index.html
	w3m -dump $< > $@

all: 	$(TOOLS) $(MANPAGES) ${HTMLMANPAGES} techdoc.pdf techdoc/index.html
	$(Q)make -C po all
	$(Q)make -s tests

apparmor_parser: $(OBJECTS) $(PCREOBJECTS) $(AAREOBJECTS)
	rm -f ./libstdc++.a
	ln -s `g++ -print-file-name=libstdc++.a`
	g++ $(EXTRA_CFLAGS) -o $@ $(OBJECTS) $(PCREOBJECTS) $(LIBS) \
	      ${LEXLIB}  $(AAREOBJECTS) -static-libgcc -L.

parser_yacc.c parser_yacc.h: parser_yacc.y parser.h
	$(YACC) $(YFLAGS) -o parser_yacc.c parser_yacc.y 

parser_lex.c: parser_lex.l parser_yacc.h parser.h
	$(LEX) ${LEXFLAGS} -o$@ $<

parser_lex.o: parser_lex.c parser.h parser_yacc.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_misc.o: parser_misc.c parser.h parser_yacc.h af_names.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_yacc.o: parser_yacc.c parser_yacc.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_main.o: parser_main.c parser.h parser_version.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_interface.o: parser_interface.c parser.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_include.o: parser_include.c parser.h parser_include.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_merge.o: parser_merge.c parser.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_regex.o: parser_regex.c parser.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_symtab.o: parser_symtab.c parser.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_variable.o: parser_variable.c parser.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_policy.o: parser_policy.c parser.h parser_yacc.h
	$(CC) $(EXTRA_CFLAGS) -c -o $@ $<

parser_version.h: Makefile
	@echo \#define PARSER_VERSION \"$(VERSION)\" > .ver
	@mv -f .ver $@

af_names.h: /usr/include/linux/socket.h
	LC_ALL=C sed -n -e "/AF_RXRPC/d" -n -e "/AF_MAX/d" -n -e "/AF_UNSPEC/d" -n -e "/AF_UNIX/d" -n -e "/AF_LOCAL/d" -n -e "/AF_NETLINK/d" -n -e "/AF_LLC/d" -n -e "/AF_IUCV/d" -n -e "/AF_TIPC/d" -e "s/^\#define[ \\t]\\+AF_\\([A-Z0-9_]\\+\\)[ \\t]\\+\\([0-9]\\+\\)\\(.*\\)\$$/AA_GEN_NET_ENT(\"\\L\\1\", \\UAF_\\1)/p" $< > $@



tst_symtab: parser_symtab.c parser.h parser_variable.o
	$(Q)$(CC) -DUNIT_TEST $(EXTRA_CFLAGS) $(TEST_FLAGS) -o $@ $< parser_variable.o $(BUILD_OUTPUT)

tst_variable: parser_variable.c parser.h $(filter-out parser_variable.o, ${TEST_OBJECTS})
	$(Q)$(CC) -DUNIT_TEST $(EXTRA_CFLAGS) $(TEST_FLAGS) -o $@ $< $(filter-out parser_variable.o, ${TEST_OBJECTS}) $(BUILD_OUTPUT)

tst_misc: parser_misc.c parser.h parser_yacc.h
	$(Q)$(CC) -DUNIT_TEST $(EXTRA_CFLAGS) $(TEST_FLAGS) -o $@ $< $(BUILD_OUTPUT)

tst_regex: parser_regex.c parser.h parser_yacc.h $(PCREOBJECTS)
	$(Q)$(CC) -DUNIT_TEST $(EXTRA_CFLAGS) $(TEST_FLAGS) -o $@ $< $(PCREOBJECTS) $(BUILD_OUTPUT)

.SILENT: check
.PHONY: check
check: tests

.SILENT: tests
tests: ${TESTS}
	for test in ${TESTS} ; do echo "*** running $${test}" && ./$${test} $(BUILD_OUTPUT) ; done
	$(Q)make -s -C tst tests

# always need to rebuild.
.SILENT: $(AAREOBJECTS)
.PHONY: $(AAREOBJECTS)
$(AAREOBJECTS):
	make -C $(AAREDIR)

.SILENT: $(PCREOBJECTS)
.PHONY: $(PCREOBJECTS)
$(PCREOBJECTS):
	make -C $(PCREDIR) "CFLAGS=$(EXTRA_CFLAGS)"

.PHONY: install-rhel4
install-rhel4: install-redhat

.PHONY: install-redhat
install-redhat:
	install -m 755 -d $(DESTDIR)/etc/init.d
	install -m 755 rc.apparmor.$(subst install-,,$@) $(DESTDIR)/etc/init.d/apparmor
	install -m 755 rc.aaeventd.redhat $(DESTDIR)/etc/init.d/aaeventd

.PHONY: install-suse
install-suse:
	install -m 755 -d $(DESTDIR)/etc/init.d
	install -m 755 rc.apparmor.$(subst install-,,$(@)) $(DESTDIR)/etc/init.d/boot.apparmor
	install -m 755 rc.aaeventd.$(subst install-,,$(@)) $(DESTDIR)/etc/init.d/aaeventd
	install -m 755 -d $(DESTDIR)/sbin
	ln -sf /etc/init.d/boot.apparmor $(DESTDIR)/sbin/rcapparmor
	ln -sf rcapparmor $(DESTDIR)/sbin/rcsubdomain
	ln -sf /etc/init.d/aaeventd $(DESTDIR)/sbin/rcaaeventd

.PHONY: install-slackware
install-slackware:
	install -m 755 -d $(APPARMOR_BIN_PREFIX)/install
	install -m 755 frob_slack_rc $(APPARMOR_BIN_PREFIX)/install
	install -m 755 -d $(DESTDIR)/etc/rc.d
	install -m 755 rc.apparmor.$(subst install-,,$(@)) $(DESTDIR)/etc/rc.d/rc.apparmor

.PHONY: install-debian
install-debian:

INSTALLDEPS=$(TOOLS)
ifdef DISTRO
INSTALLDEPS+=install-$(DISTRO)
endif

.PHONY: install
install: $(INSTALLDEPS)
	install -m 755 -d $(DESTDIR)/sbin
	install -m 755 ${TOOLS} $(DESTDIR)/sbin
	install -m 755 -d $(INSTALL_CONFDIR)
	install -m 644 subdomain.conf $(INSTALL_CONFDIR)
	install -m 755 -d ${DESTDIR}/var/lib/apparmor
	install -m 755 -d $(APPARMOR_BIN_PREFIX)
	install -m 755 rc.apparmor.functions $(APPARMOR_BIN_PREFIX)
	make -C po install NAME=${NAME} DESTDIR=${DESTDIR}
	make install_manpages DESTDIR=${DESTDIR}

.SILENT: clean
.PHONY: clean
clean: _clean
	rm -f core core.* *.o *.s *.a *~
	rm -f $(TOOLS) $(TESTS)
	rm -f $(LEX_C_FILES)
	rm -f $(YACC_C_FILES)
	rm -f parser_version.h
	rm -f $(NAME)*.tar.gz $(NAME)*.tgz
	rm -f libstdc++.a
	rm -f af_names.h
	-rm -rf techdoc.{aux,log,pdf,toc,txt} techdoc/
	make -s -C $(PCREDIR) clean
	make -s -C $(AAREDIR) clean
	make -s -C po clean

.SILENT: dist_clean
dist_clean:
	@make clean
	@rm -f $(LEX_C_FILES) $(YACC_C_FILES)

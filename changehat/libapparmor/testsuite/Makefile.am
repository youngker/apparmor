SUBDIRS = lib config libaalogparse.test
PACKAGE = libaalogparse
AUTOMAKE_OPTIONS = dejagnu

INCLUDES = -I. -I$(top_srcdir)/src

AM_CPPFLAGS = $(DEBUG_FLAGS) -DLOCALEDIR=\"${localedir}\"
AM_CFLAGS = -Wall

noinst_PROGRAMS = test_multi.multi

test_multi_multi_SOURCES	= test_multi.c
test_multi_multi_CFLAGS		= $(CFLAGS) -Wall
test_multi_multi_LDFLAGS	= $(LDFLAGS)
test_multi_multi_LDADD		= ../src/.libs/libapparmor.a

clean-local:
	rm -f tmp.err.* tmp.out.* site.exp site.bak

EXTRA_DIST = test_multi/*.in test_multi/*.out test_multi/*.err

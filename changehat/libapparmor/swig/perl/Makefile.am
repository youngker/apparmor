if HAVE_PERL

PERL_MAKEFILE = Makefile.perl

WRAPPER_SOURCES = libapparmor_wrap.c LibAppArmor.pm

all-local: .build-stamp

.build-stamp: $(WRAPPER_SOURCES) $(PERL_MAKEFILE)
	make -f $(PERL_MAKEFILE)
	touch .build-stamp

check-local: .build-stamp
	make -f $(PERL_MAKEFILE) test

install-exec-local: .build-stamp
	make -f $(PERL_MAKEFILE) install_vendor

clean-local: $(PERL_MAKEFILE)
	make -f $(PERL_MAKEFILE) clean
	rm -f $(PERL_MAKEFILE).old
	rm -rf build

$(PERL_MAKEFILE): Makefile.PL
	$(PERL) Makefile.PL VERSION="0.1" OBJECT="../../src/.libs/libapparmor.so libapparmor_wrap.o" CCFLAGS="-I../../src -D_REENTRANT -D_GNU_SOURCE -DTHREADS_HAVE_PIDS -fno-strict-aliasing -pipe -Wdeclaration-after-statement" OPTIMIZE="$(CFLAGS) -shared -I$(includedir) -D_REENTRANT -D_GNU_SOURCE -DTHREADS_HAVE_PIDS -fno-strict-aliasing -pipe -Wdeclaration-after-statement"


$(WRAPPER_SOURCES): ../SWIG/*.i
	$(SWIG) -perl -I../../src -I../SWIG -o libapparmor_wrap.c libapparmor.i

endif

EXTRA_DIST = Makefile.PL $(WRAPPER_SOURCES) examples/*.pl


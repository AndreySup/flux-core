
AM_CPPFLAGS = \
	@GCCWARN@ \
	-I$(top_srcdir) -I$(top_srcdir)/src/include \
	-I$(top_srcdir)/src/common/libflux \
	$(JSON_CFLAGS) $(ZMQ_CFLAGS) $(PYTHON_CPPFLAGS) \
	$(CODE_COVERAGE_CFLAGS)

AM_LDFLAGS = \
	-avoid-version -module $(san_ld_zdef_flag) \
	$(PYTHON_LDFLAGS) -Wl,-rpath,$(PYTHON_PREFIX)/lib \
	$(CODE_COVERAGE_LDFLAGS)

MAKE_BINDING=$(top_srcdir)/src/bindings/python/make_binding.py
SUFFIXES = _build.py

common_libs = $(top_builddir)/src/common/libflux-core.la \
	      $(top_builddir)/src/common/libflux-internal.la \
	      $(JSON_LIBS) \
	      $(ZMQ_LIBS)

_build.py.c:
	$(PYTHON) $*_build.py


_core_build.py: $(MAKE_BINDING)
	$(PYTHON) $(MAKE_BINDING) --path $(top_srcdir)/src/common/libflux \
	  			  --package flux \
	                          --modname _core \
				  --add_sub '.*va_list.*|||' \
				  --ignore_header 'handle_impl' \
				  --ignore_header 'macros' \
				  --add_long_sub 'FLUX_SEC_TYPE_ALL.*\n.*\),|||FLUX_SEC_TYPE_ALL = 7,'\
				  flux.h

fluxpy_LTLIBRARIES = _core.la _barrier.la _kvs.la _jsc.la _kz.la

_core_la_SOURCES = _core.c
_core_la_LIBADD = $(common_libs)
_core_la_DEPENDENCIES = _core_build.py

_jsc_build.py: $(top_srcdir)/src/modules/libjsc/jstatctl.h $(MAKE_BINDING) _core_build.py
	$(PYTHON) $(MAKE_BINDING) --path $(top_srcdir)/src/modules/libjsc \
	  			  --package flux \
	                          --modname _jsc \
				  --include_ffi _core_build \
				  jstatctl.h

_jsc_la_SOURCES = _jsc.c
_jsc_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/modules/libjsc
_jsc_la_LIBADD = $(top_builddir)/src/modules/libjsc/libflux-jsc.la $(common_libs)
_jsc_la_DEPENDENCIES = _jsc_build.py

_barrier_build.py: $(top_srcdir)/src/modules/barrier/barrier.h $(MAKE_BINDING) _core_build.py
	$(PYTHON) $(MAKE_BINDING) --path $(top_srcdir)/src/modules/barrier \
	  			  --package flux \
	                          --modname _barrier \
				  --include_ffi _core_build \
				  barrier.h

_barrier_la_SOURCES = _barrier.c
_barrier_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/modules/barrier
_barrier_la_LIBADD = $(top_builddir)/src/modules/barrier/libflux-barrier.la $(common_libs)
_barrier_la_DEPENDENCIES = _barrier_build.py


_kvs_build.py: $(top_srcdir)/src/modules/kvs/kvs.h $(MAKE_BINDING) _core_build.py
	$(PYTHON) $(MAKE_BINDING) --path $(top_srcdir)/src/modules/kvs \
	  			  --package flux \
	                          --modname _kvs \
				  --include_ffi _core_build \
				  kvs.h

_kvs_la_SOURCES = _kvs.c
_kvs_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/modules/kvs
_kvs_la_LIBADD = $(top_builddir)/src/modules/kvs/libflux-kvs.la $(common_libs)
_kvs_la_DEPENDENCIES = _kvs_build.py

_kz_build.py: $(top_srcdir)/src/modules/libkz/kz.h $(MAKE_BINDING) _core_build.py
	$(PYTHON) $(MAKE_BINDING) --path $(top_srcdir)/src/modules/libkz \
				  --package flux \
	                          --modname _kz \
				  --include_ffi _core_build \
				  --add_sub '.*KZ_FLAGS_DELAYCOMMIT.*,|||KZ_FLAGS_DELAYCOMMIT = 0x0c00' \
				  kz.h

_kz_la_SOURCES = _kz.c $(top_srcdir)/src/modules/libkz/kz.h
_kz_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/modules/libkz
_kz_la_LIBADD = $(top_builddir)/src/modules/libkz/libkz.la $(top_builddir)/src/modules/kvs/libflux-kvs.la  $(common_libs)
_kz_la_DEPENDENCIES = _kz_build.py

fluxpy_PYTHON=\
	      __init__.py\
	      kvs.py\
	      wrapper.py\
	      rpc.py\
	      message.py\
	      constants.py\
	      jsc.py\
	      command_helpers.py\
	      sec.py

.PHONY: lib-copy clean-local-check

lib-copy: ${fluxpy_PYTHON} ${fluxpy_LTLIBRARIES}
	-echo Copying libraries to where they can be used by python in-tree
	for LIB in ${fluxpy_LTLIBRARIES:la=so} ; do \
	    cp -f .libs/$$LIB ./ ; \
	done
	[ "$(top_srcdir)" != "$(top_builddir)" ] && cp $(top_srcdir)/src/bindings/python/flux/*.py ./ || true

#TODO: there must be a better way to do this
check-local: lib-copy
all-local: lib-copy


clean-local: clean-local-check
	-rm -f *.c *_build.py *.so *.pyc *.pyo

clean-local-check:
	[ "$(top_srcdir)" != "$(top_builddir)" ] && rm -f $(top_builddir)/src/bindings/python/flux/*.py || true

SUBDIRS = core

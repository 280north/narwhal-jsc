
include config.mk

CFLAGS+= -Iinclude
LFLAGS+= -Llib

NARWHAL_CFLAGS=$(JSCORE_CFLAGS)
NARWHAL_LFLAGS=$(JSCORE_LFLAGS) -lnarwhal

MODULES=$(patsubst %.cc,%.$(SO_EXT),$(patsubst src/%,lib/%,$(shell find src -name '*.cc')))
MODULE_FLAGS=$(CFLAGS) $(SO_CFLAGS) $(NARWHAL_CFLAGS) $(LFLAGS) $(SO_LFLAGS) $(NARWHAL_LFLAGS)

all: jscore webkit

jscore: executables/narwhal-jscore modules config-jscore
webkit: executables/narwhal-webkit modules config-webkit

# CONFIGURE:

config.mk:
	sh configure

config-jscore:
	echo 'export NARWHAL_JSC_MODE="narwhal-jscore"' > narwhal-jsc.conf
config-webkit:
	echo 'export NARWHAL_JSC_MODE="narwhal-webkit"' > narwhal-jsc.conf

# LIBNARWHAL:

lib/libnarwhal.$(SO_EXT): narwhal.c
	$(CC) -o $@ $< \
		$(CFLAGS) $(SO_CFLAGS) $(JSCORE_CFLAGS) \
		$(LFLAGS) $(SO_LFLAGS) $(JSCORE_LFLAGS)

# EXECUTABLES:

executables/narwhal-jscore: narwhal-jscore.c lib/libnarwhal.$(SO_EXT)
	mkdir -p `dirname $@`
	$(CC) -o $@ $< \
		$(CFLAGS) $(NARWHAL_CFLAGS) $(LFLAGS) $(NARWHAL_LFLAGS)

executables/narwhal-webkit-cocoa: narwhal-webkit-cocoa.m lib/libnarwhal.$(SO_EXT)
	mkdir -p `dirname $@`
	$(CC) -o $@ $< \
		$(CFLAGS) $(NARWHAL_CFLAGS) $(LFLAGS) $(NARWHAL_LFLAGS) \
		 -framework WebKit -framework Foundation -framework AppKit

executables/narwhal-webkit-gtk: narwhal-webkit-gtk.c lib/libnarwhal.$(SO_EXT)
	mkdir -p `dirname $@`
	$(CC) -o $@ $< \
		$(CFLAGS) $(NARWHAL_CFLAGS) $(LFLAGS) $(NARWHAL_LFLAGS)

executables/narwhal-webkit: executables/narwhal-webkit-$(PLATFORM)

# MODULES:

modules: $(MODULES)

lib/%.$(SO_EXT): src/%.cc
	mkdir -p `dirname $@`
	$(CPP) -o $@ $< $(MODULE_FLAGS)

lib/readline.$(SO_EXT): src/readline.cc
	mkdir -p `dirname $@`
	$(CPP) -o $@ $< $(MODULE_FLAGS) $(READLINE_CFLAGS) $(READLINE_LFLAGS)

lib/binary-engine.$(SO_EXT): src/binary-engine.cc
	mkdir -p `dirname $@`
	$(CPP) -o $@ $< $(MODULE_FLAGS) $(ICONV_CFLAGS) $(ICONV_LFLAGS)

lib/io-engine.$(SO_EXT): src/io-engine.cc
	mkdir -p `dirname $@`
	$(CPP) -o $@ $< $(MODULE_FLAGS) $(ICONV_CFLAGS) $(ICONV_LFLAGS)

lib/jack/handler/jill.$(SO_EXT): src/jack/handler/jill.cc deps/http-parser/http_parser.o lib/io-engine.$(SO_EXT) lib/binary-engine.$(SO_EXT)
	mkdir -p `dirname $@`
	$(CPP) -o $@ $< $(MODULE_FLAGS) deps/http-parser/http_parser.o lib/io-engine.$(SO_EXT) lib/binary-engine.$(SO_EXT)

# DEPENDENCIES:

deps/http-parser/http_parser.o:
	cd deps/http-parser && make http_parser.o

# CLEAN:

clean:
	rm -rf executables
	find lib \( -name "*.so" -or -name "*.dylib" \) -print -delete
	cd deps/http-parser && make clean

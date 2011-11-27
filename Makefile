
include config.mk

LIB_DIR   =lib

CFLAGS   +=-Iinclude
MODULES   =$(patsubst %.cc,%.$(SO_EXT),$(patsubst src/%,lib/%,$(shell find src -name '*.cc')))
LFLAGS   += -liconv -L$(LIB_DIR) -lnarwhal  

SOURCE    =narwhal-jsc.c

JSCORE_CONFIG=Production
JSCORE_FRAMEWORK=$(FRAMEWORKS_DIR)/JavaScriptCore.framework
JSCORE_BUILD=deps/JavaScriptCore/build/$(JSCORE_CONFIG)/JavaScriptCore.framework
JSCORE_CHECKOUT=deps/JavaScriptCore

SYSTEM_JSC=/System/Library/Frameworks/JavaScriptCore.framework/Versions/A/JavaScriptCore
RELATIVE_JSC=@executable_path/../frameworks/JavaScriptCore.framework/JavaScriptCore

ABSOLUTE_LIBNARWHAL=$(LIB_DIR)/libnarwhal.$(SO_EXT)
RELATIVE_LIBNARWHAL=@executable_path/../lib/libnarwhal.$(SO_EXT)

JSCOCOA_FRAMEWORK=$(FRAMEWORKS_DIR)/JSCocoa.framework
JSCOCOA_BUILD=deps/JSCocoa/JSCocoa/build/Release/JSCocoa.framework
JSCOCOA_CHECKOUT=deps/JSCocoa

all: webkit webkit-debug jscore

config:
	sh configure

jscore:	      config bin/narwhal-jscore modules config-jscore
webkit:	      config bin/narwhal-webkit modules config-webkit
webkit-debug: config bin/narwhal-webkit-debug modules config-webkit-debug
jscocoa:      config frameworks-jscocoa bin/narwhal-jscocoa modules config-jscocoa


lib/libnarwhal.$(SO_EXT): narwhal.c
	$(CC) -o $@ $< $(SO_CFLAGS) $(SO_LFLAGS) $(CFLAGS) $(JSCORE_CFLAGS) $(JSCORE_LFLAGS)

bin/narwhal-jscore: $(SOURCE) lib/libnarwhal.$(SO_EXT)
	mkdir -p `dirname $@`
	$(CC) -o $@ $(SOURCE) $(CFLAGS) $(LFLAGS) $(JSCORE_CFLAGS) $(JSCORE_LFLAGS)

bin/narwhal-webkit: $(SOURCE) lib/libnarwhal.$(SO_EXT)
	mkdir -p `dirname $@`
	#$(CC) -o $@ -x objective-c $(SOURCE) $(CFLAGS) $(INCLUDES) $(LIBS) $(LIBS_WEBKIT) 
	$(CC) -o $@ $(CFLAGS) $(WEBKIT_CFLAGS) $(SOURCE) $(LFLAGS) $(WEBKIT_LFLAGS) 

bin/narwhal-webkit-debug: $(SOURCE) NWDebug.m lib/libnarwhal.$(SO_EXT)
	mkdir -p `dirname $@`
	$(CC) -o $@ -DWEBKIT_DEBUG -x objective-c $(SOURCE) NWDebug.m $(CFLAGS) $(INCLUDES) $(LIBS) \
		-framework Foundation -framework WebKit \
		-framework AppKit -sectcreate __TEXT __info_plist Info.plist

bin/narwhal-jscocoa: $(SOURCE) lib/libnarwhal.$(SO_EXT)
	mkdir -p `dirname $@`
	$(CC) -o $@ -DJSCOCOA -x objective-c $(SOURCE) $(CFLAGS) $(INCLUDES) $(LIBS) \
		-framework Foundation -framework WebKit \
		-framework JSCocoa
config-jscore:
	echo 'export NARWHAL_JSC_MODE="jscore"' > narwhal-jsc.conf

config-webkit:
	echo 'export NARWHAL_JSC_MODE="webkit"' > narwhal-jsc.conf

config-webkit-debug:
	echo 'export NARWHAL_JSC_MODE="webkit-debug"' > narwhal-jsc.conf

config-jscocoa: 
	echo 'export NARWHAL_JSC_MODE="jscocoa"' > narwhal-jsc.conf

modules: $(MODULES) 

rewrite-lib-paths:
	find lib -name "*.$(SO_EXT)" \! -path "*.dSYM*" -exec install_name_tool -change "$(SYSTEM_JSC)" "$(RELATIVE_JSC)" {} \;
	find lib -name "*.$(SO_EXT)" \! -path "*.dSYM*" -exec install_name_tool -change "$(ABSOLUTE_LIBNARWHAL)" "$(RELATIVE_LIBNARWHAL)" {} \;

lib/%.$(SO_EXT): src/%.cc
	mkdir -p `dirname $@`
	$(CPP) -o $@ $< $(CFLAGS) $(SO_CFLAGS) $(JSCORE_CFLAGS) $(SO_LFLAGS) $(LFLAGS) $(JSCORE_LFLAGS)
	#install_name_tool -change "$(SYSTEM_JSC)" "$(RELATIVE_JSC)" "$@"

lib/readline.$(SO_EXT): src/readline.cc lib/libedit.$(SO_EXT)
	mkdir -p `dirname $@`
	$(CPP) -o $@ $< $(CFLAGS) -Wl,-install_name,`basename $@` $(SO_CFLAGS) $(SO_LFLAGS) $(LFLAGS) $(JSCORE_CFLAGS) $(JSCORE_LFLAGS) -DUSE_EDITLINE -Ideps/libedit-20100424-3.0/src -ledit

lib/jack/handler/jill.$(SO_EXT): src/jack/handler/jill.cc deps/http-parser/http_parser.o lib/io-engine.$(SO_EXT) lib/binary-engine.$(SO_EXT)
	mkdir -p `dirname $@`
	$(CPP) -o $@ $< $(CFLAGS) $(SO_CFLAGS) $(SO_LFLAGS) $(JSCORE_CFLAGS) $(LFLAGS) $(JSCORE_LFLAGS) deps/http-parser/http_parser.o lib/io-engine.$(SO_EXT) lib/binary-engine.$(SO_EXT)

deps/http-parser/http_parser.o:
	cd deps/http-parser && make http_parser.o

deps/libedit-20100424-3.0/src/.libs/libedit.$(SO_EXT): deps/libedit-20100424-3.0
	chmod +x deps/libedit-20100424-3.0/configure
	if [ "$(PLATFORM)" = "Mac OS X" ]; then \
	  (cd deps/libedit-20100424-3.0 && \
	  env CFLAGS="$(CFLAGS)" \
	     LDFLAGS="$(CFLAGS)" \
	     ./configure --disable-static --enable-shared --disable-dependency-tracking && \
	  make) \
	else \
	  (cd deps/libedit-20100424-3.0 && \
	  aclocal && \
      libtoolize && \
      autoconf && \
      ./configure --disable-static --enable-shared --disable-dependency-tracking && \
	 make) \
	 fi

lib/libedit.$(SO_EXT): deps/libedit-20100424-3.0/src/.libs/libedit.$(SO_EXT)
	rm -rf lib/libedit*
	cp -a deps/libedit-20100424-3.0/src/.libs/libedit*.$(SO_EXT)* lib/

frameworks: $(JSCORE_FRAMEWORK)

frameworks-jscocoa: $(JSCOCOA_FRAMEWORK)

$(JSCORE_FRAMEWORK): $(JSCORE_BUILD)
	mkdir -p `dirname $@`
	cp -r $< $@
	install_name_tool -id "@executable_path/../$(FRAMEWORKS_DIR)/JavaScriptCore.framework/JavaScriptCore" $@/JavaScriptCore
$(JSCORE_BUILD): $(JSCORE_CHECKOUT)
	cd deps/JavaScriptCore && xcodebuild -target JavaScriptCore -configuration $(JSCORE_CONFIG)
$(JSCORE_CHECKOUT):
	mkdir -p `dirname $@`
	svn checkout http://svn.webkit.org/repository/webkit/trunk/JavaScriptCore $@

$(JSCOCOA_FRAMEWORK): $(JSCOCOA_BUILD)
	mkdir -p `dirname $@`
	cp -r $< $@
	install_name_tool -id "@loader_path/../$(FRAMEWORKS_DIR)/JSCocoa.framework/JSCocoa" "$@/JSCocoa"
	install_name_tool -change "$(SYSTEM_JSC)" "$(RELATIVE_JSC)" "$@/JSCocoa";

$(JSCOCOA_BUILD): $(JSCOCOA_CHECKOUT)
	cd $(JSCOCOA_CHECKOUT)/JSCocoa && xcodebuild -project "JSCocoa (embed).xcodeproj"

$(JSCOCOA_CHECKOUT):
	mkdir -p `dirname $@`
	git clone git://github.com/parmanoir/jscocoa.git $@

clean:
	find lib -name "*.$(SO_EXT)" -exec rm -rf {} \;
	rm -rf bin/narwhal-jscore* bin/narwhal-webkit* bin/*.$(SO_EXT)* bin/*.dSYM lib/*.dSYM *.ii *.s

cleaner: clean
	cd deps/http-parser && make clean
	rm -rf $(JSCORE_FRAMEWORK) $(JSCOCOA_FRAMEWORK) $(JSCORE_BUILD) $(JSCOCOA_BUILD)

pristine: cleaner
	rm -rf deps

CPP       =g++
CC        =gcc
CPPFLAGS  = -0s -force_cpusubtype_ALL -mmacosx-version-min=10.4 -arch i386 -arch ppc
#CPPFLAGS += -g -O0
#CPPFLAGS += -DDEBUG_ON
#CPPFLAGS += -save-temps

FRAMEWORKS_DIR=frameworks
#FRAMEWORKS=-Fframeworks
#FRAMEWORKS=-F/Users/tlrobinson/code/WebKit/WebKitBuild/Release/

LIB_DIR   =lib

INCLUDES  =-Iinclude
MODULES   =$(patsubst %.cc,%.dylib,$(patsubst src/%,lib/%,$(shell find src -name '*.cc')))
LIBS      =-framework JavaScriptCore -L/usr/lib -liconv -L$(LIB_DIR) -lnarwhal $(FRAMEWORKS)

SOURCE    =narwhal-jsc.c

JSCORE_CONFIG=Production
JSCORE_FRAMEWORK=$(FRAMEWORKS_DIR)/JavaScriptCore.framework
JSCORE_BUILD=deps/JavaScriptCore/build/$(JSCORE_CONFIG)/JavaScriptCore.framework
JSCORE_CHECKOUT=deps/JavaScriptCore

SYSTEM_JSC=/System/Library/Frameworks/JavaScriptCore.framework/Versions/A/JavaScriptCore
RELATIVE_JSC=@executable_path/../frameworks/JavaScriptCore.framework/JavaScriptCore

ABSOLUTE_LIBNARWHAL=$(LIB_DIR)/libnarwhal.dylib
RELATIVE_LIBNARWHAL=@executable_path/../lib/libnarwhal.dylib

JSCOCOA_FRAMEWORK=$(FRAMEWORKS_DIR)/JSCocoa.framework
JSCOCOA_BUILD=deps/JSCocoa/JSCocoa/build/Release/JSCocoa.framework
JSCOCOA_CHECKOUT=deps/JSCocoa

all: webkit webkit-debug jscore

config:
	sh configure

jscore: 		config bin/narwhal-jscore modules config-jscore
webkit: 		config bin/narwhal-webkit modules config-webkit
webkit-debug:	config bin/narwhal-webkit-debug modules config-webkit-debug
jscocoa: 		config frameworks-jscocoa bin/narwhal-jscocoa modules config-jscocoa


lib/libnarwhal.dylib: narwhal.c
	$(CC) -o $@ $< -dynamiclib $(CPPFLAGS) $(INCLUDES) $(FRAMEWORKS) -framework JavaScriptCore

bin/narwhal-jscore: $(SOURCE) lib/libnarwhal.dylib
	mkdir -p `dirname $@`
	$(CC) -o $@ $(SOURCE) $(CPPFLAGS) $(INCLUDES) $(LIBS)
	install_name_tool -change "$(ABSOLUTE_LIBNARWHAL)" "$(RELATIVE_LIBNARWHAL)" "$@"

bin/narwhal-webkit: $(SOURCE) lib/libnarwhal.dylib
	mkdir -p `dirname $@`
	$(CC) -o $@ -DWEBKIT -x objective-c $(SOURCE) $(CPPFLAGS) $(INCLUDES) $(LIBS) \
		-framework Foundation -framework WebKit
	install_name_tool -change "$(ABSOLUTE_LIBNARWHAL)" "$(RELATIVE_LIBNARWHAL)" "$@"

bin/narwhal-webkit-debug: $(SOURCE) NWDebug.m lib/libnarwhal.dylib
	mkdir -p `dirname $@`
	$(CC) -o $@ -DWEBKIT_DEBUG -x objective-c $(SOURCE) NWDebug.m $(CPPFLAGS) $(INCLUDES) $(LIBS) \
		-framework Foundation -framework WebKit \
		-framework AppKit -sectcreate __TEXT __info_plist Info.plist
	install_name_tool -change "$(ABSOLUTE_LIBNARWHAL)" "$(RELATIVE_LIBNARWHAL)" "$@"

bin/narwhal-jscocoa: $(SOURCE) lib/libnarwhal.dylib
	mkdir -p `dirname $@`
	$(CC) -o $@ -DJSCOCOA -x objective-c $(SOURCE) $(CPPFLAGS) $(INCLUDES) $(LIBS) \
		-framework Foundation -framework WebKit \
		-framework JSCocoa
	install_name_tool -change "$(ABSOLUTE_LIBNARWHAL)" "$(RELATIVE_LIBNARWHAL)" "$@"


config-jscore:
	echo 'export NARWHAL_JSC_MODE="jscore"' > narwhal-jsc.conf
	# rm -f bin/narwhal-jsc
	# ln -s narwhal-jscore bin/narwhal-jsc

config-webkit:
	echo 'export NARWHAL_JSC_MODE="webkit"' > narwhal-jsc.conf
	# rm -f bin/narwhal-jsc
	# ln -s narwhal-webkit bin/narwhal-jsc

config-webkit-debug:
	echo 'export NARWHAL_JSC_MODE="webkit-debug"' > narwhal-jsc.conf
	# rm -f bin/narwhal-jsc
	# ln -s narwhal-webkit-debug bin/narwhal-jsc

config-jscocoa: 
	echo 'export NARWHAL_JSC_MODE="jscocoa"' > narwhal-jsc.conf
	# rm -f bin/narwhal-jsc
	# ln -s narwhal-jscocoa bin/narwhal-jsc

modules: $(MODULES) rewrite-lib-paths

rewrite-lib-paths:
	#find lib -name "*.dylib" \! -path "*.dSYM*" -exec install_name_tool -change "$(SYSTEM_JSC)" "$(RELATIVE_JSC)" {} \;
	find lib -name "*.dylib" \! -path "*.dSYM*" -exec install_name_tool -change "$(ABSOLUTE_LIBNARWHAL)" "$(RELATIVE_LIBNARWHAL)" {} \;

lib/%.dylib: src/%.cc
	mkdir -p `dirname $@`
	$(CPP) -o $@ $< $(CPPFLAGS) $(INCLUDES) -dynamiclib $(LIBS)
	#install_name_tool -change "$(SYSTEM_JSC)" "$(RELATIVE_JSC)" "$@"

lib/readline.dylib: src/readline.cc lib/libedit.dylib
	mkdir -p `dirname $@`
	$(CPP) -o $@ $< $(CPPFLAGS) $(INCLUDES) -dynamiclib $(LIBS) -DUSE_EDITLINE -Ideps/libedit-20100424-3.0/src -ledit
	#install_name_tool -change "$(SYSTEM_JSC)" "$(RELATIVE_JSC)" "$@"
	install_name_tool -change "lib/libedit.dylib" "@executable_path/../lib/libedit.dylib" "$@"

lib/jack/handler/jill.dylib: src/jack/handler/jill.cc deps/http-parser/http_parser.o lib/io-engine.dylib lib/binary-engine.dylib
	mkdir -p `dirname $@`
	$(CPP) -o $@ $< $(CPPFLAGS) $(INCLUDES) -dynamiclib $(LIBS) deps/http-parser/http_parser.o lib/io-engine.dylib lib/binary-engine.dylib
	#install_name_tool -change "$(SYSTEM_JSC)" "$(RELATIVE_JSC)" "$@"
	install_name_tool -change "lib/io-engine.dylib" "@executable_path/../lib/io-engine.dylib" "$@"
	install_name_tool -change "lib/binary-engine.dylib" "@executable_path/../lib/binary-engine.dylib" "$@"

deps/http-parser/http_parser.o:
	cd deps/http-parser && make http_parser.o

deps/libedit-20100424-3.0/src/.libs/libedit.dylib: deps/libedit-20100424-3.0
	chmod +x deps/libedit-20100424-3.0/configure
	cd deps/libedit-20100424-3.0 && \
		env CFLAGS="-force_cpusubtype_ALL -mmacosx-version-min=10.4 -arch i386 -arch ppc" \
		LDFLAGS="-force_cpusubtype_ALL -mmacosx-version-min=10.4 -arch i386 -arch ppc" ./configure --disable-dependency-tracking && \
		make

lib/libedit.dylib: deps/libedit-20100424-3.0/src/.libs/libedit.dylib
	cp $< $@

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
	find lib -name "*.dylib" -exec rm -rf {} \;
	rm -rf bin/narwhal-jscore* bin/narwhal-webkit* bin/*.dylib bin/*.dSYM lib/*.dylib lib/*.dSYM *.o *.ii *.s

cleaner: clean
	cd deps/http-parser && make clean
	rm -rf $(JSCORE_FRAMEWORK) $(JSCOCOA_FRAMEWORK) $(JSCORE_BUILD) $(JSCOCOA_BUILD)

pristine: cleaner
	rm -rf deps

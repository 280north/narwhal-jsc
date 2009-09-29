CPP       =g++
CPPFLAGS   = -0s
#CPPFLAGS += -g -O0
#CPPFLAGS += -DDEBUG_ON
#CPPFLAGS += -save-temps

FRAMEWORKS_DIR=frameworks

INCLUDES  =-Iinclude
MODULES   =$(patsubst %.cc,%.dylib,$(patsubst src/%,lib/%,$(shell find src -name '*.cc')))
LIBS      =-framework JavaScriptCore -L/usr/lib -lreadline -liconv -Llib -lnarwhal
LIBS     +=-F$(FRAMEWORKS_DIR)

SOURCE    =narwhal-jsc.c
EXECUTABLE=bin/narwhal-jsc

JSCORE_CONFIG=Production
JSCORE_FRAMEWORK=$(FRAMEWORKS_DIR)/JavaScriptCore.framework
JSCORE_BUILD=deps/JavaScriptCore/build/$(JSCORE_CONFIG)/JavaScriptCore.framework
JSCORE_CHECKOUT=deps/JavaScriptCore

SYSTEM_JSC=/System/Library/Frameworks/JavaScriptCore.framework/Versions/A/JavaScriptCore
RELATIVE_JSC=@executable_path/../frameworks/JavaScriptCore.framework/JavaScriptCore

JSCOCOA_FRAMEWORK=$(FRAMEWORKS_DIR)/JSCocoa.framework
JSCOCOA_BUILD=deps/JSCocoa/JSCocoa/build/Release/JSCocoa.framework
JSCOCOA_CHECKOUT=deps/JSCocoa

all: frameworks jsc modules rewrite-lib-paths
jscocoa: frameworks-jscocoa jsc-jscocoa modules rewrite-lib-paths
webkit: jsc-webkit modules

lib/libnarwhal.dylib: narwhal.c
	$(CPP) $(CPPFLAGS) $(INCLUDES) -dynamiclib -o $@ $< -framework JavaScriptCore

jsc: $(SOURCE) lib/libnarwhal.dylib
	mkdir -p `dirname $(EXECUTABLE)`
	$(CPP) $(CPPFLAGS) $(INCLUDES) -o $(EXECUTABLE) $(SOURCE) $(LIBS) 

jsc-webkit: $(SOURCE) lib/libnarwhal.dylib
	mkdir -p `dirname $(EXECUTABLE)`
	$(CPP) $(CPPFLAGS) $(INCLUDES) -DWEBKIT -o $(EXECUTABLE) -x objective-c $(SOURCE) $(LIBS) \
		-framework WebKit -framework Foundation -ObjC

jsc-jscocoa: $(SOURCE) lib/libnarwhal.dylib
	mkdir -p `dirname $(EXECUTABLE)`
	$(CPP) $(CPPFLAGS) $(INCLUDES) -DJSCOCOA -o $(EXECUTABLE) -x objective-c $(SOURCE) $(LIBS) \
		-framework JSCocoa -framework Foundation -ObjC -F$(FRAMEWORKS_DIR)

frameworks: $(JSCORE_FRAMEWORK)

franmeworks-jscocoa: $(JSCORE_FRAMEWORK) $(JSCOCOA_FRAMEWORK)

modules: $(MODULES)

rewrite-lib-paths:
	find lib -name "*.dylib" \! -path "*.dSYM*" -exec install_name_tool -change "$(SYSTEM_JSC)" "$(RELATIVE_JSC)" {} \;

mongoose.o: mongoose.c
	gcc $(CPPFLAGS) -W -Wall -std=c99 -pedantic -fomit-frame-pointer -c mongoose.c

lib/jack/handler/mongoose.dylib: src/jack/handler/mongoose.cc mongoose.o
	mkdir -p `dirname $@`
	$(CPP) $(CPPFLAGS) $(INCLUDES) -dynamiclib -o $@ $< $(LIBS) mongoose.o
	#install_name_tool -change "$(SYSTEM_JSC)" "$(RELATIVE_JSC)" "$@"

deps/http-parser/http_parser.o: deps/http-parser
	cd deps/http-parser && make http_parser.o

deps/http-parser:
	git clone git://github.com/ry/http-parser.git $@

lib/jack/handler/jill.dylib: src/jack/handler/jill.cc deps/http-parser/http_parser.o lib/io-engine.dylib lib/binary-engine.dylib
	mkdir -p `dirname $@`
	$(CPP) $(CPPFLAGS) $(INCLUDES) -dynamiclib -o $@ $< $(LIBS) deps/http-parser/http_parser.o lib/io-engine.dylib lib/binary-engine.dylib
	#install_name_tool -change "$(SYSTEM_JSC)" "$(RELATIVE_JSC)" "$@"
	
lib/%.dylib: src/%.cc
	mkdir -p `dirname $@`
	$(CPP) $(CPPFLAGS) $(INCLUDES) -dynamiclib -o $@ $< $(LIBS)
	#install_name_tool -change "$(SYSTEM_JSC)" "$(RELATIVE_JSC)" "$@"

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
	rm -rf bin/narwhal-jsc* bin/*.dylib bin/*.dSYM lib/*.dylib lib/*.dSYM $(EXECUTABLE) *.o *.ii *.s

cleaner: clean
	rm -rf $(JSCORE_FRAMEWORK) $(JSCOCOA_FRAMEWORK) $(JSCORE_BUILD) $(JSCOCOA_BUILD)

pristine: cleaner
	rm -rf deps

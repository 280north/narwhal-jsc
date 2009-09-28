CPP       =g++
CPPFLAGS  =-g -O0 -x objective-c #-save-temps

FRAMEWORKS_DIR=frameworks

INCLUDES  =-Iinclude
LIBS      =-lreadline -F$(FRAMEWORKS_DIR) -framework JavaScriptCore -framework WebKit -framework Foundation -L/usr/lib -liconv
MODULES   =$(patsubst %.cc,%.dylib,$(patsubst src/%,lib/%,$(wildcard src/*.cc)))

SOURCE    =narwhal-jsc.c
EXECUTABLE=bin/narwhal-jsc

JSCORE_CONFIG=Debug
JSCORE_FRAMEWORK=$(FRAMEWORKS_DIR)/JavaScriptCore.framework
JSCORE_BUILD=deps/JavaScriptCore/build/$(JSCORE_CONFIG)/JavaScriptCore.framework
JSCORE_CHECKOUT=deps/JavaScriptCore

JSCOCOA_FRAMEWORK=$(FRAMEWORKS_DIR)/JSCocoa.framework
JSCOCOA_BUILD=deps/JSCocoa/JSCocoa/build/Release/JSCocoa.framework
JSCOCOA_CHECKOUT=deps/JSCocoa

FRAMEWORKS=$(JSCORE_FRAMEWORK)

all: frameworks jsc modules
jscocoa: frameworks jsc-jscocoa modules
	
jsc: $(SOURCE)
	mkdir -p `dirname $(EXECUTABLE)`
	$(CPP) $(CPPFLAGS) $(INCLUDES) -o $(EXECUTABLE) $(SOURCE) $(LIBS)

jsc-jscocoa: $(SOURCE)
	mkdir -p `dirname $(EXECUTABLE)`
	$(CPP) $(CPPFLAGS) $(INCLUDES) -DJSCOCOA -o $(EXECUTABLE) -x objective-c $(SOURCE) $(LIBS) \
		-framework JSCocoa -framework Foundation -ObjC

modules: $(MODULES)

mongoose.o: mongoose.c
	gcc $(CPPFLAGS) -W -Wall -std=c99 -pedantic -fomit-frame-pointer -c mongoose.c

lib/mongoose.dylib: src/mongoose.cc mongoose.o
	mkdir -p `dirname $@`
	$(CPP) $(CPPFLAGS) $(INCLUDES) -dynamiclib -o $@ $< $(LIBS) mongoose.o
	install_name_tool -change "/System/Library/Frameworks/JavaScriptCore.framework/Versions/A/JavaScriptCore" "@executable_path/../frameworks/JavaScriptCore.framework/JavaScriptCore" $@

deps/http-parser/http_parser.o: deps/http-parser
	cd deps/http-parser && make http_parser.o

deps/http-parser:
	git clone git://github.com/ry/http-parser.git $@

lib/jill.dylib: src/jill.cc deps/http-parser/http_parser.o lib/io-engine.dylib lib/binary-engine.dylib
	mkdir -p `dirname $@`
	$(CPP) $(CPPFLAGS) $(INCLUDES) -dynamiclib -o $@ $< $(LIBS) deps/http-parser/http_parser.o lib/io-engine.dylib lib/binary-engine.dylib
	install_name_tool -change "/System/Library/Frameworks/JavaScriptCore.framework/Versions/A/JavaScriptCore" "@executable_path/../frameworks/JavaScriptCore.framework/JavaScriptCore" $@
	
lib/%.dylib: src/%.cc
	mkdir -p `dirname $@`
	$(CPP) $(CPPFLAGS) $(INCLUDES) -dynamiclib -o $@ $< $(LIBS)
	install_name_tool -change "/System/Library/Frameworks/JavaScriptCore.framework/Versions/A/JavaScriptCore" "@executable_path/../frameworks/JavaScriptCore.framework/JavaScriptCore" $@

frameworks: $(FRAMEWORKS)

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
	install_name_tool -id "@loader_path/../$(FRAMEWORKS_DIR)/JSCocoa.framework/JSCocoa" $@/JSCocoa
	install_name_tool -change "/System/Library/Frameworks/JavaScriptCore.framework/Versions/A/JavaScriptCore"  "@executable_path/../$(FRAMEWORKS_DIR)/JavaScriptCore.framework/JavaScriptCore" $@/JSCocoa;

$(JSCOCOA_BUILD): $(JSCOCOA_CHECKOUT)
	cd $(JSCOCOA_CHECKOUT)/JSCocoa && xcodebuild -project "JSCocoa (embed).xcodeproj"
$(JSCOCOA_CHECKOUT):
	mkdir -p `dirname $@`
	git clone git://github.com/parmanoir/jscocoa.git $@

clean:
	rm -rf bin/narwhal-jsc* bin/*.dylib bin/*.dSYM lib/*.dylib lib/*.dSYM $(EXECUTABLE) *.o *.ii *.s

cleaner: clean
	rm -rf $(JSCORE_FRAMEWORK) $(JSCOCOA_FRAMEWORK) $(JSCORE_BUILD) $(JSCOCOA_BUILD)

pristine: cleaner
	rm -rf deps

CPP       =g++
CPPFLAGS  =-g -arch i386
INCLUDES  =-Iinclude
LIBS      =-lreadline -F. -framework JavaScriptCore
MODULES   =$(patsubst %.cc,%.dylib,$(patsubst src/%,lib/%,$(wildcard src/*.cc)))

SOURCE    =narwhal-jsc.c
EXECUTABLE=bin/narwhal-jsc

JSCFLAGS=-DJSCOCOA -framework JSCocoa -framework Foundation -ObjC -x objective-c

all: $(EXECUTABLE) modules

$(EXECUTABLE): jsc
	
jsc: $(SOURCE) bin
	$(CPP) $(CPPFLAGS) $(INCLUDES) -o $(EXECUTABLE) $(SOURCE) $(LIBS)

jscocoa: $(SOURCE) bin
	$(CPP) $(CPPFLAGS) $(INCLUDES) $(JSCFLAGS) -o $(EXECUTABLE) $(SOURCE) $(LIBS)

modules: $(MODULES)
	
lib/%.dylib: src/%.cc lib
	$(CPP) $(CPPFLAGS) $(INCLUDES) -dynamiclib -o $@ $< -framework JavaScriptCore

frameworks: JavaScriptCore.framework JSCocoa.framework

JavaScriptCore.framework: deps/JavaScriptCore	
	cd $< && xcodebuild -target JavaScriptCore
	cp -r $</build/Production/JavaScriptCore.framework $@

JSCocoa.framework: deps/JSCocoa
	cd $</JSCocoa && xcodebuild -project "JSCocoa (embed).xcodeproj"
	cp -r $</JSCocoa/build/Release/JSCocoa.framework $@

deps/JavaScriptCore: deps
	svn checkout http://svn.webkit.org/repository/webkit/trunk/JavaScriptCore $@

deps/JSCocoa: deps
	git clone git://github.com/parmanoir/jscocoa.git $@

clean:
	rm -rf bin/narwhal-jsc* bin/*.dylib bin/*.dSYM lib/*.dylib lib/*.dSYM

cleaner: clean
	rm -rf JavaScriptCore/build JSCocoa/JSCocoa/build

pristine: clean
	rm -rf JavaScriptCore JSCocoa

bin:
	mkdir -p $@
lib:
	mkdir -p $@
deps:
	mkdir -p $@

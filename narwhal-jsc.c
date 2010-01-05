#include <narwhal.h>

#ifdef JSCOCOA
#import <JSCocoa/JSCocoa.h>
#endif

#ifdef WEBKIT
#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>
#elif WEBKIT_DEBUG
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <WebKit/WebKit.h>
#import "NWDebug.h"
#endif

int main(int argc, char *argv[], char *envp[])
{
    #ifdef WEBKIT
        NSAutoreleasePool *pool = [NSAutoreleasePool new];
        WebView *webView = [[WebView alloc] init];
        JSGlobalContextRef _context = [[webView mainFrame] globalContext];
    #elif WEBKIT_DEBUG
        NSAutoreleasePool *pool = [NSAutoreleasePool new];
        WebView *webView = NW_init(argc, argv, envp);
        JSGlobalContextRef _context = [[webView mainFrame] globalContext];
    #elif defined(JSCOCOA)
        NSAutoreleasePool *pool = [NSAutoreleasePool new];
        JSCocoaController *jsc = [JSCocoa new];
        JSGlobalContextRef _context = [jsc ctx];
    #else
        JSGlobalContextRef _context = JSGlobalContextCreate(NULL);
    #endif
    
    #ifdef WEBKIT_DEBUG
        return NSApplicationMain(argc, (const char **)argv);
    #else
        JSValueRef exception = NULL;
        JSValueRef *_exception = &exception;
        
        CALL(narwhal, argc, argv, envp, 1);
    
        int code = 0;
        if (*_exception) {
            code = 1;
            JS_Print(*_exception);
        }
        
        #ifdef WEBKIT
            [pool drain];
        #elif defined(JSCOCOA)
            [pool drain];
        #endif
        
        return code;
    #endif
}

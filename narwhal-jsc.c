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
#elif WEBKIT_GTK
#include "webkit/webkitwebview.h"
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
    #elif defined(WEBKIT_GTK)

	gtk_init(0, NULL);

        WebKitWebView*view = webkit_web_view_new();
 
        WebKitWebFrame* frame = webkit_web_view_get_main_frame(view);

        JSGlobalContextRef _context = 
             webkit_web_frame_get_global_context(frame);
    #else
        JSGlobalContextRef _context = JSGlobalContextCreate(NULL);
    #endif
    
    #ifdef WEBKIT_DEBUG
        return NSApplicationMain(argc, (const char **)argv);
    #else
        JSValueRef exception = NULL;
        JSValueRef *_exception = &exception;
        
        CALL(narwhal, argc, argv, envp, 1);
    
        int code = !!(*_exception);
        
        #ifdef WEBKIT
            [pool drain];
        #elif defined(JSCOCOA)
            [pool drain];
        #endif
        
        return code;
    #endif
}

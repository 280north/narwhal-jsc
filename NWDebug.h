#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

@interface NWApplication : NSApplication{
}
@end

@interface NWAppDelegate : NSObject {
    WebView         *webView;
    NSFileHandle    *stdinFileHandle;
}
- (void)replStart;
@end

@interface NWInspector : NSObject {
}
@end

WebView * NW_init();

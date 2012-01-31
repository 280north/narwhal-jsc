#include <narwhal.h>

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

int main(int argc, char *argv[], char *envp[])
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];

    JSGlobalContextRef _context = [[[[WebView alloc] init] mainFrame] globalContext];

    JSValueRef exception = NULL;
    JSValueRef *_exception = &exception;

    CALL(narwhal, argc, argv, envp, 1);

    int code = !!(*_exception);

    return code;
}

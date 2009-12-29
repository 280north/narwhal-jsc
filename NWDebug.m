/*
    based on "Nibless" project from http://lapcatsoftware.com/
    http://lapcatsoftware.com/downloads/Nibless.zip
*/

#import "NWDebug.h"

#import <objc/runtime.h>

//#include <narwhal.h>

static int global_argc;
static char **global_argv;
static char **global_envp;
static WebView *global_webView;

extern int narwhal(JSGlobalContextRef _context, JSValueRef *_exception, int argc, char *argv[], char *envp[], int runShell);

@implementation NWApplication

- (id)init
{
	if ((self = [super init]))
    {
		NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
		[self setDelegate:[[NWAppDelegate alloc] init]];
		[pool release];
	}
	return self;
}

- (void)dealloc
{
	id delegate = [self delegate];
    if (delegate)
    {
		[self setDelegate:nil];
		[delegate release];
	}
	[super dealloc];
}

@end


@implementation NWAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    webView = global_webView;// [[WebView alloc] init]
    
    id inspector = [webView inspector];
    Class inspectorClass = [[inspector class] class];
    //class_addMethod(inspectorClass, @selector(isSelectorExcludedFromWebScript:), (IMP)NW_isSelectorExcludedFromWebScript, "B@::");
    method_exchangeImplementations(
        class_getClassMethod(inspectorClass, @selector(isSelectorExcludedFromWebScript:)),
        class_getClassMethod([NWInspector class], @selector(isSelectorExcludedFromWebScript:))
    );
    
    id win = [webView windowScriptObject];
    [win setValue:inspector forKey:@"_inspector"];
    
    JSGlobalContextRef context = [[webView mainFrame] globalContext];
    JSValueRef exception = NULL;
    narwhal(context, &exception, global_argc, global_argv, global_envp, 0);
    if (exception) {
        exit(1);
    }
    
    [self replStart];
    
    //[[webView inspector] show:self];
    //[inspector showConsole:self];
    
    [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
}

- (void)replPrompt
{
    fprintf(stdout, "js> ");
    fflush(stdout);
    [stdinFileHandle readInBackgroundAndNotify];
}

- (void)replStart
{
    stdinFileHandle = [[NSFileHandle fileHandleWithStandardInput] retain];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(didReadStdin:)
                                                 name:NSFileHandleReadCompletionNotification
                                               object:stdinFileHandle];
    [self replPrompt];
}

- (void)didReadStdin:(NSNotification *)aNotification
{
    NSData *data = [[aNotification userInfo] objectForKey:NSFileHandleNotificationDataItem];
    NSString *string = [[NSString alloc] initWithData:data encoding:NSASCIIStringEncoding];

    JSStringRef source = JSStringCreateWithUTF8CString([string UTF8String]);
    EvaluateREPL([[webView mainFrame] globalContext], source);
    JSStringRelease(source);

    [self replPrompt];
}

@end

@implementation NWInspector

+ (BOOL)isSelectorExcludedFromWebScript:(SEL)aSelector
{
    //if (aSelector == @selector(nameAtIndex:))
        return NO;
    //return YES;
}

@end


@implementation NSBundle (NWDebug)

+ (BOOL)NW_loadNibNamed:(NSString *)aNibName owner:(id)owner
{
	BOOL didLoadNib = YES;
	if (aNibName != nil || owner != NSApp)
	{
		didLoadNib = [self NW_loadNibNamed:aNibName owner:owner];
	}
	return didLoadNib;
}

@end


void NW_install_NSBundle_hack()
{
    Class bundleClass = [NSBundle class];
    Method originalMethod = class_getClassMethod(bundleClass, @selector(loadNibNamed:owner:));
    Method categoryMethod = class_getClassMethod(bundleClass, @selector(NW_loadNibNamed:owner:));
    method_exchangeImplementations(originalMethod, categoryMethod);
}

void NW_register_defaults()
{    
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    [[NSUserDefaults standardUserDefaults] registerDefaults:
        [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithBool:YES], @"WebKitDeveloperExtras",
            [NSNumber numberWithBool:NO], @"WebKitInspectorAttached",
            
            [NSNumber numberWithBool:YES], @"WebKit Web Inspector Setting - debuggerEnabled",
            [NSNumber numberWithBool:YES], @"WebKit Web Inspector Setting - profilerEnabled",
            //[NSNumber numberWithBool:YES], @"WebKit Web Inspector Setting - resourceTrackingEnabled",
            nil]];

    [pool release];
}

WebView * NW_init(int argc, char *argv[], char *envp[])
{
    NW_install_NSBundle_hack();
    NW_register_defaults();
    
    global_argc = argc;
    global_argv = argv;
    global_envp = envp;
    global_webView = [[WebView alloc] init];
    
    return global_webView;
}

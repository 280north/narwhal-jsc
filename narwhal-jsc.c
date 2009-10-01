#define NOTRACE

#include <JavaScriptCore/JavaScriptCore.h>
#include <stdio.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <narwhal.h>

#include <dlfcn.h>

#ifdef JSCOCOA
#import <JSCocoa/JSCocoa.h>
#endif

#ifdef WEBKIT
#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>
#endif

NarwhalContext narwhal_context;

// Reads a file into a v8 string.
JSStringRef ReadFile(const char* name) {
    FILE* file = fopen(name, "rb");
    if (file == NULL) return NULL;

    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    rewind(file);

    char *chars = (char*)malloc(sizeof(char) * (size + 1));
    int i;
    for (i = 0; i < size;) {
        int read = fread(&chars[i], 1, size - i, file);
        i += read;
    }
    chars[size] = '\0';
    fclose(file);
    JSStringRef result = JSStringCreateWithUTF8CString(chars);
    free(chars);
    return result;
}

FUNCTION(NW_print)
{
    size_t i;
    for (i = 0; i < ARGC; i++) {
        JS_Print(ARGV(i));
        if (_exception)
            return NULL;
    }
        
    return JS_undefined;
}
END

FUNCTION(NW_read, ARG_UTF8(path))
{
    ARG_COUNT(1);

    JSStringRef contents = ReadFile(path);

    if (contents) {
        JSValueRef result = JSValueMakeString(_context, contents);
        JSStringRelease(contents);
        
        if (result)
            return result;
    }
    
    *_exception = JSValueMakeStringWithUTF8CString(_context, "read() error occurred");
    
    return JS_undefined;
}
END

FUNCTION(NW_isFile, ARG_UTF8(path))
{
    ARG_COUNT(1);

    struct stat stat_info;
    int ret = stat(path, &stat_info);

    return JS_bool(ret != -1 && S_ISREG(stat_info.st_mode));
}
END

typedef JSObjectCallAsFunctionCallback factory_t;
typedef const char *(*getModuleName_t)(NarwhalContext *);

FUNCTION(NW_requireNative, ARG_UTF8(topId), ARG_UTF8(path))
{
    ARG_COUNT(2)
    
    DEBUG("RequireNative: topId=[%s] path=[%s] \n", topId, path);
    
    void *handle = dlopen(path, RTLD_LOCAL | RTLD_LAZY);
    if (handle == NULL) {
        printf("dlopen error: %s\n", dlerror());
        return JS_null;
    }
    
    getModuleName_t narwhalModuleInit = (getModuleName_t)dlsym(handle, "narwhalModuleInit");
    if (narwhalModuleInit == NULL) {
        fprintf(stderr, "dlsym (getModuleName) error: %s\n", dlerror());
        return JS_null;
    }
    
    const char *moduleName = narwhalModuleInit(&narwhal_context);
    
    factory_t func = (factory_t)dlsym(handle, moduleName);
    if (func == NULL) {
        fprintf(stderr, "dlsym (%s) error: %s\n", moduleName, dlerror());
        return JS_null;
    }
    
    return JS_fn(func);
}
END

JSObjectRef envpToObject(JSGlobalContextRef _context, JSValueRef *_exception, char *envp[])
{
    JSObjectRef ENV = JSObjectMake(_context, NULL, NULL);
    
    char *key, *value;
    while (key = *(envp++)) {
        value = strchr(key, '=') + 1;
        *(value - 1) = '\0';
        
        SET_VALUE(ENV, key, JS_str_utf8(value, strlen(value)));
        HANDLE_EXCEPTION(true, true);
        
        *(value - 1) = '=';
    }
    
    return ENV;
}

JSObjectRef argvToArray(JSGlobalContextRef _context, JSValueRef *_exception, int argc, char *argv[])
{
    JSValueRef arguments[argc];
    
    int i;
    for (i = 0; i < argc; i++)
        arguments[i] = JSValueMakeStringWithUTF8CString(_context, argv[i]);
    
    JSObjectRef ARGS = JSObjectMakeArray(_context, argc, arguments, _exception);
    HANDLE_EXCEPTION(true, true);
    
    return ARGS;
}

// The read-eval-execute loop of the shell.
void* RunShell(JSContextRef _context) {
    printf("Narwhal version %s, JavaScriptCore engine\n", NARWHAL_VERSION);
    while (true)
    {
#ifdef JSCOCOA
        NSAutoreleasePool *pool = [NSAutoreleasePool new];
#endif
        char *str = readline("> ");
        if (str && *str)
            add_history(str);
        
        if (!str)
            break;
        
        JSStringRef source = JSStringCreateWithUTF8CString(str);
        free(str);
        
        LOCK();
        
        JSValueRef exception = NULL;
        JSValueRef *_exception = &exception;
        if (JSCheckScriptSyntax(_context, source, 0, 0, _exception) && !(*_exception))
        {
            JSValueRef value = JSEvaluateScript(_context, source, 0, 0, 0, _exception);
            HANDLE_EXCEPTION(true, false);
            
            if (value && !JSValueIsUndefined(_context, value)) {
                JS_Print(value);
                HANDLE_EXCEPTION(true, false);
            }
        }
        else
        {
            HANDLE_EXCEPTION(true, false);
        }
        
        JSStringRelease(source);
        
        UNLOCK();
        
#ifdef JSCOCOA
        [pool drain];
#endif
    }
    printf("\n");
}

JSValueRef narwhal(JSGlobalContextRef _context, JSValueRef *_exception, int argc, char *argv[], char *envp[])
{
    pthread_mutex_t	_mutex = PTHREAD_MUTEX_INITIALIZER;

    narwhal_context.context = _context;
    narwhal_context.mutex = &_mutex;
    
    LOCK();

    JSObjectRef global = JS_GLOBAL;

    SET_VALUE(global, "print",          JS_fn(NW_print));
    SET_VALUE(global, "isFile",         JS_fn(NW_isFile));
    SET_VALUE(global, "read",           JS_fn(NW_read));
    SET_VALUE(global, "requireNative",  JS_fn(NW_requireNative));
    SET_VALUE(global, "ARGS",           CALL(argvToArray, argc, argv));
    SET_VALUE(global, "ENV",            CALL(envpToObject, envp));
    
    // Load bootstrap.js
    char *NARWHAL_ENGINE_HOME = getenv("NARWHAL_ENGINE_HOME");
    char *bootstrapPathRelative = "/bootstrap.js";
    char bootstrapPathFull[strlen(NARWHAL_ENGINE_HOME) + strlen(bootstrapPathRelative) + 1];
    snprintf(bootstrapPathFull, sizeof(bootstrapPathFull), "%s%s", NARWHAL_ENGINE_HOME, bootstrapPathRelative);
    
    JSStringRef bootstrapSource = ReadFile(bootstrapPathFull);
    if (!bootstrapSource) {
        THROW("Error reading bootstrap.js\n");
    }
    
    JSEvaluateScript(_context, bootstrapSource, 0, 0, 0, _exception);
    JSStringRelease(bootstrapSource);
    
    UNLOCK();
    
    if (!*_exception && argc <= 1)
        RunShell(_context);
}

int main(int argc, char *argv[], char *envp[])
{
    #ifdef WEBKIT
        NSAutoreleasePool *pool = [NSAutoreleasePool new];
        WebView * webView = [[WebView alloc] init];
        JSGlobalContextRef _context = [[webView mainFrame] globalContext];
    #elif defined(JSCOCOA)
        NSAutoreleasePool *pool = [NSAutoreleasePool new];
        JSCocoaController *jsc = [JSCocoa new];
        JSGlobalContextRef _context = [jsc ctx];
    #else
        JSGlobalContextRef _context = JSGlobalContextCreate(NULL);
    #endif
    
    JSValueRef exception = NULL;
    JSValueRef *_exception = &exception;
        
    CALL(narwhal, argc, argv, envp);
    
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
}

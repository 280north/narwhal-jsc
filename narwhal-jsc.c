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

void JSValuePrint(JSContextRef ctx, JSValueRef value, JSValueRef *_exception)
{
    JSStringRef string = JSValueToStringCopy(ctx, value, _exception);
    if (!string || _exception && *_exception)
        return;
    
    size_t length = JSStringGetMaximumUTF8CStringSize(string);
    
    char *buffer = (char*)malloc(length);
    if (!buffer) {
        if (_exception)
            *_exception = JSValueMakeStringWithUTF8CString(ctx, "print error (malloc)");
        JSStringRelease(string);
        return;
    }
    
    JSStringGetUTF8CString(string, buffer, length);
    JSStringRelease(string);
    
    puts(buffer);
    free(buffer);
}


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

FUNCTION(Print)
{
    size_t i;
    for (i = 0; i < ARGC; i++) {
        JSValuePrint(context, ARGV(i), _exception);
        if (_exception)
            return NULL;
    }
        
    return JS_undefined;
}
END

FUNCTION(Read, ARG_UTF8(path))
{
    ARG_COUNT(1);

    JSStringRef contents = ReadFile(path);

    if (contents) {
        JSValueRef result = JSValueMakeString(context, contents);
        JSStringRelease(contents);
        
        if (result)
            return result;
    }
    
    *_exception = JSValueMakeStringWithUTF8CString(context, "read() error occurred");
    
    return JS_undefined;
}
END

FUNCTION(IsFile, ARG_UTF8(path))
{
    ARG_COUNT(1);

    struct stat stat_info;
    int ret = stat(path, &stat_info);

    return JS_bool(ret != -1 && S_ISREG(stat_info.st_mode));
}
END

typedef JSObjectCallAsFunctionCallback factory_t;
typedef const char *(*getModuleName_t)(void);

FUNCTION(RequireNative, ARG_UTF8(topId), ARG_UTF8(path))
{
    ARG_COUNT(2)
    
    DEBUG("RequireNative: topId=[%s] path=[%s] \n", topId, path);
    
    void *handle = dlopen(path, RTLD_LOCAL | RTLD_LAZY);
    if (handle == NULL) {
        printf("dlopen error: %s\n", dlerror());
        return JS_null;
    }
    
    getModuleName_t getModuleName = (getModuleName_t)dlsym(handle, "getModuleName");
    if (getModuleName == NULL) {
        fprintf(stderr, "dlsym (getModuleName) error: %s\n", dlerror());
        return JS_null;
    }
    
    factory_t func = (factory_t)dlsym(handle, getModuleName());
    if (func == NULL) {
        fprintf(stderr, "dlsym (%s) error: %s\n", getModuleName(), dlerror());
        return JS_null;
    }
    
    return JS_fn(func);
}
END

JSObjectRef envpToObject(JSGlobalContextRef context, char *envp[])
{
    JSValueRef _exception = NULL;
    JSObjectRef ENV = JSObjectMake(context, NULL, NULL);
    
    char *key, *value;
    while (key = *(envp++)) {
        value = strchr(key, '=') + 1;
        *(value - 1) = '\0';
        
        JSStringRef propertyName = JSStringCreateWithUTF8CString(key);
        JSObjectSetProperty(context,
            ENV,
            propertyName,
            JSValueMakeStringWithUTF8CString(context, value),
            kJSPropertyAttributeNone,
            &_exception);
        JSStringRelease(propertyName);
        
        if (_exception) {
            JSValuePrint(context, _exception, NULL);
            ENV = NULL;
        }
        
        *(value - 1) = '=';
    }
    
    return ENV;
}

JSObjectRef argvToArray(JSGlobalContextRef context, int argc, char *argv[])
{
    JSValueRef _exception = NULL;
    JSValueRef arguments[argc];
    
    int i;
    for (i = 0; i < argc; i++)
        arguments[i] = JSValueMakeStringWithUTF8CString(context, argv[i]);
    
    JSObjectRef ARGS = JSObjectMakeArray(context, argc, arguments, &_exception);
    if (_exception) {
        JSValuePrint(context, _exception, NULL);
        ARGS = NULL;
    }
    
    return ARGS;
}

// The read-eval-execute loop of the shell.
void RunShell(JSContextRef context) {
    printf("Narwhal version %s, JavaScriptCore version %s\n", "0.1", "FOO");
    while (true)
    {
#ifdef JSCOCOA
        NSAutoreleasePool *pool = [NSAutoreleasePool new];
#endif
        char *str = readline("> ");
        if (str && *str)
            add_history(str);
        
        if (str == NULL) break;
        
        JSStringRef source = JSStringCreateWithUTF8CString(str);
        free(str);
        
        JSValueRef _exception = NULL;
        
        if (JSCheckScriptSyntax(context, source, 0, 0, &_exception) && !_exception)
        {
            JSValueRef value = JSEvaluateScript(context, source, 0, 0, 0, &_exception);
            
            if (_exception)
                JSValuePrint(context, _exception, NULL);
            
            if (value && !JSValueIsUndefined(context, value))
                JSValuePrint(context, value, &_exception);
        }
        else
        {
            printf("Syntax error\n");
        }
        
        JSStringRelease(source);

#ifdef JSCOCOA
        [pool drain];
#endif
    }
    printf("\n");
}

int narwhal(int argc, char *argv[], char *envp[])
{
#ifdef JSCOCOA
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    JSCocoaController* jsc = [JSCocoa new];
    JSGlobalContextRef context = [jsc ctx];
#else
    JSGlobalContextRef context = JSGlobalContextCreate(NULL);
#endif

    JSObjectRef global = JS_GLOBAL;
    
    JSValueRef exception = NULL;
    JSValueRef *_exception = &exception;

    SET_VALUE(global, "print",          JS_fn(Print));
    SET_VALUE(global, "isFile",         JS_fn(IsFile));
    SET_VALUE(global, "read",           JS_fn(Read));
    SET_VALUE(global, "requireNative",  JS_fn(RequireNative));
    SET_VALUE(global, "ARGS",           argvToArray(context, argc, argv));
    SET_VALUE(global, "ENV",            envpToObject(context, envp));
    
    // Load bootstrap.js
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%s/bootstrap.js", getenv("NARWHAL_ENGINE_HOME"));
    JSStringRef source = ReadFile(buffer);
    if (!source) {
        printf("Error reading bootstrap.js\n");
        return 1;
    }
    JSEvaluateScript(context, source, 0, 0, 0, _exception);
    JSStringRelease(source);
    if (_exception) {
        JSValuePrint(context, *_exception, NULL);
        return 1;
    }
    
    RunShell(context);

#ifdef JSCOCOA
    [pool drain];
#endif
}

int main(int argc, char *argv[], char *envp[])
{
    return narwhal(argc, argv, envp);
}

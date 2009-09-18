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

void JSValuePrint(JSContextRef ctx, JSValueRef value, JSValueRef *exception)
{
    JSStringRef string = JSValueToStringCopy(ctx, value, exception);
    if (!string || exception && *exception)
        return;
    
    size_t length = JSStringGetMaximumUTF8CStringSize(string);
    
    char *buffer = (char*)malloc(length);
    if (!buffer) {
        if (exception)
            *exception = JSValueMakeStringWithUTF8CString(ctx, "print error (malloc)");
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

JSValueRef Print(
    JSContextRef context,
    JSObjectRef function,
    JSObjectRef thisObject,
    size_t argumentCount,
    const JSValueRef arguments[],
    JSValueRef *exception)
{
    size_t i;
    for (i = 0; i < argumentCount; i++) {
        JSValuePrint(context, arguments[i], exception);
        if (exception)
            return NULL;
    }
        
    return JSValueMakeUndefined(context);
}

JSValueRef Read(
    JSContextRef context,
    JSObjectRef function,
    JSObjectRef thisObject,
    size_t argumentCount,
    const JSValueRef arguments[],
    JSValueRef *exception)
{
    if (argumentCount == 0) {
        *exception = JSValueMakeStringWithUTF8CString(context, "read() takes one argument");
        return JSValueMakeUndefined(context);
    }
    
    JSStringRef string = JSValueToStringCopy(context, arguments[0], exception);
    if (*exception) {
        return JSValueMakeUndefined(context);
    }
    
    size_t length = JSStringGetMaximumUTF8CStringSize(string);
    char *buffer = (char*)malloc(length);
    if (!buffer) {
        *exception = JSValueMakeStringWithUTF8CString(context, "read() error occurred");
        return JSValueMakeUndefined(context);
    }
        
    JSStringGetUTF8CString(string, buffer, length);
    JSStringRelease(string);
    
    JSStringRef contents = ReadFile(buffer);
    free(buffer);
    
    if (contents) {
        JSValueRef result = JSValueMakeString(context, contents);
        JSStringRelease(contents);
        
        if (result)
            return result;
    }
    
    *exception = JSValueMakeStringWithUTF8CString(context, "read() error occurred");
    return JSValueMakeUndefined(context);
}

FUNCTION(IsFile)
{
    ARG_COUNT(1);
    
    ARGN_UTF8(buffer, 0);
    
	struct stat stat_info;
    int ret = stat(buffer, &stat_info);
    
    return JSValueMakeBoolean(context, ret != -1 && S_ISREG(stat_info.st_mode));
}
END

typedef JSValueRef (*factory_t)(JSContextRef context,\
    JSObjectRef function,\
    JSObjectRef thisObject,\
    size_t argumentCount,\
    const JSValueRef arguments[],\
    JSValueRef *exception);
typedef const char *(*getModuleName_t)(void);

FUNCTION(RequireNative)
{
	ARG_COUNT(2)
	ARGN_UTF8(topId,0);
    ARGN_UTF8(path,1);
    
    printf("RequireNative! [%s] [%s] \n", topId, path);
    
    void *handle = dlopen(path, RTLD_LOCAL | RTLD_LAZY);
    //printf("handle=%p\n", handle);
    if (handle == NULL) {
        printf("dlopen error: %s\n", dlerror());
    	return JS_null;
    }
    
    getModuleName_t getModuleName = (getModuleName_t)dlsym(handle, "getModuleName");
    if (getModuleName == NULL) {
        printf("dlsym (getModuleName) error: %s\n", dlerror());
    	return JS_null;
    }
    printf("getModuleName=%p moduleName=%s\n", getModuleName, getModuleName());
    
    factory_t func = (factory_t)dlsym(handle, getModuleName());
    //printf("func=%p\n", func);
    if (func == NULL) {
        printf("dlsym (%s) error: %s\n", getModuleName(), dlerror());
    	return JS_null;
    }
    
    return JS_fn(func);
}
END

JSObjectRef envpToObject(JSGlobalContextRef context, char *envp[])
{
    JSValueRef exception = NULL;
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
            &exception);
        JSStringRelease(propertyName);
        
        if (exception) {
            JSValuePrint(context, exception, NULL);
            ENV = NULL;
        }
        
        *(value - 1) = '=';
    }
    
    return ENV;
}

JSObjectRef argvToArray(JSGlobalContextRef context, int argc, char *argv[])
{
    JSValueRef exception = NULL;
    JSValueRef arguments[argc];
    
    int i;
    for (i = 0; i < argc; i++)
        arguments[i] = JSValueMakeStringWithUTF8CString(context, argv[i]);
    
    JSObjectRef ARGS = JSObjectMakeArray(context, argc, arguments, &exception);
    if (exception) {
        JSValuePrint(context, exception, NULL);
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
        
        JSValueRef exception = NULL;
        
        if (JSCheckScriptSyntax(context, source, 0, 0, &exception) && !exception)
        {
            JSValueRef value = JSEvaluateScript(context, source, 0, 0, 0, &exception);
            
            if (exception)
                JSValuePrint(context, exception, NULL);
            
            if (value && !JSValueIsUndefined(context, value))
                JSValuePrint(context, value, &exception);
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

int main(int argc, char *argv[], char *envp[])
{    
#ifdef JSCOCOA
	NSAutoreleasePool *pool = [NSAutoreleasePool new];
	JSCocoaController* jsc = [JSCocoa new];
	JSGlobalContextRef context = [jsc ctx];
#else
	JSGlobalContextRef context = JSGlobalContextCreate(NULL);
#endif

    JSObjectRef global = JSContextGetGlobalObject(context);
    
    JSValueRef exception = NULL;
    JSStringRef propertyName;
    JSValueRef value;

    // print
    propertyName = JSStringCreateWithUTF8CString("print");
    value = JSObjectMakeFunctionWithCallback(context, propertyName, Print);
    JSObjectSetProperty(context, global, propertyName, value, kJSPropertyAttributeNone, &exception);
    JSStringRelease(propertyName);
    if (exception) {
        JSValuePrint(context, exception, NULL);
        return 1;
    }
    
    // isFile
    propertyName = JSStringCreateWithUTF8CString("isFile");
    value = JSObjectMakeFunctionWithCallback(context, propertyName, IsFile);
    JSObjectSetProperty(context, global, propertyName, value, kJSPropertyAttributeNone, &exception);
    JSStringRelease(propertyName);
    if (exception) {
        JSValuePrint(context, exception, NULL);
        return 1;
    }
    
    // read
    propertyName = JSStringCreateWithUTF8CString("read");
    value = JSObjectMakeFunctionWithCallback(context, propertyName, Read);
    JSObjectSetProperty(context, global, propertyName, value, kJSPropertyAttributeNone, &exception);
    JSStringRelease(propertyName);
    if (exception) {
        JSValuePrint(context, exception, NULL);
        return 1;
    }
    
    // requireNative
    propertyName = JSStringCreateWithUTF8CString("requireNative");
    value = JSObjectMakeFunctionWithCallback(context, propertyName, RequireNative);
    JSObjectSetProperty(context, global, propertyName, value, kJSPropertyAttributeNone, &exception);
    JSStringRelease(propertyName);
    if (exception) {
        JSValuePrint(context, exception, NULL);
        return 1;
    }
    
    // ARGS
    value = argvToArray(context, argc, argv);
    if (value) {
        propertyName = JSStringCreateWithUTF8CString("ARGS");
        JSObjectSetProperty(context, global, propertyName, value, kJSPropertyAttributeNone, &exception);
        JSStringRelease(propertyName);
        if (exception) {
            JSValuePrint(context, exception, NULL);
            return 1;
        }
    }
    else
        printf("Problem setting up ARGS\n");

    // ENV
    value = envpToObject(context, envp);
    if (value) {
        propertyName = JSStringCreateWithUTF8CString("ENV");
        JSObjectSetProperty(context, global, propertyName, value, kJSPropertyAttributeNone, &exception);
        JSStringRelease(propertyName);
        if (exception) {
            JSValuePrint(context, exception, NULL);
            return 1;
        }
    }
    else
        printf("Problem setting up ENV\n");
    
    // Load bootstrap.js
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%s/bootstrap.js", getenv("NARWHAL_ENGINE_HOME"));
    JSStringRef source = ReadFile(buffer);
    if (!source) {
        printf("Error reading bootstrap.js\n");
        return 1;
    }
    JSEvaluateScript(context, source, 0, 0, 0, &exception);
    JSStringRelease(source);
    if (exception) {
        JSValuePrint(context, exception, NULL);
        return 1;
    }
    
    RunShell(context);

#ifdef JSCOCOA
	[pool drain];
#endif
}

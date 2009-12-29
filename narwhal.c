#define NOTRACE

#include <narwhal.h>

//#ifdef WEBKIT

JSObjectRef JSObjectMakeArray(JSContextRef _context, size_t argc, const JSValueRef argv[],  JSValueRef* _exception)
{
    JSObjectRef Array = GET_OBJECT(JS_GLOBAL, "Array");
    return CALL_AS_CONSTRUCTOR(Array, argc, argv);
}

JSObjectRef JSObjectMakeDate(JSContextRef _context, size_t argc, const JSValueRef argv[],  JSValueRef* _exception)
{
    JSObjectRef Date = GET_OBJECT(JS_GLOBAL, "Date");
    return CALL_AS_CONSTRUCTOR(Date, argc, argv);
}

JSObjectRef JSObjectMakeError(JSContextRef _context, size_t argc, const JSValueRef argv[],  JSValueRef* _exception)
{
    JSObjectRef Error = GET_OBJECT(JS_GLOBAL, "Error");
    return CALL_AS_CONSTRUCTOR(Error, argc, argv);
}

JSObjectRef JSObjectMakeRegExp(JSContextRef _context, size_t argc, const JSValueRef argv[],  JSValueRef* _exception)
{
    JSObjectRef RegExp = GET_OBJECT(JS_GLOBAL, "RegExp");
    return CALL_AS_CONSTRUCTOR(RegExp, argc, argv);
}

//#endif


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

void* EvaluateREPL(JSContextRef _context, JSStringRef source)
{
    static JSStringRef replTag;
    if (!replTag)
        replTag = JSStringCreateWithUTF8CString("<repl>");
    
    JSValueRef exception = NULL;
    JSValueRef *_exception = &exception;
    if (JSCheckScriptSyntax(_context, source, 0, 0, _exception) && !(*_exception))
    {
        JSValueRef value = JSEvaluateScript(_context, source, 0, replTag, 0, _exception);
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
}

// The read-eval-execute loop of the shell.
void* RunREPL(JSContextRef _context) {
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
        
        EvaluateREPL(_context, source);
        
        JSStringRelease(source);
        
        UNLOCK();
        
#ifdef JSCOCOA
        [pool drain];
#endif
    }
    printf("\n");
}

JSValueRef narwhal_wrapped(JSGlobalContextRef _context, JSValueRef *_exception, int argc, char *argv[], char *envp[], int runShell)
{
    pthread_mutex_t	_mutex = PTHREAD_MUTEX_INITIALIZER;

    narwhal_context.context = _context;
    narwhal_context.mutex = &_mutex;
    
    LOCK();
    
    // TODO: cleanup all this. strcpy, sprintf, etc BAD!
    char executable[1024];
    char buffer[1024];
    
    // start with the executable name from argv[0]
    strcpy(executable, argv[0]);
    
    // follow any symlinks
    size_t len;
    while ((int)(len = readlink(executable, buffer, sizeof(buffer))) >= 0) {
        buffer[len] = '\0';
        // make relative to symlink's directory
        if (buffer[0] != '/') {
            char tmp[1024];
            strcpy(tmp, buffer);
            sprintf(buffer, "%s/%s", (char *)dirname(executable), tmp);
        }
        strcpy(executable, buffer);
    }
    
    // make absolute
    if (executable[0] != '/') {
        char tmp[1024], tmp2[1024];
        getcwd(tmp, sizeof(tmp));
        sprintf(buffer, "%s/%s", tmp, executable);
        strcpy(executable, buffer);
    }
    
    char NARWHAL_HOME[1024], NARWHAL_ENGINE_HOME[1024];

    // try getting NARWHAL_ENGINE_HOME from env variable. fall back to 2nd ancestor of executable path
    if (getenv("NARWHAL_ENGINE_HOME"))
        strcpy(NARWHAL_ENGINE_HOME, getenv("NARWHAL_ENGINE_HOME"));
    else
        strcpy(NARWHAL_ENGINE_HOME, (char *)dirname((char *)dirname(executable)));

    // try getting NARWHAL_HOME from env variable. fall back to 2nd ancestor of NARWHAL_ENGINE_HOME
    if (getenv("NARWHAL_HOME"))
        strcpy(NARWHAL_HOME, getenv("NARWHAL_HOME"));
    else
        strcpy(NARWHAL_HOME, (char *)dirname((char *)dirname(NARWHAL_ENGINE_HOME)));

    JSObjectRef ARGS = CALL(argvToArray, argc, argv);
    JSObjectRef ENV = CALL(envpToObject, envp);
    
    SET_VALUE(ENV, "NARWHAL_HOME",        JS_str_utf8(NARWHAL_HOME, strlen(NARWHAL_HOME)));
    SET_VALUE(ENV, "NARWHAL_ENGINE_HOME", JS_str_utf8(NARWHAL_ENGINE_HOME, strlen(NARWHAL_ENGINE_HOME)));

    JSObjectRef global = JS_GLOBAL;

    SET_VALUE(global, "print",          JS_fn(NW_print));
    SET_VALUE(global, "isFile",         JS_fn(NW_isFile));
    SET_VALUE(global, "read",           JS_fn(NW_read));
    SET_VALUE(global, "requireNative",  JS_fn(NW_requireNative));
    SET_VALUE(global, "ARGS",           ARGS);
    SET_VALUE(global, "ENV",            ENV);
    
    // Load bootstrap.js
    char *bootstrapPathRelative = "/bootstrap.js";
    char bootstrapPathFull[strlen(NARWHAL_ENGINE_HOME) + strlen(bootstrapPathRelative) + 1];
    snprintf(bootstrapPathFull, sizeof(bootstrapPathFull), "%s%s", NARWHAL_ENGINE_HOME, bootstrapPathRelative);
    
    JSStringRef bootstrapSource = ReadFile(bootstrapPathFull);
    if (!bootstrapSource) {
        THROW("Error reading bootstrap.js\n");
    }
    
    JSStringRef bootstrapTag = JSStringCreateWithUTF8CString(bootstrapPathFull);
    
    JSEvaluateScript(_context, bootstrapSource, 0, bootstrapTag, 0, _exception);
    if (*_exception) {
        JS_Print(*_exception);
    }
    
    JSStringRelease(bootstrapSource);
    JSStringRelease(bootstrapTag);
    
    UNLOCK();
    
    if (!*_exception && argc <= 1 && runShell)
        RunREPL(_context);

}

int narwhal(JSGlobalContextRef _context, JSValueRef *_exception, int argc, char *argv[], char *envp[], int runShell)
{
    narwhal_wrapped(_context, _exception, argc, argv, envp, runShell);
    if (*_exception)
        return 1;
    return 0;
}

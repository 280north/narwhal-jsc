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

typedef JSObjectCallAsFunctionCallback NW_factory_t;
typedef const char *(*NW_narwhalModuleInit_t)();

FUNCTION(NW_requireNative, ARG_UTF8(topId), ARG_UTF8(path))
{
    ARG_COUNT(2)
    
    DEBUG("RequireNative: topId=[%s] path=[%s] \n", topId, path);
    
    void *handle = dlopen(path, RTLD_LOCAL | RTLD_LAZY);
    if (handle == NULL) {
        printf("dlopen error: %s\n", dlerror());
        return JS_null;
    }
    
    NW_narwhalModuleInit_t narwhalModuleInit = (NW_narwhalModuleInit_t)dlsym(handle, "narwhalModuleInit");
    if (narwhalModuleInit == NULL) {
        fprintf(stderr, "dlsym (getModuleName) error: %s\n", dlerror());
        return JS_null;
    }
    
    const char *moduleName = narwhalModuleInit();
    
    NW_factory_t func = (NW_factory_t)dlsym(handle, moduleName);
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

DESTRUCTOR(Context_finalize)
{    
    GET_INTERNAL(ContextPrivate *, data, object);
    DEBUG("freeing Context=[%p]\n", data);
    if (data) {
        // free data->context
        free(data);
    }
}
END
JSClassRef Context_class(JSContextRef _context)
{
    static JSClassRef jsClass;
    if (!jsClass)
    {
        JSClassDefinition definition = kJSClassDefinitionEmpty;
        definition.className = "Context";
        definition.finalize = Context_finalize;

        jsClass = JSClassCreate(&definition);
    }
    return jsClass;
}
JSObjectRef Context_new(JSContextRef _context, JSValueRef *_exception, JSContextRef context)
{
    ContextPrivate *data = (ContextPrivate*)malloc(sizeof(ContextPrivate));
    if (!data) return NULL;
    
    data->context = context;
    
    return JSObjectMake(_context, Context_class(_context), data);
}

FUNCTION(NW_inititialize, ARG_OBJ(context), ARG_OBJ(ARGS), ARG_OBJ(ENV))
{    
    GET_INTERNAL(ContextPrivate*, data, context);
    
    // printf("=========\n");
    // JS_Print(context);
    // JS_Print(ARGS);
    // JS_Print(ENV);
    // printf("---------\n");
    // 
    // printf("=========\n");
    // JS_Print(context);
    // JS_Print(ARGS);
    // JS_Print(ENV);
    // printf("+++++++++\n");
    
    JSValueRef NARWHAL_HOME_val = GET_VALUE(ENV, "NARWHAL_HOME");
    JSValueRef NARWHAL_ENGINE_HOME_val = GET_VALUE(ENV, "NARWHAL_ENGINE_HOME");
    
    // printf("=========\n");
    // JS_Print(NARWHAL_HOME_val);
    // JS_Print(NARWHAL_ENGINE_HOME_val);
    // printf(".........\n");
    
    GET_UTF8(NARWHAL_HOME, NARWHAL_HOME_val);
    GET_UTF8(NARWHAL_ENGINE_HOME, NARWHAL_ENGINE_HOME_val);
    
    // printf("_context=%p\n", _context);
    _context = data->context;
    // printf("_context=%p\n", _context);
    
    JSObjectRef global = JS_GLOBAL;
    
    SET_VALUE(global, "print",          JS_fn(NW_print));
    SET_VALUE(global, "isFile",         JS_fn(NW_isFile));
    SET_VALUE(global, "read",           JS_fn(NW_read));
    SET_VALUE(global, "requireNative",  JS_fn(NW_requireNative));
    SET_VALUE(global, "ARGS",           ARGS);
    SET_VALUE(global, "ENV",            ENV);
    // SET_VALUE(global, "narwhal_init",   JS_fn(NW_inititialize));
    
    // Load bootstrap.js
    char *bootstrapPathRelative = "/bootstrap.js";
    char bootstrapPathFull[strlen(NARWHAL_ENGINE_HOME) + strlen(bootstrapPathRelative) + 1];
    snprintf(bootstrapPathFull, sizeof(bootstrapPathFull), "%s%s", NARWHAL_ENGINE_HOME, bootstrapPathRelative);
    
    JSStringRef bootstrapSource = ReadFile(bootstrapPathFull);
    if (!bootstrapSource) {
        THROW("Error reading %s\n", bootstrapPathFull);
    }
    
    JSStringRef bootstrapTag = JSStringCreateWithUTF8CString(bootstrapPathFull);
    
    JSEvaluateScript(_context, bootstrapSource, 0, bootstrapTag, 0, _exception);
    if (*_exception) {
        JS_Print(*_exception);
    }
    
    JSStringRelease(bootstrapSource);
    JSStringRelease(bootstrapTag);
}
END

JSValueRef narwhal_wrapped(JSGlobalContextRef _context, JSValueRef *_exception, int argc, char *argv[], char *envp[], int runShell)
{
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
            // glibc's dirname modifies it's argument so copy first
            char tmp[1024], tmp2[1024];
            strcpy(tmp, buffer);
            strcpy(tmp2, executable);
            sprintf(buffer, "%s/%s", (char *)dirname(tmp2), tmp);
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
        strcpy(NARWHAL_ENGINE_HOME, (char *)getenv("NARWHAL_ENGINE_HOME"));
    else {
        // glibc's dirname modifies it's argument so copy first
        char tmp[1024];
        strcpy(tmp, executable);
        strcpy(NARWHAL_ENGINE_HOME, (char *)dirname((char *)dirname(tmp)));
    }

    // try getting NARWHAL_HOME from env variable. fall back to 2nd ancestor of NARWHAL_ENGINE_HOME
    if (getenv("NARWHAL_HOME"))
        strcpy(NARWHAL_HOME, (char *)getenv("NARWHAL_HOME"));
    else {
        // glibc's dirname modifies it's argument so copy first
        char tmp[1024];
        strcpy(tmp, NARWHAL_ENGINE_HOME);
        strcpy(NARWHAL_HOME, (char *)dirname((char *)dirname(tmp)));
    }

    JSObjectRef ARGS = CALL(argvToArray, argc, argv);
    JSObjectRef ENV = CALL(envpToObject, envp);

    SET_VALUE(ENV, "NARWHAL_HOME",        JS_str_utf8(NARWHAL_HOME, strlen(NARWHAL_HOME)));
    SET_VALUE(ENV, "NARWHAL_ENGINE_HOME", JS_str_utf8(NARWHAL_ENGINE_HOME, strlen(NARWHAL_ENGINE_HOME)));

    SET_VALUE(ENV, "NARWHAL_RUN_SHELL",   runShell ? JS_str_utf8("1", 1) : JS_str_utf8("0", 1));
    SET_VALUE(ENV, "NARWHAL_VERSION",     JS_str_utf8(NARWHAL_VERSION, strlen(NARWHAL_VERSION)));

    JSObjectRef context = CALL(Context_new, _context);
    ARGS_ARRAY(init_args, context, ARGS, ENV);

    // HACK: call directly
    NW_inititialize(_context, NULL, NULL, 3, init_args, _exception);
    //CALL_AS_FUNCTION(JS_fn(NW_inititialize), JS_GLOBAL, 3, init_args);
}

int narwhal(JSGlobalContextRef _context, JSValueRef *_exception, int argc, char *argv[], char *envp[], int runShell)
{
    narwhal_wrapped(_context, _exception, argc, argv, envp, runShell);
    if (*_exception)
        return 1;
    return 0;
}

#ifndef __NARWHAL__
#define __NARWHAL__

// platform name
#define NARWHAL_JSC

#include <JavaScriptCore/JavaScriptCore.h>
#include <pthread.h>

//#define DEBUG_ON
#ifdef DEBUG_ON
#define DEBUG(...) LOG(__VA_ARGS__)
#define THROW_DEBUG " (%s:%d)\n"
#else
#define NOTRACE
#define DEBUG(...)
#define THROW_DEBUG
#endif

// "#define NOTRACE" at the top of files you don't want to enable tracing on
#ifdef NOTRACE
#define TRACE(...)
#else
#define TRACE(...) LOG(__VA_ARGS__);
#endif

#define ERROR(...) LOG(__VA_ARGS__)
#define LOG(...) fprintf(stderr, __VA_ARGS__);

#define THROW(format, ...) \
    { char msg[1024]; snprintf(msg, 1024, format THROW_DEBUG, ##__VA_ARGS__, __FILE__, __LINE__); \
    DEBUG("THROWING: %s", msg); \
    *_exception = JSValueMakeStringWithUTF8CString(_context, msg); \
    return NULL;/*JSValueMakeUndefined(_context);*/ }

#define HANDLE_EXCEPTION_BLOCK(block) if (*_exception) block;
#define HANDLE_EXCEPTION_PRINT() HANDLE_EXCEPTION_BLOCK({ JS_Print(*_exception); });
#define HANDLE_EXCEPTION_RETURN(...) HANDLE_EXCEPTION_BLOCK({ return __VA_ARGS__; });

#define HANDLE_EXCEPTION(shouldPrint, shouldReturn) \
    HANDLE_EXCEPTION_BLOCK({ \
        if (shouldPrint) {fprintf(stderr, "ERROR:" THROW_DEBUG, __FILE__, __LINE__); JS_Print(*_exception);} \
        if (shouldReturn) return NULL; \
    })

JSValueRef JSValueMakeStringWithUTF8CString(JSContextRef _context, const char *string)
{
    JSStringRef stringRef = JSStringCreateWithUTF8CString(string);
    if (!stringRef)
        return NULL;
    JSValueRef valueRef = JSValueMakeString(_context, stringRef);
    JSStringRelease(stringRef);
    return valueRef;
}

JSValueRef JSValueMakeStringWithUTF16(JSContextRef _context, JSChar *string, size_t len)
{
    JSStringRef stringRef = JSStringCreateWithCharacters(string, len);
    if (!stringRef)
        return NULL;
    JSValueRef valueRef = JSValueMakeString(_context, stringRef);
    JSStringRelease(stringRef);
    return valueRef;
}

void JSValuePrint(JSContextRef _context, JSValueRef *_exception, JSValueRef value)
{
    JSValueRef tmpException = NULL;
    
    if (!value) return;

    JSStringRef string = JSValueToStringCopy(_context, value, &tmpException);
    if (!string || tmpException) {
        if (_exception)
            *_exception = tmpException;
        return;
    }

    size_t length = JSStringGetMaximumUTF8CStringSize(string);
    char buffer[length];
    
    JSStringGetUTF8CString(string, buffer, length);
    JSStringRelease(string);
    
    puts(buffer);
}

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#define THIS _this
#define JS_GLOBAL JSContextGetGlobalObject(_context)

#define JSString JSValueRef
#define JSObject JSObjectRef

#define ARGC _argc
#define ARGV(index) _argv[index]
#define ARG_COUNT(c) if ( ARGC != c ) { THROW("Insufficient arguments"); }

#define IS_NUMBER(value)    JSValueIsNumber(_context, value)
#define IS_OBJECT(value)    JSValueIsObject(_context, value)
#define IS_FUNCTION(value)  JSObjectIsFunction(_context, value)
#define IS_STRING(value)    JSValueIsString(_context, value)

#define ARGN_INT(variable, index) \
    if (index >= ARGC || !IS_NUMBER(ARGV(index))) THROW("Argument %d must be a number.", index) \
    int variable = (int)JSValueToNumber(_context, ARGV(index), _exception); \
    if (*_exception) { return NULL; };
    
#define ARGN_DOUBLE(variable, index) \
    if (index >= ARGC || !IS_NUMBER(ARGV(index))) THROW("Argument %d must be a number.", index) \
    double variable = JSValueToNumber(_context, ARGV(index), _exception); \
    if (*_exception) { return NULL; };
    
#define ARGN_OBJ(variable, index) \
    if (index >= ARGC || !IS_OBJECT(ARGV(index))) THROW("Argument %d must be an object.", index) \
    JSObjectRef variable = JSValueToObject(_context, ARGV(index), _exception); \
    if (*_exception) { return NULL; };
    
#define ARGN_FN(variable, index) \
    ARGN_OBJ(variable, index); \
    if (!IS_FUNCTION(variable)) { THROW("Argument %d must be a function.", index); } \

#define ARGN_STR(variable, index) \
    if (index >= ARGC || !IS_STRING(ARGV(index))) THROW("Argument %d must be a string.", index) \
    JSValueRef variable = ARGV(index);

#define ARGN_STR_OR_NULL(variable, index) \
    JSValueRef variable = ((index < ARGC) && IS_STRING(ARGV(index))) ? ARGV(index) : NULL;


#define GET_UTF8(variable, value) \
    {_tmpStr = JSValueToStringCopy(_context, value, _exception);\
    if (*_exception) { return NULL; }\
    _tmpSz = JSStringGetMaximumUTF8CStringSize(_tmpStr); }\
    char variable[_tmpSz]; \
    {JSStringGetUTF8CString(_tmpStr, variable, _tmpSz);\
    JSStringRelease(_tmpStr);}
    
#define ARGN_UTF8_CAST(variable, index) \
    if (index >= ARGC) THROW("Argument %d must be a string.", index) \
    GET_UTF8(variable, ARGV(index));

#define ARGN_UTF8(variable, index) \
    if (index >= ARGC && !IS_STRING(ARGV(index))) THROW("Argument %d must be a string.", index) \
    ARGN_UTF8_CAST(variable, index)

#define ARG_INT(variable)       0; ARGN_INT(variable, _argn); _argn++; 0
#define ARG_DOUBLE(variable)    0; ARGN_INT(variable, _argn); _argn++; 0
#define ARG_OBJ(variable)       0; ARGN_OBJ(variable, _argn); _argn++; 0
#define ARG_FN(variable)        0; ARGN_FN(variable, _argn); _argn++; 0
#define ARG_STR(variable)       0; ARGN_STR(variable, _argn); _argn++; 0
#define ARG_UTF8(variable)      0; ARGN_UTF8(variable, _argn); _argn++; 0
#define ARG_UTF8_CAST(variable) 0; ARGN_UTF8_CAST(variable, _argn); _argn++; 0

#define JS_undefined    JSValueMakeUndefined(_context)
#define JS_null         JSValueMakeNull(_context)
#define JS_int(number)  JSValueMakeNumber(_context, (double)number)
#define JS_bool(b)      JSValueMakeBoolean(_context, (int)b)
#define JS_str_utf8(str, len) JSValueMakeStringWithUTF8CString(_context, str)    
#define JS_str_utf16(str, len) JSValueMakeStringWithUTF16(_context, (JSChar*)str, (len)/sizeof(JSChar))


#define JS_obj(value)   JSValueToObject(_context, value, _exception)
#define JS_fn(f)        JSObjectMakeFunctionWithCallback(_context, NULL, f)
#define JS_array(count, array) JSObjectMakeArray(_context, count, array, _exception)

#define GET_VALUE(object, name) _GET_VALUE(_context, _exception, object, name)
JSValueRef _GET_VALUE(JSContextRef _context, JSValueRef *_exception, JSObjectRef object, const char *name) {
    JSStringRef nameStr = JSStringCreateWithUTF8CString(name);
    JSValueRef value = JSObjectGetProperty(_context, object, nameStr, _exception);
    JSStringRelease(nameStr);
    return value;
}

#define SET_VALUE(object, name, value) \
    {JSStringRef nameStr = JSStringCreateWithUTF8CString(name); \
    JSObjectSetProperty(_context, object, nameStr, value, kJSPropertyAttributeNone, _exception); \
    JSStringRelease(nameStr); \
    if (*_exception) { return NULL; }}

#define GET_OBJECT(object, name) \
    JSValueToObject(_context, GET_VALUE(object, name), _exception)

#define GET_INT(object, name) \
    ((int)JSValueToNumber(_context, GET_VALUE(object, name), _exception))

#define GET_BOOL(object, name) \
    JSValueToBoolean(_context, GET_VALUE(object, name))

#define EXPORTS(name, object) SET_VALUE(Exports, name, object);

#define FUNC_HEADER(f) \
    JSValueRef f( \
    JSContextRef _ctx, \
    JSObjectRef function, \
    JSObjectRef _this, \
    size_t _argc, \
    const JSValueRef _argv[], \
    JSValueRef *_exception) \

#define FUNCTION(f,...) \
    FUNC_HEADER(f) \
    { \
        TRACE(" *** F: %s\n", STRINGIZE(f)) \
        JSContextRef _context = _ctx; \
        int _argn = 0; \
        JSStringRef _tmpStr; \
        size_t _tmpSz; \
        __VA_ARGS__; \
        
#define CONSTRUCTOR(f,...) \
    JSObjectRef f( \
        JSContextRef _context, \
        JSObjectRef constructor, \
        size_t _argc, \
        const JSValueRef _argv[], \
        JSValueRef *_exception) \
    { \
        TRACE(" *** C: %s\n", STRINGIZE(f)) \
        int _argn = 0; \
        JSStringRef _tmpStr; \
        size_t _tmpSz; \
        __VA_ARGS__; \

#define END \
    }

#define NARWHAL_MODULE(MODULE_NAME) \
    JSObjectRef Require; \
    JSObjectRef Exports; \
    JSObjectRef Module; \
    JSObjectRef System; \
    JSObjectRef Print; \
    JSContextRef _context; \
    NarwhalContext narwhal_context; \
    \
    void print(const char * string)\
    {\
        JSValueRef argv[] = { JS_str_utf8(string, strlen(string)) }; \
        JSObjectCallAsFunction(_context, Print, NULL, 1, argv, NULL); \
    }\
    JSObjectRef require(const char *id) {\
        JSValueRef argv[] = { JS_str_utf8(id, strlen(id)) }; \
        return JSValueToObject(_context, JSObjectCallAsFunction(_context, Require, NULL, 1, argv, NULL), NULL); \
    }\
    \
    const char *moduleName = STRINGIZE(MODULE_NAME);\
    extern "C" const char * narwhalModuleInit(NarwhalContext *_narwhal_context) { \
        narwhal_context = *_narwhal_context; \
        return moduleName; \
    }\
    \
    extern "C" FUNC_HEADER(MODULE_NAME) \
        { _context = _ctx; \
        ARG_COUNT(5); \
        Require = JSValueToObject(_context, ARGV(0), _exception); \
        if (*_exception) { return NULL; }; \
        if (!JSObjectIsFunction(_context, Require)) { THROW("Argument 0 must be a function."); } \
        Exports = JSValueToObject(_context, ARGV(1), _exception); \
        if (*_exception) { return NULL; }; \
        Module = JSValueToObject(_context, ARGV(2), _exception); \
        if (*_exception) { return NULL; }; \
        System = JSValueToObject(_context, ARGV(3), _exception); \
        if (*_exception) { return NULL; }; \
        Print = JSValueToObject(_context, ARGV(4), _exception); \
        if (*_exception) { return NULL; }; \
        if (!JSObjectIsFunction(_context, Print)) { THROW("Argument 4 must be a function."); } \

#define END_NARWHAL_MODULE \
    return JS_undefined; }\

#define GET_INTERNAL(type, name, object) \
    type name = (type) JSObjectGetPrivate(object)
#define SET_INTERNAL(object, data) \
    JSObjectSetPrivate(object, (void *)data)

#define DESTRUCTOR(f) void f(JSObjectRef object) \
    { \

#define ARGS_ARRAY(name, ...) JSValueRef name[] = { __VA_ARGS__ };

#define CALL_AS_FUNCTION(object, thisObject, argc, argv) \
    JSObjectCallAsFunction(_context, object, thisObject, argc, argv, _exception)
    
#define CALL_AS_CONSTRUCTOR(object, argc, argv) \
    JSObjectCallAsConstructor(_context, object, argc, argv, _exception)

#define GET_UTF16(string, buffer, length) _GET_UTF16(_context, _exception, string, (JSChar**)buffer, length)
int _GET_UTF16(JSContextRef _context, JSValueRef *_exception, JSValueRef str, JSChar **buffer, size_t *length) {
    JSStringRef string = JSValueToStringCopy(_context, str, _exception);
    
    *length = (size_t)(JSStringGetLength(string) * sizeof(JSChar));
    *buffer = (JSChar*)malloc(*length);
    
    if (!*buffer)
        return 0;
    
    memcpy(*buffer, JSStringGetCharactersPtr(string), *length);
    
    JSStringRelease(string);
    
    return 1;
}

#define CALL(f, ...) f(_context, _exception, __VA_ARGS__)

#define JS_Print(value) CALL(JSValuePrint, value)

typedef struct __NarwhalContext NarwhalContext;
struct __NarwhalContext {
    pthread_mutex_t *mutex;
    JSContextRef context;
};

extern NarwhalContext narwhal_context;

#define LOCK() \
    {DEBUG("locking %p (%lu)\n", narwhal_context.mutex, (unsigned long)pthread_self()) \
    int ret = pthread_mutex_lock(narwhal_context.mutex); \
    if (ret) fprintf(stderr, "pthread_mutex_lock error: %d", ret); \
    else {DEBUG("locked %p (%lu)\n", narwhal_context.mutex, (unsigned long)pthread_self())} };

#define UNLOCK() \
    {DEBUG("unlocking %p (%lu)\n", narwhal_context.mutex, (unsigned long)pthread_self()) \
    int ret = pthread_mutex_unlock(narwhal_context.mutex); \
    if (ret) fprintf(stderr, "pthread_mutex_unlock error: %d", ret); \
    else {DEBUG("unlocked %p (%lu)\n", narwhal_context.mutex, (unsigned long)pthread_self())}};

JSClassRef Custom_class(JSContextRef _context)
{
    static JSClassRef jsClass;
    if (!jsClass)
    {
        JSClassDefinition definition = kJSClassDefinitionEmpty;
        jsClass = JSClassCreate(&definition);
    }
    return jsClass;
}


extern JSObjectRef Require;
extern JSObjectRef Exports;
extern JSObjectRef Module;
extern JSObjectRef System;
extern JSObjectRef Print;
extern JSContextRef _context;
extern NarwhalContext narwhal_context;
extern void print(const char * string);
extern JSObjectRef require(const char *id);

#endif

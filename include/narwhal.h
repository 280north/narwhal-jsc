#ifndef __NARWHAL__
#define __NARWHAL__

#include <JavaScriptCore/JavaScriptCore.h>

#define NOTRACE

#ifdef NOTRACE
#define TRACE(...)
#else
#define TRACE(...) fprintf(stderr, __VA_ARGS__);
#endif

#define DEBUG(...)
//#define DEBUG(...) fprintf(stderr, __VA_ARGS__);

//#define THROW_DEBUG
#define THROW_DEBUG " (%s:%d)\n"

#define THROW(format, ...) \
    { char msg[1024]; snprintf(msg, 1024, format THROW_DEBUG, ##__VA_ARGS__, __FILE__, __LINE__); \
    DEBUG("THROWING: %s", msg); \
    *_exception = JSValueMakeStringWithUTF8CString(context, msg); \
    return NULL;/*JSValueMakeUndefined(context);*/ }


JSValueRef JSValueMakeStringWithUTF8CString(JSContextRef ctx, const char *string)
{
    JSStringRef stringRef = JSStringCreateWithUTF8CString(string);
    if (!stringRef)
        return NULL;
    JSValueRef valueRef = JSValueMakeString(ctx, stringRef);
    JSStringRelease(stringRef);
    return valueRef;
}

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#define THIS _this
#define JS_GLOBAL JSContextGetGlobalObject(context)

#define ARGC _argc
#define ARGV(index) _argv[index]
#define ARG_COUNT(c) if ( ARGC != c ) { THROW("Insufficient arguments"); }

#define IS_NUMBER(value)    JSValueIsNumber(context, value)
#define IS_OBJECT(value)    JSValueIsObject(context, value)
#define IS_FUNCTION(value)  JSObjectIsFunction(context, value)
#define IS_STRING(value)    JSValueIsString(context, value)

#define ARGN_INT(variable, index) \
    if (index >= ARGC || !IS_NUMBER(ARGV(index))) THROW("Argument %d must be a number.", index) \
    int variable = (int)JSValueToNumber(context, ARGV(index), _exception); \
    if (*_exception) { return NULL; };
    
#define ARGN_DOUBLE(variable, index) \
    if (index >= ARGC || !IS_NUMBER(ARGV(index))) THROW("Argument %d must be a number.", index) \
    double variable = JSValueToNumber(context, ARGV(index), _exception); \
    if (*_exception) { return NULL; };
    
#define ARGN_OBJ(variable, index) \
    if (index >= ARGC || !IS_OBJECT(ARGV(index))) THROW("Argument %d must be an object.", index) \
    JSObjectRef variable = JSValueToObject(context, ARGV(index), _exception); \
    if (*_exception) { return NULL; };
    
#define ARGN_FN(variable, index) \
    if (!IS_FUNCTION(variable)) { THROW("Argument %d must be a function.", index); } \
    ARGN_OBJ(variable, index); \

#define ARGN_STR(variable, index) \
    if (index >= ARGC || !IS_STRING(ARGV(index))) THROW("Argument %d must be a string.", index) \
    JSStringRef variable = JSValueToStringCopy(context, ARGV(index), _exception);\
    if (*_exception) { return NULL; }

#define ARGN_UTF8_CAST(variable, index) \
    if (index >= ARGC) THROW("Argument %d must be a string.", index) \
    {_tmpStr = JSValueToStringCopy(context, ARGV(index), _exception);\
    if (*_exception) { return NULL; }\
    _tmpSz = JSStringGetMaximumUTF8CStringSize(_tmpStr); }\
    char variable[_tmpSz]; \
    {JSStringGetUTF8CString(_tmpStr, variable, _tmpSz);\
    JSStringRelease(_tmpStr);}

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

#define JS_undefined    JSValueMakeUndefined(context)
#define JS_null         JSValueMakeNull(context)
#define JS_int(number)  JSValueMakeNumber(context, (double)number)
#define JS_bool(b)      JSValueMakeBoolean(context, (int)b)
#define JS_str_utf8(str, len) JSValueMakeStringWithUTF8CString(context, str)

#define JS_obj(value)   JSValueToObject(context, value, _exception)
#define JS_fn(f)        JSObjectMakeFunctionWithCallback(context, NULL, f)

JSValueRef _GET_VALUE(JSContextRef context, JSObjectRef object, const char *name, JSValueRef *_exception) {
    JSStringRef nameStr = JSStringCreateWithUTF8CString(name);
    JSValueRef value = JSObjectGetProperty(context, object, nameStr, _exception);
    JSStringRelease(nameStr);
    return value;
}

#define GET_VALUE(object, name) \
    _GET_VALUE(context, object, name, _exception)
//    JSObjectGetProperty(context, object, JSStringCreateWithUTF8CString(name), _exception) 

#define SET_VALUE(object, name, value) \
    {JSStringRef nameStr = JSStringCreateWithUTF8CString(name); \
    JSObjectSetProperty(context, object, nameStr, value, kJSPropertyAttributeNone, _exception); \
    JSStringRelease(nameStr); \
    if (*_exception) { return NULL; }}

#define GET_OBJECT(object, name) \
    JSValueToObject(context, GET_VALUE(object, name), _exception)

#define GET_INT(object, name) \
    ((int)JSValueToNumber(context, GET_VALUE(object, name), _exception))

#define GET_BOOL(object, name) \
    JSValueToBoolean(context, GET_VALUE(object, name))

#define EXPORTS(name, object) SET_VALUE(Exports, name, object);

#define FUNC_HEADER(f) \
    JSValueRef f( \
    JSContextRef ctx, \
    JSObjectRef function, \
    JSObjectRef _this, \
    size_t _argc, \
    const JSValueRef _argv[], \
    JSValueRef *_exception) \

#define FUNCTION(f,...) \
    FUNC_HEADER(f) \
    { \
        TRACE(" *** F: %s\n", STRINGIZE(f)) \
        JSContextRef context = ctx; \
        size_t _tmpSz; \
        JSStringRef _tmpStr; \
        int _argn = 0; \
        __VA_ARGS__; \
        
#define CONSTRUCTOR(f,...) \
    JSObjectRef f( \
        JSContextRef context, \
        JSObjectRef constructor, \
        size_t _argc, \
        const JSValueRef _argv[], \
        JSValueRef *_exception) \
    { \
        TRACE(" *** C: %s\n", STRINGIZE(f)) \
        size_t _tmpSz; \
        JSStringRef _tmpStr; \
        int _argn = 0; \
        __VA_ARGS__; \

#define END \
    }\

#define NARWHAL_MODULE(MODULE_NAME) \
    JSObjectRef Require; \
    JSObjectRef Exports; \
    JSObjectRef Module; \
    JSObjectRef System; \
    JSObjectRef Print; \
    JSContextRef context; \
    \
    void print(const char * string)\
    {\
        JSValueRef argv[] = { JS_str_utf8(string, strlen(string)) }; \
        JSObjectCallAsFunction(context, Print, NULL, 1, argv, NULL); \
    }\
    JSObjectRef require(const char *id) {\
        JSValueRef argv[] = { JS_str_utf8(id, strlen(id)) }; \
        return JSValueToObject(context, JSObjectCallAsFunction(context, Require, NULL, 1, argv, NULL), NULL); \
    }\
    \
    const char *moduleName = STRINGIZE(MODULE_NAME);\
    extern "C" const char * getModuleName() { return moduleName; }\
    \
    extern "C" FUNC_HEADER(MODULE_NAME) \
        { context = ctx; \
        ARG_COUNT(5); \
        Require = JSValueToObject(context, ARGV(0), _exception); \
        if (*_exception) { return NULL; }; \
        if (!JSObjectIsFunction(context, Require)) { THROW("Argument 0 must be a function."); } \
        Exports = JSValueToObject(context, ARGV(1), _exception); \
        if (*_exception) { return NULL; }; \
        Module = JSValueToObject(context, ARGV(2), _exception); \
        if (*_exception) { return NULL; }; \
        System = JSValueToObject(context, ARGV(3), _exception); \
        if (*_exception) { return NULL; }; \
        Print = JSValueToObject(context, ARGV(4), _exception); \
        if (*_exception) { return NULL; }; \
        if (!JSObjectIsFunction(context, Print)) { THROW("Argument 4 must be a function."); } \

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
    JSObjectCallAsFunction(context, object, thisObject, argc, argv, _exception)
    
#define CALL_AS_CONSTRUCTOR(object, argc, argv) \
    JSObjectCallAsConstructor(context, object, argc, argv, _exception)


#endif

#ifndef __NARWHAL__
#define __NARWHAL__

#include <JavaScriptCore/JavaScriptCore.h>

#define NOTRACE

#ifdef NOTRACE
#define TRACE(...)
#else
#define TRACE(...) printf(__VA_ARGS__);
#endif


JSValueRef JSValueMakeStringWithUTF8CString(JSContextRef ctx, const char *string)
{
    JSStringRef stringRef = JSStringCreateWithUTF8CString(string);
    if (!stringRef)
        printf("OH NO!!!!\n");
    JSValueRef valueRef = JSValueMakeString(ctx, stringRef);
    JSStringRelease(stringRef);
    return valueRef;
}

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#define THIS _this

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

#define ARGN_UTF8(variable, index) \
    if (index >= ARGC || !IS_STRING(ARGV(index))) THROW("Argument %d must be a string.", index) \
    char *variable = NULL;\
    {JSStringRef jsStr = JSValueToStringCopy(context, ARGV(index), _exception);\
    if (*_exception) { return NULL; }\
    size_t len = JSStringGetMaximumUTF8CStringSize(jsStr);\
    variable = (char*)malloc(len);\
    if (!variable) { JSStringRelease(jsStr); THROW("OOM"); }\
    JSStringGetUTF8CString(jsStr, variable, len);\
    JSStringRelease(jsStr);}

#define ARG_INT(variable)  0; ARGN_INT(variable, _argn); _argn++; 0
#define ARG_OBJ(variable)  0; ARGN_OBJ(variable, _argn); _argn++; 0
#define ARG_FN(variable)   0; ARGN_FN(variable, _argn); _argn++; 0
#define ARG_STR(variable)  0; ARGN_STR(variable, _argn); _argn++; 0
#define ARG_UTF8(variable) 0; ARGN_UTF8(variable, _argn); _argn++; 0

#define JS_undefined    JSValueMakeUndefined(context)
#define JS_null         JSValueMakeNull(context)
#define JS_int(number)  JSValueMakeNumber(context, (double)number)
#define JS_bool(b)      JSValueMakeNumber(context, (int)b)
#define JS_str_utf8(str, len) JSValueMakeStringWithUTF8CString(context, str)

#define JS_obj(value)   JSValueToObject(context, value, _exception)


#define JS_fn(f)        JSObjectMakeFunctionWithCallback(context, NULL, f)
#define JS_str_ref(str) JSStringCreateWithUTF8CString(str)

#define GET_VALUE(object, name) \
    JSObjectGetProperty(context, object, JS_str_ref(name), _exception) 

#define SET_VALUE(object, name, value) \
    {JSObjectSetProperty(context, object, JS_str_ref(name), value, kJSPropertyAttributeNone, _exception); \
    if (*_exception) { return NULL; }}

#define GET_OBJECT(object, name) \
    JSValueToObject(context, GET_VALUE(object, name), _exception)

#define GET_INT(object, name) \
    ((int)JSValueToNumber(context, GET_VALUE(object, name), _exception))

#define GET_BOOL(object, name) \
    JSValueToBoolean(context, GET_VALUE(object, name))

#define EXPORTS(name, object) SET_VALUE(Exports, name, object);

#define FUNC_HEADER(f) JSValueRef f( \
    JSContextRef ctx, \
    JSObjectRef function, \
    JSObjectRef _this, \
    size_t _argc, \
    const JSValueRef _argv[], \
    JSValueRef *_exception) \

#define FUNCTION(f,...) FUNC_HEADER(f) \
    { \
        TRACE(" *** F: %s\n", STRINGIZE(f)) \
        JSContextRef context = ctx; \
        int _argn = 0; \
        __VA_ARGS__; \
        
#define CONSTRUCTOR(f,...) JSObjectRef f( \
    JSContextRef context, \
    JSObjectRef constructor, \
    size_t _argc, \
    const JSValueRef _argv[], \
    JSValueRef *_exception) \
    { \
        TRACE(" *** C: %s\n", STRINGIZE(f)) \
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

#define THROW(...) \
    { char msg[1024]; snprintf(msg, 1024, __VA_ARGS__); \
    *_exception = JSValueMakeStringWithUTF8CString(context, msg); \
    return NULL;/*JSValueMakeUndefined(context);*/ }


#define ARGS_ARRAY(name, ...) JSValueRef name[] = { __VA_ARGS__ };

#define CALL_AS_FUNCTION(object, thisObject, argc, argv) \
    JSObjectCallAsFunction(context, object, thisObject, argc, argv, _exception)
    
#define CALL_AS_CONSTRUCTOR(object, argc, argv) \
    JSObjectCallAsConstructor(context, object, argc, argv, _exception)


#endif

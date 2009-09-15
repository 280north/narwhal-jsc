#ifndef __NARWHAL__
#define __NARWHAL__

#include <JavaScriptCore/JavaScriptCore.h>

JSValueRef JSValueMakeStringWithUTF8CString(JSContextRef ctx, const char *string)
{
    JSStringRef stringRef = JSStringCreateWithUTF8CString(string);
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

#define ARGN_INT(variable, index) \
    int variable = (int)JSValueToNumber(context, ARGV(index), _exception); \
    if (*_exception) { return JS_undefined; };

#define ARGN_OBJ(variable, index) \
    JSObjectRef variable = JSValueToObject(context, ARGV(index), _exception); \
    if (*_exception) { return JS_undefined; };
    
#define ARGN_FN(variable, index) \
    ARGN_OBJ(variable, index); \
    if (!JSObjectIsFunction(context, variable)) { THROW("Argument index must be a function."); }

#define ARGN_STR(variable, index) \
    JSStringRef variable = JSValueToStringCopy(context, ARGV(index), _exception);\
    if (*_exception) { return JS_undefined; }

#define ARGN_UTF8(variable, index) \
    char *variable = NULL;\
    {JSStringRef jsStr = JSValueToStringCopy(context, ARGV(index), _exception);\
    if (*_exception) { return JS_undefined; }\
    size_t len = JSStringGetMaximumUTF8CStringSize(jsStr);\
    variable = (char*)malloc(len);\
    if (!variable) { JSStringRelease(jsStr); THROW("OOM"); }\
    JSStringGetUTF8CString(jsStr, variable, len);\
    JSStringRelease(jsStr);}

#define ARG_INT(variable)  0; ARGN_INT(variable, _argn++); 0
#define ARG_OBJ(variable)  0; ARGN_OBJ(variable, _argn++); 0
#define ARG_FN(variable)   0; ARGN_FNvariable, _argn++); 0
#define ARG_STR(variable)  0; ARGN_STR(variable, _argn++); 0
#define ARG_UTF8(variable) 0; ARGN_UTF8(variable, _argn++); 0

#define JS_undefined    JSValueMakeUndefined(context)
#define JS_null         JSValueMakeNull(context)
#define JS_int(number)  JSValueMakeNumber(context, (double)number)
#define JS_str_utf8(str, len) JSValueMakeStringWithUTF8CString(context, str)

#define JS_fn(f)        JSObjectMakeFunctionWithCallback(context, NULL, f)
#define JS_str_ref(str) JSStringCreateWithUTF8CString(str)

#define OBJECT_GET(object, name) \
    JSObjectGetProperty(context, object, JS_str_ref(name), _exception) 

#define OBJECT_SET(object, name, value) \
    {JSObjectSetProperty(context, object, JS_str_ref(name), value, kJSPropertyAttributeNone, _exception); \
    if (*_exception) { return JS_undefined; }}

#define EXPORTS(name, object) OBJECT_SET(Exports, name, object);

#define FUNC_HEADER(f) JSValueRef f( \
    JSContextRef ctx, \
    JSObjectRef function, \
    JSObjectRef _this, \
    size_t _argc, \
    const JSValueRef _argv[], \
    JSValueRef *_exception) \
    
#define FUNCTION(f,...) FUNC_HEADER(f) \
    { JSContextRef context = ctx; \
        int _argn = 0; \
        __VA_ARGS__; \

#define CONSTRUCTOR(f) JSObjectRef f( \
    JSContextRef context, \
    JSObjectRef constructor, \
    size_t _argc, \
    const JSValueRef _argv[], \
    JSValueRef* _exception) \
    { \

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
        JSValueRef argv[1] = { JS_str_utf8(string, strlen(string)) }; \
        JSObjectCallAsFunction(context, Print, NULL, 1, argv, NULL); \
    }\
    JSObjectRef require(const char *id) {\
        JSValueRef argv[1] = { JS_str_utf8(id, strlen(id)) }; \
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
        if (*_exception) { return JS_undefined; }; \
        if (!JSObjectIsFunction(context, Require)) { THROW("Argument 0 must be a function."); } \
        Exports = JSValueToObject(context, ARGV(1), _exception); \
        if (*_exception) { return JS_undefined; }; \
        Module = JSValueToObject(context, ARGV(2), _exception); \
        if (*_exception) { return JS_undefined; }; \
        System = JSValueToObject(context, ARGV(3), _exception); \
        if (*_exception) { return JS_undefined; }; \
        Print = JSValueToObject(context, ARGV(4), _exception); \
        if (*_exception) { return JS_undefined; }; \
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
    return JSValueMakeUndefined(context); }

#endif

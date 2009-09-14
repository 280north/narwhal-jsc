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

#define ARGC argumentCount
#define ARG_COUNT(c) if ( ARGC != c ) { THROW("Insufficient arguments"); }

#define THROW(str) \
    {*exception = JSValueMakeStringWithUTF8CString(context, str);\
    return JSValueMakeUndefined(context);}

#define ARG_obj(variable, index) \
    JSObjectRef variable = JSValueToObject(context, arguments[index], exception); \
    if (*exception) { return JS_undefined; };
    
#define ARG_fn(variable, index) \
    ARG_obj(variable, index); \
    if (!JSObjectIsFunction(context, variable)) { THROW("Argument index must be a function."); }
    
#define ARG_int(variable, index) \
    int variable = (int)JSValueToNumber(context, arguments[index], exception); \
    if (*exception) { return JS_undefined; };

#define ARG_utf8(variable, index) \
    char *variable = NULL;\
    {JSStringRef jsStr = JSValueToStringCopy(context, arguments[index], exception);\
    if (*exception) { return JS_undefined; }\
    size_t len = JSStringGetMaximumUTF8CStringSize(jsStr);\
    variable = (char*)malloc(len);\
    if (!variable) { JSStringRelease(jsStr); THROW("OOM"); }\
    JSStringGetUTF8CString(jsStr, variable, len);\
    JSStringRelease(jsStr);}

#define JS_fn(f)        JSObjectMakeFunctionWithCallback(context, NULL, f)
#define JS_undefined    JSValueMakeUndefined(context)
#define JS_null         JSValueMakeNull(context)
#define JS_str_ref(str) JSStringCreateWithUTF8CString(str)
#define JS_str_val(str) JSValueMakeStringWithUTF8CString(context, str)

#define OBJECT_SET(object, name, value) \
    {JSObjectSetProperty(context, object, JS_str_ref(name), value, kJSPropertyAttributeNone, exception); \
    if (*exception) { return JS_undefined; }}

#define EXPORTS(name, object) OBJECT_SET(Exports, name, object);

#define FUNC_HEADER(f) JSValueRef f( \
    JSContextRef ctx, \
    JSObjectRef function, \
    JSObjectRef thisObject, \
    size_t argumentCount, \
    const JSValueRef arguments[], \
    JSValueRef *exception) \
    
#define FUNCTION(f) FUNC_HEADER(f) \
    { JSContextRef context = ctx; \

#define CONSTRUCTOR(f) JSObjectRef f( \
    JSContextRef context, \
    JSObjectRef constructor, \
    size_t argc, \
    const JSValueRef argv[], \
    JSValueRef* exception) \
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
        JSValueRef argv[1] = { JS_str_val(string) }; \
        JSObjectCallAsFunction(context, Print, NULL, 1, argv, NULL); \
    }\
    JSObjectRef require(const char *id) {\
        JSValueRef argv[1] = { JS_str_val(id) }; \
        return JSValueToObject(context, JSObjectCallAsFunction(context, Require, NULL, 1, argv, NULL), NULL); \
    }\
    \
    const char *moduleName = STRINGIZE(MODULE_NAME);\
    extern "C" const char * getModuleName() { return moduleName; }\
    \
    extern "C" FUNC_HEADER(MODULE_NAME) \
        { context = ctx; \
        ARG_COUNT(5); \
        Require = JSValueToObject(context, arguments[0], exception); \
        if (*exception) { return JS_undefined; }; \
        if (!JSObjectIsFunction(context, Require)) { THROW("Argument 0 must be a function."); } \
        Exports = JSValueToObject(context, arguments[1], exception); \
        if (*exception) { return JS_undefined; }; \
        Module = JSValueToObject(context, arguments[2], exception); \
        if (*exception) { return JS_undefined; }; \
        System = JSValueToObject(context, arguments[3], exception); \
        if (*exception) { return JS_undefined; }; \
        Print = JSValueToObject(context, arguments[4], exception); \
        if (*exception) { return JS_undefined; }; \
        if (!JSObjectIsFunction(context, Print)) { THROW("Argument 4 must be a function."); } \

#define END_NARWHAL_MODULE \
    return JS_undefined; }\

#endif

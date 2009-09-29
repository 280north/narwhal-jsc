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

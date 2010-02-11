#include <narwhal.h>

// typedef struct __ContextPrivate {
//     JSContextRef context;
// } ContextPrivate;
// 
// DESTRUCTOR(Context_finalize)
// {    
//     GET_INTERNAL(ContextPrivate *, data, object);
//     DEBUG("freeing Context=[%p]\n", data);
//     if (data) {
//         // free data->context
//         free(data);
//     }
// }
// END
// 
// JSClassRef Context_class(JSContextRef _context)
// {
//     static JSClassRef jsClass;
//     if (!jsClass)
//     {
//         JSClassDefinition definition = kJSClassDefinitionEmpty;
//         definition.finalize = Context_finalize;
// 
//         jsClass = JSClassCreate(&definition);
//     }
//     return jsClass;
// }
// 
// JSObjectRef Context_new(JSContextRef _context, JSValueRef *_exception)
// {
//     ContextPrivate *data = (ContextPrivate*)malloc(sizeof(ContextPrivate));
//     if (!data) return NULL;
//     
//     data->context = JSGlobalContextCreate(NULL);
//     
//     return JSObjectMake(_context, Context_class(_context), data);
// }

FUNCTION(INTERPRETER_JSGlobalContextCreate)
{
    JSGlobalContextRef new_context;

    if (JSGlobalContextCreateInGroup)
        new_context = JSGlobalContextCreateInGroup(JSContextGetGroup(_context), NULL);
    else
        new_context = JSGlobalContextCreate(NULL);

    return CALL(Context_new, new_context);
}
END

FUNCTION(INTERPRETER_JSContextGetGlobalObject, ARG_OBJ(context))
{
    GET_INTERNAL(ContextPrivate*, data, context);
    JSObjectRef global = JSContextGetGlobalObject(data->context);
    return global;
}
END

FUNCTION(INTERPRETER_JSEvaluateScript, ARG_OBJ(context), ARG_STR(source), ARG_OBJ(global), ARG_STR(sourceURL), ARG_INT(lineNumber))
{
    GET_INTERNAL(ContextPrivate*, data, context);
    
    // cleanup this exception handling
    HANDLE_EXCEPTION(true,true);
    JSStringRef sourceStr = JSValueToStringCopy(_context, source, _exception);
    HANDLE_EXCEPTION(true,true);
    JSStringRef sourceURLStr = JSValueToStringCopy(_context, sourceURL, _exception);
    HANDLE_EXCEPTION(true,true);
    JSValueRef result = JSEvaluateScript(
        data->context,
        sourceStr,
        global,
        sourceURLStr,
        lineNumber,
        _exception
    );
    HANDLE_EXCEPTION(true,true);
    
    JSStringRelease(sourceStr);
    JSStringRelease(sourceURLStr);
    
    return result;
}
END

NARWHAL_MODULE(interpreter)
{
    EXPORTS("JSGlobalContextCreate", JS_fn(INTERPRETER_JSGlobalContextCreate));
    EXPORTS("JSContextGetGlobalObject", JS_fn(INTERPRETER_JSContextGetGlobalObject));
    EXPORTS("JSEvaluateScript", JS_fn(INTERPRETER_JSEvaluateScript));
    EXPORTS("initializeNarwhal", JS_fn(NW_inititialize));
    
    NW_require("interpreter.js");
}
END_NARWHAL_MODULE

#include <narwhal.h>
#include <stdlib.h>
#include <unistd.h>

#include <binary-engine.h>

#include "../../../mongoose.h"


typedef JSValueRef (*NWFunction)(JSContextRef, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef *);

typedef struct __MongoosePrivate MongoosePrivate;
struct __MongoosePrivate {
    struct mg_context *mongooseContext;
    JSContextRef jsContext;
    JSObjectRef app;
};

typedef struct __MongooseConnection MongooseConnection;
struct __MongooseConnection {
    struct mg_connection *conn;
};

JSClassRef MongooseContext_class(JSContextRef _context)
{
    static JSClassRef jsClass;
    if (!jsClass)
    {
        JSClassDefinition definition = kJSClassDefinitionEmpty;
        jsClass = JSClassCreate(&definition);
    }
    return jsClass;
}

FUNCTION(Mongoose_writer)
{    
    printf("asdfadsf");
    //return JS_undefined;
///*
    GET_INTERNAL(MongooseConnection*, data, THIS);
    if (!data)
        THROW("Problem writing in Mongoose");
    
    // FIXME: this is failing on legit objects for some reason
    //if (ARGC < 1 || !IS_OBJECT(ARGV(0)))
    //    THROW("First argument to writer must be an object which has a toByteString method");
    
    CALL(JSValuePrint, ARGV(0));
    HANDLE_EXCEPTION(true, true);
    
    JSObjectRef chunk = JSValueToObject(_context, ARGV(0), _exception);
    HANDLE_EXCEPTION(true, true);
    
    JSObjectRef toByteString = GET_OBJECT(chunk, "toByteString");
    HANDLE_EXCEPTION(true, true);
    
    if (!IS_FUNCTION(toByteString))
        THROW("First argument to writer must be an object which has a toByteString method");
    
    JSValueRef byteStringValue = JSObjectCallAsFunction(_context, toByteString, chunk, 0, NULL, _exception);
    HANDLE_EXCEPTION(true, true);
    
    if (!IS_OBJECT(byteStringValue))
        THROW("toByteString did not return a ByteString object");
    
    JSObjectRef byteString = JSValueToObject(_context, byteStringValue, _exception);
    HANDLE_EXCEPTION(true, true);
    
    JSObjectRef _bytes = GET_OBJECT(byteString, "_bytes");
    HANDLE_EXCEPTION(true, true);
    
    GET_INTERNAL(BytesPrivate*, bytesData, _bytes);

    mg_write(data->conn, bytesData->buffer, bytesData->length);
    
    return JS_undefined;
//*/
}
END

JSValueRef handlerWrapper(
    JSContextRef _context, JSValueRef *_exception,
    struct mg_connection *conn, const struct mg_request_info *info, void *user_data
) {    
    MongoosePrivate *data = (MongoosePrivate*)user_data;
    
    JSObjectRef env = JSObjectMake(_context, NULL, NULL);
    char buffer[1024];
    
    printf("status_code=%d\n", info->status_code);
    
    *_exception = NULL;
    /*
    SET_VALUE(env, "SCRIPT_NAME", JS_str_utf8("", 0));
    HANDLE_EXCEPTION(true, true);
    SET_VALUE(env, "PATH_INFO", JS_str_utf8(info->uri, strlen(info->uri)));
    HANDLE_EXCEPTION(true, true);
    SET_VALUE(env, "REQUEST_METHOD", JS_str_utf8(info->request_method, strlen(info->request_method)));
    HANDLE_EXCEPTION(true, true);
    
    SET_VALUE(env, "QUERY_STRING", JS_str_utf8(info->query_string, strlen(info->query_string)));
    HANDLE_EXCEPTION(true, true);
    SET_VALUE(env, "REMOTE_USER", JS_str_utf8(info->remote_user, strlen(info->remote_user)));
    HANDLE_EXCEPTION(true, true);
    
    SET_VALUE(env, "SERVER_NAME", JS_str_utf8("", 0));
    HANDLE_EXCEPTION(true, true);
    SET_VALUE(env, "SERVER_PORT", JS_str_utf8("", 0));
    HANDLE_EXCEPTION(true, true);
    
    snprintf(buffer, sizeof(buffer), "HTTP/%s", info->http_version);
    SET_VALUE(env, "HTTP_VERSION", JS_str_utf8(buffer, strlen(buffer)));
    HANDLE_EXCEPTION(true, true);
    
    long ip = info->remote_ip;
    snprintf(buffer, sizeof(buffer), "%hu.%hu.%hu.%hu", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
    SET_VALUE(env, "REMOTE_ADDRESS", JS_str_utf8(buffer, strlen(buffer)));
    HANDLE_EXCEPTION(true, true);
    
    //SET_VALUE(env, "jsgi.version",      );
    //SET_VALUE(env, "jsgi.input",        );
    //SET_VALUE(env, "jsgi.errors",       );
    SET_VALUE(env, "jsgi.multithread",  JS_bool(1));
    HANDLE_EXCEPTION(true, true);
    SET_VALUE(env, "jsgi.multiprocess", JS_bool(1));
    HANDLE_EXCEPTION(true, true);
    SET_VALUE(env, "jsgi.run_once",     JS_bool(0));
    HANDLE_EXCEPTION(true, true);
    SET_VALUE(env, "jsgi.url_scheme",   JS_str_utf8("http", strlen("http")));
    HANDLE_EXCEPTION(true, true);
    
    // headers
    snprintf(buffer, sizeof(buffer), "HTTP_");
    for (int i = 0, num_headers = info->num_headers; i < num_headers; i++) {
        // convert to CGI style keys
        char c, *dst = buffer + 5, *src = info->http_headers[i].name;
        while (c = *(src++))
            *(dst++) = (c == '-') ? '_' : toupper(c);
        *dst = '\0';
        
        char *value = info->http_headers[i].value;
        SET_VALUE(env, buffer, JS_str_utf8(value, strlen(value)));
        HANDLE_EXCEPTION(true, true);
    }
    */
    
    *_exception = NULL;
    //JSValueRef argv[1] = { env };
    JSValueRef res = JSObjectCallAsFunction(_context, data->app, NULL, 0, NULL, _exception);
    
    //CALL_AS_FUNCTION(data->app, NULL, 0, NULL);
    if (*_exception) {
        JSValuePrint(_context, NULL, *_exception);
        mg_printf(conn, "%s %s\r\n", "HTTP/1.1", "500 Internal Server Error");
        mg_printf(conn, "%s: %s\r\n", "Content-Type", "text/plain");
        mg_printf(conn, "%s: %s\r\n", "Connection", "close");
        mg_printf(conn, "\r\n");
        mg_printf(conn, "%s", "500 Internal Server Error");
        return NULL;
    }
    
    JSObjectRef result = JSValueToObject(_context, res, _exception);
    HANDLE_EXCEPTION(true, true);
    
    // STATUS:
    int status = GET_INT(result, "status");
    mg_printf(conn, "%s %d\r\n", "HTTP/1.1", status);
    
    // HEADERS:
    JSObjectRef headers = GET_OBJECT(result, "headers");
    JSPropertyNameArrayRef headerNames = JSObjectCopyPropertyNames(_context, headers);
    size_t headerCount = JSPropertyNameArrayGetCount(headerNames);
    
    for (size_t i = 0; i < headerCount; i++) {
        JSStringRef headerName = JSPropertyNameArrayGetNameAtIndex(headerNames, i);
        
        size_t sz = JSStringGetMaximumUTF8CStringSize(headerName);
        char name[sz];
        JSStringGetUTF8CString(headerName, name, sz);
        
        JSStringRef headerValue = JSValueToStringCopy(_context, GET_VALUE(headers, name), _exception);
        
        sz = JSStringGetMaximumUTF8CStringSize(headerValue);
        char value[sz];
        JSStringGetUTF8CString(headerValue, value, sz);
        
        char *start = value, *end = value;
        for (;;) {
            while (*end != '\n' && *end != '\0') end++;
            char orig = *end;
            
            *end = '\0';
            mg_printf(conn, "%s: %s\r\n", name, start);
            *end = orig;

            if (orig == '\0') break;
            end = start = end + 1;
        }
    }
    
    mg_printf(conn, "\r\n");
    
    // BODY:
    JSObjectRef body = GET_OBJECT(result, "body");
    HANDLE_EXCEPTION(true, true);
    JSObjectRef forEach = GET_OBJECT(body, "forEach");
    //printf("forEach=%p\n", forEach);
    HANDLE_EXCEPTION(true, true);
    
    JSObjectRef writer = JSObjectMakeFunctionWithCallback(_context, NULL, Mongoose_writer);
    
    MongooseConnection conn_data;
    conn_data.conn = conn;
    JSObjectRef that = JSObjectMake(_context, MongooseContext_class(_context), &conn_data);
    //SET_INTERNAL(that, data);
    
    JSObjectRef binder = GET_OBJECT(writer, "bind");
    HANDLE_EXCEPTION(true, true);
    
    //JSGarbageCollect(_context);
    *_exception = NULL;
    JSValueRef argv1[1] = { that };
    JSValueRef boundWriter = JSObjectCallAsFunction(_context, binder, writer, 1, argv1, _exception);
    HANDLE_EXCEPTION(true, true);
    
    *_exception = NULL;
    //JSValueRef argv2[1] = { writer };
    JSValueRef argv2[1] = { boundWriter };
    //printf("%p %p %p %p %p %p\n", _context, forEach, body, argv2, _exception, *_exception);
    JSObjectCallAsFunction(_context, forEach, body, 1, argv2, _exception);
    HANDLE_EXCEPTION(true, true);
}
    
void handler(struct mg_connection *conn, const struct mg_request_info *info, void *user_data) {
    LOCK();
    
    MongoosePrivate *data = (MongoosePrivate*)user_data;
    
    JSContextRef _context = data->jsContext;
    JSValueRef exception = NULL;
    JSValueRef *_exception = &exception;
    
    JSValueRef result = CALL(handlerWrapper, conn, info, user_data);
    
    if (*_exception)
        JSValuePrint(_context, NULL, *_exception);
    
    UNLOCK();
}

FUNCTION(MG_run, ARG_FN(app))
{
    int port = 8080;

    if (ARGC > 1 && IS_OBJECT(ARGV(1))) {
        JSObjectRef options = TO_OBJECT(ARGV(1));
        port = GET_INT(options, "port");
        if (*_exception) {
            JS_Print(*_exception);
            *_exception = NULL;
        }
    }
    fprintf(stderr, "Starting Mongoose on port %d...", port);
    
    MongoosePrivate *data = (MongoosePrivate*)malloc(sizeof(MongoosePrivate));
    printf("%p\n", data);
    
	data->mongooseContext = mg_start();
    data->jsContext = _context;
    data->app = app;
    
    //JSGlobalContextRetain(_context);
    JSValueProtect(_context, app);
	
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%d", port);
	mg_set_option(data->mongooseContext, "ports", buffer);
	mg_set_option(data->mongooseContext, "max_threads", "1");
	
	mg_set_uri_callback(data->mongooseContext, "/*", &handler, (void*)data);
    
    JSObjectRef mgCtx = JSObjectMake(_context, NULL, data);
    
    JSObjectCallAsFunction(_context, app, NULL, 0, NULL, _exception);
    JSObjectCallAsFunction(_context, app, NULL, 0, NULL, _exception);
    JSObjectCallAsFunction(_context, app, NULL, 0, NULL, _exception);
    
    return mgCtx;
}
END

FUNCTION(MG_stop, ARG_OBJ(mgCtx))
{
    GET_INTERNAL(MongoosePrivate *, data, mgCtx);

    printf("%p\n", data);
    if (!data) THROW("Invalid context");

    mg_stop(data->mongooseContext);
}
END

NARWHAL_MODULE(os_engine)
{
    EXPORTS("stop", JS_fn(MG_stop));
    EXPORTS("run", JS_fn(MG_run));
}
END_NARWHAL_MODULE

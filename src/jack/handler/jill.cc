#include <narwhal.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "../../../deps/http-parser/http_parser.h"

#include <binary-engine.h>
#include <io-engine.h>

JSObjectRef ByteIO;
JSObjectRef ByteString;

typedef struct _client_data {
    int     fd;
    size_t  len;
    char    *buf;
    
    JSContextRef _context;
    JSValueRef *_exception;
    
    JSObjectRef env;
    JSObjectRef app;
    
    char    *path;
    ssize_t pathLen;
    
    char    *query;
    ssize_t queryLen;
    
    char    *hName;
    ssize_t hNameLen;
    
    char    *hValue;
    ssize_t hValueLen;
    
    char    *body;
    ssize_t bodyLen;
    
    int     complete;
} client_data;

JSValueRef processHeader(
    JSContextRef _context, JSValueRef *_exception,
    client_data *data
) {
    if (data->hValueLen > 0) {
        JSObjectRef env = data->env;
        
        data->hName[data->hNameLen] = '\0';
        data->hValue[data->hValueLen] = '\0';
        
        char buffer[data->hNameLen + 5];
        memcpy(buffer, "HTTP_", 5);
        char c, *dst = buffer + 5, *src = data->hName;
        while (c = *(src++))
            *(dst++) = (c == '-') ? '_' : toupper(c);
        *dst = '\0';
        
        if (!strcmp("HTTP_CONTENT_LENGTH", buffer) || !strcmp("HTTP_CONTENT_TYPE", buffer)) {
            SET_VALUE(env, buffer + 5, JS_str_utf8(data->hValue, data->hValueLen));
        } else {
            SET_VALUE(env, buffer, JS_str_utf8(data->hValue, data->hValueLen));
        }
        
        data->hName = NULL;
        data->hNameLen = 0;
        data->hValue = NULL;
        data->hValueLen = 0;
    }
    
    return NULL;
}

FUNCTION(HS_writer)
{
    GET_INTERNAL(client_data*, data, THIS);
    if (!data)
        THROW("Problem writing");
    
    // FIXME: this is failing on legit objects for some reason
    //if (ARGC < 1 || !IS_OBJECT(ARGV(0)))
    //    THROW("First argument to writer must be an object which has a toByteString method");
    
    //CALL(JSValuePrint, ARGV(0));
    //HANDLE_EXCEPTION(true, true);
    
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

    if (send(data->fd, bytesData->buffer, bytesData->length, 0) < 0)
        THROW("Client closed connection");
    
    return JS_undefined;
}
END

JSValueRef handlerWrapper(
    JSContextRef _context, JSValueRef *_exception,
    http_parser *parser
) {
    client_data *data = (client_data *)parser->data;
    JSObjectRef env = data->env;
    
    // add the final header
    CALL(processHeader, data);
    
    // SCRIPT_NAME
    SET_VALUE(env, "SCRIPT_NAME", JS_str_utf8("", 0));
    HANDLE_EXCEPTION(true, true);
    
    // PATH_INFO
    if (data->path) {
        data->path[data->pathLen] = '\0';
        SET_VALUE(env, "PATH_INFO", JS_str_utf8(data->path, data->pathLen));
    } else {
        SET_VALUE(env, "PATH_INFO", JS_str_utf8("", 0));
    }
    HANDLE_EXCEPTION(true, true);
    
    // QUERY_STRING
    if (data->query) {
        data->query[data->queryLen] = '\0';
        SET_VALUE(env, "QUERY_STRING", JS_str_utf8(data->query, data->queryLen));
    } else {
        SET_VALUE(env, "QUERY_STRING", JS_str_utf8("", 0));
    }
    HANDLE_EXCEPTION(true, true);
    
    // REQUEST_METHOD
    char *methodName = "GET";
    switch (parser->method) {
        case HTTP_COPY      : methodName = "COPY"; break;
        case HTTP_DELETE    : methodName = "DELETE"; break;
        case HTTP_HEAD      : methodName = "HEAD"; break;
        case HTTP_LOCK      : methodName = "LOCK"; break;
        case HTTP_MKCOL     : methodName = "MKCOL"; break;
        case HTTP_MOVE      : methodName = "MOVE"; break;
        case HTTP_OPTIONS   : methodName = "OPTIONS"; break;
        case HTTP_POST      : methodName = "POST"; break;
        case HTTP_PROPFIND  : methodName = "PROPFIND"; break;
        case HTTP_PROPPATCH : methodName = "PROPPATCH"; break;
        case HTTP_PUT       : methodName = "PUT"; break;
        case HTTP_TRACE     : methodName = "TRACE"; break;
        case HTTP_UNLOCK    : methodName = "UNLOCK"; break;
        case HTTP_GET       : methodName = "GET"; break;
    }
    SET_VALUE(env, "REQUEST_METHOD", JS_str_utf8(methodName, strlen(methodName)));
    HANDLE_EXCEPTION(true, true);
    
    // SERVER_PROTOCOL
    char *versionName = "HTTP/1.1";
    switch (parser->version) {
        case HTTP_VERSION_09    : versionName = "HTTP/0.9"; break;
        case HTTP_VERSION_10    : versionName = "HTTP/1.0"; break;
        case HTTP_VERSION_11    : versionName = "HTTP/1.1"; break;
    }
    SET_VALUE(env, "SERVER_PROTOCOL", JS_str_utf8(versionName, strlen(versionName)));
    HANDLE_EXCEPTION(true, true);
    
    // SERVER_NAME
    SET_VALUE(env, "SERVER_NAME", JS_str_utf8("", 0));
    HANDLE_EXCEPTION(true, true);
    
    // SERVER_PORT
    SET_VALUE(env, "SERVER_PORT", JS_str_utf8("", 0));
    HANDLE_EXCEPTION(true, true);
    
    // REMOTE_USER
    //SET_VALUE(env, "REMOTE_USER", JS_str_utf8(info->remote_user, strlen(info->remote_user)));
    //HANDLE_EXCEPTION(true, true);
    
    // REMOTE_ADDR
    //long ip = info->remote_ip;
    //snprintf(buffer, sizeof(buffer), "%hu.%hu.%hu.%hu", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
    //SET_VALUE(env, "REMOTE_ADDR", JS_str_utf8(buffer, strlen(buffer)));
    //HANDLE_EXCEPTION(true, true);
    
    // jsgi.version
    ARGS_ARRAY(argvVersion, JS_int(0), JS_int(2));
    SET_VALUE(env, "jsgi.version", JS_array(2, argvVersion));
    HANDLE_EXCEPTION(true, true);
    
    // jsgi.erros
    SET_VALUE(env, "jsgi.errors", GET_OBJECT(System, "stderr"));
    HANDLE_EXCEPTION(true, true);
    
    // jsgi.input
    if (!data->body) {
        data->body = "";
        data->bodyLen = 0;
    }
    
    // TODO: modify parsing to use a separate buffer directly to prevent copying
    char *bodyBuffer = (char *)malloc(data->bodyLen * sizeof(char));
    memcpy(bodyBuffer, data->body, data->bodyLen);
    
    JSObjectRef bytes = CALL(Bytes_new, bodyBuffer, data->bodyLen);
    HANDLE_EXCEPTION(true, true);
    
    // new ByteString(bytes, 0, 0)
    ARGS_ARRAY(byteStringArgs, bytes, JS_int(0), JS_int(data->bodyLen));
    JSObjectRef byteString = CALL_AS_CONSTRUCTOR(ByteString, 3, byteStringArgs);
    
    // new ByteIO(byteString)
    ARGS_ARRAY(byteIoArgs, byteString);
    JSObjectRef input = CALL_AS_CONSTRUCTOR(ByteIO, 1, byteIoArgs);
    HANDLE_EXCEPTION(true, true);
    
    SET_VALUE(env, "jsgi.input", input);
    HANDLE_EXCEPTION(true, true);
    
    // jsgi.multithread
    SET_VALUE(env, "jsgi.multithread",  JS_bool(0));
    HANDLE_EXCEPTION(true, true);
    
    // jsgi.multiprocess
    SET_VALUE(env, "jsgi.multiprocess", JS_bool(1));
    HANDLE_EXCEPTION(true, true);
    
    // jsgi.run_once
    SET_VALUE(env, "jsgi.run_once",     JS_bool(0));
    HANDLE_EXCEPTION(true, true);
    
    // jsgi.url_scheme
    SET_VALUE(env, "jsgi.url_scheme",   JS_str_utf8("http", strlen("http")));
    HANDLE_EXCEPTION(true, true);
    
    // Invoke the application
    JSValueRef argv[1] = { env };
    JSValueRef res = JSObjectCallAsFunction(_context, data->app, NULL, 1, argv, _exception);
    //HANDLE_EXCEPTION(true, true);

    char buffer[1024];
    
    if (*_exception) {
        JSValuePrint(_context, NULL, *_exception);
        snprintf(buffer, sizeof(buffer), "%s %s\r\n", "HTTP/1.1", "500 Internal Server Error");
        if (send(data->fd, buffer, strlen(buffer), 0) < 0)
            THROW("Client closed connection");
        snprintf(buffer, sizeof(buffer), "%s: %s\r\n", "Content-Type", "text/plain");
        if (send(data->fd, buffer, strlen(buffer), 0) < 0)
            THROW("Client closed connection");
        snprintf(buffer, sizeof(buffer), "%s: %s\r\n", "Connection", "close");
        if (send(data->fd, buffer, strlen(buffer), 0) < 0)
            THROW("Client closed connection");
        snprintf(buffer, sizeof(buffer), "\r\n");
        if (send(data->fd, buffer, strlen(buffer), 0) < 0)
            THROW("Client closed connection");
        snprintf(buffer, sizeof(buffer), "%s", "500 Internal Server Error");
        if (send(data->fd, buffer, strlen(buffer), 0) < 0)
            THROW("Client closed connection");
        return NULL;
    }
    
    JSObjectRef result = JSValueToObject(_context, res, _exception);
    HANDLE_EXCEPTION(true, true);
    
    // STATUS:
    int status = GET_INT(result, "status");
    HANDLE_EXCEPTION(true, true);
    
    char *reason = "OK";
    snprintf(buffer, sizeof(buffer), "%s %d %s\r\n", "HTTP/1.1", status, reason);
    if (send(data->fd, buffer, strlen(buffer), 0) < 0)
        THROW("Client closed connection");
    
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
            snprintf(buffer, sizeof(buffer), "%s: %s\r\n", name, start);
            if (send(data->fd, buffer, strlen(buffer), 0) < 0)
                THROW("Client closed connection");
            *end = orig;

            if (orig == '\0') break;
            end = start = end + 1;
        }
    }
    
    snprintf(buffer, sizeof(buffer), "\r\n");
    if (send(data->fd, buffer, strlen(buffer), 0) < 0)
        THROW("Client closed connection");
    
    // BODY:
    JSObjectRef body = GET_OBJECT(result, "body");
    HANDLE_EXCEPTION(true, true);
    JSObjectRef forEach = GET_OBJECT(body, "forEach");
    HANDLE_EXCEPTION(true, true);
    
    JSObjectRef writer = JSObjectMakeFunctionWithCallback(_context, NULL, HS_writer);

    JSObjectRef that = JSObjectMake(_context, Custom_class(_context), data);
    
    JSObjectRef binder = GET_OBJECT(writer, "bind");
    HANDLE_EXCEPTION(true, true);
    
    //JSGarbageCollect(_context);
    *_exception = NULL;
    JSValueRef argv1[1] = { that };
    JSValueRef boundWriter = JSObjectCallAsFunction(_context, binder, writer, 1, argv1, _exception);
    HANDLE_EXCEPTION(true, true);
    
    *_exception = NULL;
    JSValueRef argv2[1] = { boundWriter };
    JSObjectCallAsFunction(_context, forEach, body, 1, argv2, _exception);
    HANDLE_EXCEPTION(true, true);
    
    return 0;
}

void dispatch(http_parser *parser) {
    client_data *data = (client_data *)parser->data;
    JSContextRef _context = data->_context;
    JSValueRef *_exception = data->_exception;

    if (data->complete)
        return;
        
    data->complete = 1;

    DEBUG("Dispatching...\n");

    *_exception = NULL;
    JSValueRef result = handlerWrapper(_context, _exception, parser);
    
    if (*_exception) {
        printf("ERROR:");
        JSValueRef exceptionToPrint = *_exception;
        *_exception = NULL;
        JS_Print(exceptionToPrint);
    }
}

int on_message_begin(http_parser *parser) {
    client_data *data = (client_data *)parser->data;
    JSContextRef _context = data->_context;
    JSValueRef *_exception = data->_exception;
    
    data->env = JSObjectMake(_context, NULL, NULL);
    
    return 0;
}

int on_path(http_parser *parser, const char *at, size_t length) {
    client_data *data = (client_data *)parser->data;
    // JSContextRef _context = data->_context;
    // JSValueRef *_exception = data->_exception;
    
    if (!data->path) {
        data->path = (char *)at;
        data->pathLen = length;
    }
    else {
         data->pathLen += length;
    }

    return 0;
}

int on_query_string(http_parser *parser, const char *at, size_t length) {
    client_data *data = (client_data *)parser->data;
    // JSContextRef _context = data->_context;
    // JSValueRef *_exception = data->_exception;
    
    if (!data->query) {
        data->query = (char *)at;
        data->queryLen = length;
    }
    else {
         data->queryLen += length;
    }
    
    return 0;
}

int on_header_field(http_parser *parser, const char *at, size_t length) {
    client_data *data = (client_data *)parser->data;
    JSContextRef _context = data->_context;
    JSValueRef *_exception = data->_exception;
    
    // add header, if there's one pending
    CALL(processHeader, data);
    
    if (!data->hName) {
        data->hName = (char *)at;
        data->hNameLen = length;
    }
    else {
         data->hNameLen += length;
    }
    
    return 0;
}

int on_header_value(http_parser *parser, const char *at, size_t length) {
    client_data *data = (client_data *)parser->data;
    //JSContextRef _context = data->_context;
    //JSValueRef *_exception = data->_exception;

    if (!data->hValue) {
        data->hValue = (char *)at;
        data->hValueLen = length;
    }
    else {
        data->hValueLen += length;
    }
    
    return 0;
}

int on_headers_complete(http_parser *parser) {
    //client_data *data = (client_data *)parser->data;
    //JSContextRef _context = data->_context;
    //JSValueRef *_exception = data->_exception;
    
    //printf("on_headers_complete\n");
    
    // headers done but no content-length, assume no body
    // TODO: is this the desired behavior?
    if (parser->content_length < 0)
        dispatch(parser);
    
    return 0;
}

int on_body(http_parser *parser, const char *at, size_t length) {
    client_data *data = (client_data *)parser->data;
    //JSContextRef _context = data->_context;
    //JSValueRef *_exception = data->_exception;
    
    //printf("on_body: %d\n", length);
    
    if (!data->body) {
        data->body = (char *)at;
        data->bodyLen = length;
    }
    else {
         data->bodyLen += length;
    }
    
    return 0;
}

int on_message_complete(http_parser *parser) {
    //client_data *data = (client_data *)parser->data;
    //JSContextRef _context = data->_context;
    //JSValueRef *_exception = data->_exception;
    
    //printf("on_message_complete\n");
    
    dispatch(parser);
    
    return 0;
}

FUNCTION(HS_run, ARG_FN(app))
{
    int port = 8080;
    int backlog = 100;
    
    if (ARGC > 1 && IS_OBJECT(ARGV(1))) {
        JSObjectRef options = JS_obj(ARGV(1));
        port = GET_INT(options, "port");
        if (*_exception) {
            JS_Print(*_exception);
            *_exception = NULL;
        }
    }
    fprintf(stderr, "Starting Jill on port %d...", port);
    
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
        THROW("socket error: %s", strerror(errno));
    
    int optval = 1;
    // free up the bound port immediately
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        THROW("bind error: %s", strerror(errno));
    
    if (listen(server_socket, backlog) < 0)
        THROW("listen error: %s", strerror(errno));
    
    size_t bufferSize = 1024;
    size_t bufferPosition = 0;
    char *buffer = (char *)malloc(bufferSize);
    
    while (1) {
        socklen_t client_addrlen = sizeof(struct sockaddr_in);
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_addrlen);
        if (client_socket < 0) {
            // TODO: should we really throw an exception here, or do something else?
            close(server_socket);
            THROW("accept error: %s", strerror(errno));
        }
        
        int optval = 1;
        // don't signal SIGPIPE if a socket is closed early
        setsockopt(client_socket, SOL_SOCKET, SO_NOSIGPIPE, (void *)&optval, sizeof(optval));
        
        client_data data;
        memset(&data, 0, sizeof(data));
        
        data.fd = client_socket;
        data._context = _context;
        JSValueRef exception = NULL;
        data._exception = &exception;
        data.app = app;
        
        http_parser parser;
        http_parser_init(&parser, HTTP_REQUEST);
        
        parser.data = &data;
        
        parser.on_message_begin    = on_message_begin;
        parser.on_path             = on_path;
        parser.on_query_string     = on_query_string;
        parser.on_header_field     = on_header_field;
        parser.on_header_value     = on_header_value;
        parser.on_headers_complete = on_headers_complete;
        parser.on_body             = on_body;
        parser.on_message_complete = on_message_complete;
        
        bufferPosition = 0;
        
        while (1) {
            
            if (bufferPosition >= bufferSize) {
                if (bufferPosition > bufferSize) {
                    fprintf(stderr, "bufferPosition=%d bufferSize=%d WHAT?!\n", bufferPosition, bufferSize);
                }
                
                bufferSize *= 2;
                
                fprintf(stderr, "reallocing bufferSize=%d\n", bufferSize);
                
                buffer = (char *)realloc(buffer, bufferSize);
                if (!buffer) {
                    THROW("OOM!");
                    close(client_socket);
                }
            }
            
            // FIXME: timeout
            ssize_t recved = recv(client_socket, buffer + bufferPosition, bufferSize - bufferPosition, 0);
            if (recved < 0) {
                printf("recv error: %s\n", strerror(errno));
                break;
            }

            http_parser_execute(&parser, buffer + bufferPosition, recved);

            if (http_parser_has_error(&parser)) {
                printf("parse error\n");
                if (*data._exception) {
                    JS_Print(*data._exception);
                }
                break;
            }
            
            //printf("data.complete=%d recved=%d\n", data.complete, recved);
            
            if (data.complete) {
                break;
            }
            
            if (recved == 0)
                break;
                
            bufferPosition += recved;
        }
        
        close(client_socket);
    }
    
    return JS_undefined;
}
END

NARWHAL_MODULE(http_server_engine)
{
    EXPORTS("run", JS_fn(HS_run));
    
    JSObjectRef io = require("io");
    HANDLE_EXCEPTION(true, true);
    
    ByteIO = GET_OBJECT(io, "ByteIO");
    HANDLE_EXCEPTION(true, true);
    
    JSObjectRef binary = require("binary");
    HANDLE_EXCEPTION(true, true);
    
    ByteString = GET_OBJECT(binary, "ByteString");
    HANDLE_EXCEPTION(true, true);

}
END_NARWHAL_MODULE

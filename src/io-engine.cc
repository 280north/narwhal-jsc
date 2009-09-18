#include <narwhal.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <io-engine.h>
#include <binary-engine.h>

JSClassRef IO_class(JSContextRef context);

CONSTRUCTOR(IO_constructor)
{
    IOPrivate *data = (IOPrivate*)malloc(sizeof(IOPrivate));
    data->input = -1;
    data->output = -1;
    
    if (ARGC > 0) {
       ARGN_INT(in_fd, 0);
       data->input = in_fd;
    }
    if (ARGC > 1) {
       ARGN_INT(out_fd, 1);
       data->output = out_fd;
    }
    
    DEBUG("io=[%d,%d]\n", data->input, data->output);
    
    JSObjectRef object = JSObjectMake(context, IO_class(context), (void*)data);
    return object;
}
END

FUNCTION(IO_readInto, ARG_OBJ(buffer), ARG_INT(length))
{
    GET_INTERNAL(IOPrivate*, data, THIS);
    
    int fd = data->input;
    if (fd < 0)
        THROW("Stream not open for reading.");
    
    // get the _bytes property of the ByteString/Array, and the private data of that
    JSObjectRef _bytes = GET_OBJECT(buffer, "_bytes");
    GET_INTERNAL(BytesPrivate*, bytes, _bytes);
    
    int offset = GET_INT(buffer, "_offset");
    if (ARGC > 2) {
        ARGN_INT(from, 2);
        if (from < 0)
            THROW("Tried to read out of range.");
        offset += from;
    }
    
    // FIXME: make SURE this is correct
    if (offset < 0 || length > bytes->length - offset)
        THROW("FIXME: Buffer too small. Throw or truncate?");
    
    ssize_t total = 0,
            bytesRead = 0;
    
    while (total < length) {
        bytesRead = read(fd, bytes->buffer + (offset + total), length - total);
        DEBUG("bytesRead=%d (length - total)=%d\n", bytesRead, length - total);
        if (bytesRead <= 0)
            break;
        total += bytesRead;
    }
    
    return JS_int(total);
}
END

FUNCTION(IO_writeInto, ARG_OBJ(buffer), ARG_INT(from), ARG_INT(to))
{
    GET_INTERNAL(IOPrivate*, data, THIS);
    
    int fd = data->output;
    if (fd < 0)
        THROW("Stream not open for writing.");
    
    // get the _bytes property of the ByteString/Array, and the private data of that
    JSObjectRef _bytes = GET_OBJECT(buffer, "_bytes");
    GET_INTERNAL(BytesPrivate*, bytes, _bytes);
    
    int offset = GET_INT(buffer, "_offset");
    int length = GET_INT(buffer, "_length");
    
    // FIXME: make SURE this is correct
    if (offset < 0 || from < 0 || (offset + to) > bytes->length)
        THROW("Tried to write out of range.");
    
    write(fd, bytes->buffer + (offset + from), (to - from));
    
    return JS_undefined;
}
END

FUNCTION(IO_flush)
{
    GET_INTERNAL(IOPrivate*, data, THIS);
    return THIS;
}
END

FUNCTION(IO_close)
{
    GET_INTERNAL(IOPrivate*, data, THIS);
    
    if (data->input >= 0)
        close(data->input);
    if (data->output >= 0)
        close(data->output);
        
    data->input = -1;
    data->output = -1;
    
    return JS_undefined;
}
END

void IO_finalize(JSObjectRef object)
{
    GET_INTERNAL(IOPrivate*, data, object);
    
    DEBUG("freeing io=[%d,%d]\n", data->input, data->output);
    
    if (data->input >= 0)
        close(data->input);
    if (data->output >= 0)
        close(data->output);
        
    free(data);
}

NARWHAL_MODULE(io_engine)
{
    JSObjectRef IO = JSObjectMakeConstructor(context, IO_class(context), IO_constructor);
    EXPORTS("IO", IO);
    
    EXPORTS("STDIN_FILENO", JS_int(STDIN_FILENO));
    EXPORTS("STDOUT_FILENO", JS_int(STDOUT_FILENO));
    EXPORTS("STDERR_FILENO", JS_int(STDERR_FILENO));
    
    JSObjectRef io_engine_js = require("io-engine.js");
    EXPORTS("TextIOWrapper", GET_VALUE(io_engine_js, "TextIOWrapper"));
    EXPORTS("TextInputStream", GET_VALUE(io_engine_js, "TextInputStream"));
    EXPORTS("TextOutputStream", GET_VALUE(io_engine_js, "TextOutputStream"));
}
END_NARWHAL_MODULE


JSClassRef IO_class(JSContextRef context)
{
    static JSClassRef jsClass;
    if (!jsClass)
    {
        JSStaticFunction staticFuctions[5] = {
            { "readInto",       IO_readInto,        kJSPropertyAttributeNone },
            { "writeInto",      IO_writeInto,       kJSPropertyAttributeNone },
            { "flush",          IO_flush,           kJSPropertyAttributeNone },
            { "close",          IO_close,           kJSPropertyAttributeNone },
            { NULL,             NULL,               NULL }
        };

        JSClassDefinition definition = kJSClassDefinitionEmpty;
        definition.className = "IO"; 
        definition.staticFunctions = staticFuctions;
        definition.finalize = IO_finalize;
        definition.callAsConstructor = IO_constructor;

        jsClass = JSClassCreate(&definition);
    }
    
    return jsClass;
}
    
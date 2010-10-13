#include <narwhal.h>

#include <iconv.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <io-engine.h>
#include <binary-engine.h>

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

#ifdef NARWHAL_JSC
    JSObjectRef object = JSObjectMake(_context, IO_class(_context), (void*)data);
#elif NARWHAL_V8
    NWObject object = THIS;
    SET_INTERNAL(object, data);
#endif

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
    NWObject _bytes = GET_OBJECT(buffer, "_bytes");
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
    NWObject _bytes = GET_OBJECT(buffer, "_bytes");
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
    // FIXME
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

DESTRUCTOR(IO_finalize)
{
    GET_INTERNAL(IOPrivate*, data, object);
    
    DEBUG("freeing io=[%d,%d]\n", data->input, data->output);
    
    if (data->input >= 0)
        close(data->input);
    if (data->output >= 0)
        close(data->output);
        
    free(data);
}
END

CONSTRUCTOR(TextInputStream_constructor)
{
    TextInputStreamPrivate *data = (TextInputStreamPrivate*)malloc(sizeof(TextInputStreamPrivate));
    
    //raw, lineBuffering, buffering, charset, options
    ARGN_OBJ(raw, 0);
    //ARGN_OBJ(lineBuffering, 1);
    //ARGN_OBJ(buffering, 2);
    
    NWString charsetStr = (ARGC > 3 && IS_STRING(ARGV(3))) ?
        TO_STRING(ARGV(3)) :
        JS_str_utf8("UTF-8", strlen("UTF-8"));

    //ARGN_OBJ(options, 4);
    
    data->inBuffer = (char*)malloc(1024);
    data->inBufferSize = 1024;
    data->inBufferUsed = 0;
    
    GET_INTERNAL(IOPrivate*, raw_data, raw);
    data->input = raw_data->input;
    
#ifdef NARWHAL_JSC
    JSObjectRef object = JSObjectMake(_context, TextInputStream_class(_context), (void*)data);
#elif NARWHAL_V8
    NWObject object = THIS;
    SET_INTERNAL(object, data);
#endif
    
    SET_VALUE(object, "raw", raw);
    SET_VALUE(object, "charset", charsetStr);
    
    GET_UTF8(charset, charsetStr);
    data->cd = iconv_open(UTF_16_ENCODING, charset);
    if (data->cd == (iconv_t)-1)
        THROW("Error TextInputStream (iconv_open)");
    
    return object;
}
END

DESTRUCTOR(TextInputStream_finalize)
{
    GET_INTERNAL(TextInputStreamPrivate*, data, object);
    
    if (data) {
        iconv_close(data->cd);
        if (data->inBuffer)
            free(data->inBuffer);
        free(data);
    }
}
END
    
FUNCTION(TextInputStream_read)
{
    size_t numChars = 0;
    if (ARGC > 0) {
        ARGN_INT(n, 0);
        numChars = n;
    }
    
    GET_INTERNAL(TextInputStreamPrivate*, d, THIS);
    int fd = d->input;
    iconv_t cd = d->cd;
    
    size_t outBufferSize = numChars ? numChars * sizeof(NWChar) : 1024;
    size_t outBufferUsed = 0;
    char *outBuffer = (char*)malloc(outBufferSize);
    
    int last_errno = 0;
    while (true) {
        // if the outBuffer is completely filled, double it's size
        if (outBufferUsed >= outBufferSize) {
            outBufferSize *= 2;
            DEBUG("reallocing: %d (maybe: %d)\n", outBufferSize, numChars);
            
            // if we were given a number of characters to read then we've read them all, so we're done
            if (numChars != 0)
                break;
        
            outBuffer = (char*)realloc(outBuffer, outBufferSize);
        }
        
        // if there's no data in the buffer, or it ends in the middle of a multibyte character, read more
        if (d->inBufferUsed == 0 || last_errno == EINVAL) {
            DEBUG("need more data, reading\n");
            ssize_t num = read(fd, d->inBuffer + d->inBufferUsed, d->inBufferSize - d->inBufferUsed);
            if (num <= 0) {
                DEBUG("couldn't read more, done reading for now\n");
                break;
            } else {
                d->inBufferUsed += num;
            }
        }
        
        char *in = d->inBuffer;
        size_t inLeft = d->inBufferUsed;
        char *out = outBuffer + outBufferUsed;
        size_t outLeft = outBufferSize - outBufferUsed;
        
        last_errno = 0;
        size_t ret = iconv(cd, &in, &inLeft, &out, &outLeft);
        if (ret != (size_t)-1 || errno == EINVAL || errno == E2BIG) {
            if (ret == (size_t)-1)
                last_errno = errno;
            
            if (inLeft) {
                DEBUG("shifting %d bytes down by %d (had %d)\n", inLeft, in - d->inBuffer, d->inBufferUsed);
                memmove(d->inBuffer, in, inLeft);
            }
            d->inBufferUsed = inLeft;
            
            outBufferUsed = outBufferSize - outLeft;
            if (outBufferUsed != (out - outBuffer)) printf("sanity check failed: %d %d\n", outBufferUsed, (out - outBuffer));
        } else if (errno == EILSEQ) {
            // TODO: gracefully handle this case.
            // Illegal character or shift sequence
            free(outBuffer);
            THROW("Conversion error (Illegal character or shift sequence)");
        } else if (errno == EBADF) {
            // Invalid conversion descriptor
            free(outBuffer);
            THROW("Conversion error (Invalid conversion descriptor)");
        } else {
            // This errno is not defined
            free(outBuffer);
            THROW("Conversion error (unknown)");
        }
    }
    
    /*
    FILE *tmp = fopen("tmp.utf16", "w");
    fwrite(outBuffer, 1, outBufferUsed, tmp);
    fclose(tmp);
    //*/
    
    NWValue result = JS_str_utf16(outBuffer, outBufferUsed);
    free(outBuffer);
    
    return result;
}
END

/*
FUNCTION(TextInputStream_readLine)
{
}
END

FUNCTION(TextInputStream_next)
{
}
END

FUNCTION(TextInputStream_iterator)
{
}
END

FUNCTION(TextInputStream_forEach, ARG_FN(block), ARG_OBJ(context))
{
}
END

FUNCTION(TextInputStream_input)
{
}
END

FUNCTION(TextInputStream_readLines)
{
}
END

FUNCTION(TextInputStream_readInto, ARG_OBJ(buffer))
{
}
END

FUNCTION(TextInputStream_copy, ARG_OBJ(output), ARG_OBJ(mode), ARG_OBJ(options))
{
}
END

FUNCTION(TextInputStream_close)
{
}
END
*/

#ifdef NARWHAL_JSC

extern "C" JSClassRef IO_class(JSContextRef _context)
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

extern "C" JSClassRef TextInputStream_class(JSContextRef _context)
{
    static JSClassRef jsClass;
    if (!jsClass)
    {
        JSStaticFunction staticFuctions[] = {
            { "read",       TextInputStream_read,       kJSPropertyAttributeNone },
            //{ "readLine",   TextInputStream_readLine,   kJSPropertyAttributeNone },
            //{ "next",       TextInputStream_next,       kJSPropertyAttributeNone },
            //{ "iterator",   TextInputStream_iterator,   kJSPropertyAttributeNone },
            //{ "forEach",    TextInputStream_forEach,    kJSPropertyAttributeNone },
            //{ "input",      TextInputStream_input,      kJSPropertyAttributeNone },
            //{ "readLines",  TextInputStream_readLines,  kJSPropertyAttributeNone },
            //{ "readInto",   TextInputStream_readInto,   kJSPropertyAttributeNone },
            //{ "copy",       TextInputStream_copy,       kJSPropertyAttributeNone },
            //{ "close",      TextInputStream_close,      kJSPropertyAttributeNone },
            { NULL,         NULL,                       NULL }
        };

        JSClassDefinition definition = kJSClassDefinitionEmpty;
        definition.className = "TextInputStream"; 
        definition.staticFunctions = staticFuctions;
        definition.finalize = TextInputStream_finalize;
        definition.callAsConstructor = TextInputStream_constructor;

        jsClass = JSClassCreate(&definition);
    }
    
    return jsClass;
}

#elif NARWHAL_V8

NWObject TextInputStream_class() {
    v8::Handle<v8::FunctionTemplate> ft = v8::FunctionTemplate::New(TextInputStream_constructor);
    v8::Handle<v8::ObjectTemplate>   ot = ft->InstanceTemplate();
    ot->Set("read", v8::FunctionTemplate::New(TextInputStream_read));
    ot->SetInternalFieldCount(1);
    return ft->GetFunction();
}

NWObject IO_class() {
    v8::Handle<v8::FunctionTemplate> ft = v8::FunctionTemplate::New(IO_constructor);
    v8::Handle<v8::ObjectTemplate>   ot = ft->InstanceTemplate();
    ot->Set("readInto",       v8::FunctionTemplate::New(IO_readInto));
    ot->Set("writeInto",      v8::FunctionTemplate::New(IO_writeInto));
    ot->Set("flush",          v8::FunctionTemplate::New(IO_flush));
    ot->Set("close",          v8::FunctionTemplate::New(IO_close));
    ot->SetInternalFieldCount(1);
    return ft->GetFunction();
}
#endif

NARWHAL_MODULE(io_engine)
{
    EXPORTS("IO", CLASS(IO));
    EXPORTS("TextInputStream", CLASS(TextInputStream));
    
    EXPORTS("STDIN_FILENO", JS_int(STDIN_FILENO));
    EXPORTS("STDOUT_FILENO", JS_int(STDOUT_FILENO));
    EXPORTS("STDERR_FILENO", JS_int(STDERR_FILENO));
    
    NW_require("io-engine.js");
}
END_NARWHAL_MODULE

#include <narwhal.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>
#include <errno.h>

typedef struct __BytesPrivate BytesPrivate;

struct __BytesPrivate {
    unsigned char* buffer;
    unsigned int length;
};

DESTRUCTOR(Bytes_finalize)
{    
    GET_INTERNAL(BytesPrivate *, data, object);
    printf("freeing bytes=[%p]\n", data);
    if (data) {
        if (data->buffer)
            free(data->buffer);
        data->buffer = NULL;
        free(data);
    }
}
END

JSClassRef Bytes_class(JSContextRef context)
{
    static JSClassRef jsClass;
    if (!jsClass) {
        JSClassDefinition definition = kJSClassDefinitionEmpty;
        definition.finalize = Bytes_finalize;
        
        jsClass = JSClassCreate(&definition);
    }
    return jsClass;
}

JSObjectRef Bytes_new(JSContextRef context, unsigned char* buffer, unsigned int length)
{
    BytesPrivate *data = (BytesPrivate*)malloc(sizeof(BytesPrivate));
    if (!data) return NULL;
    
    data->length = length;
    data->buffer = buffer;
    
    return JSObjectMake(context, Bytes_class(context), data);
}

FUNCTION(B_ALLOC, ARG_INT(length))
{
    unsigned char *buffer = (unsigned char*)calloc(length, sizeof(char));
    if (!buffer)
        THROW("B_ALLOC: Couldn't alloc buffer");
    
    return Bytes_new(context, buffer, length);//Persistent<Object>::New(Bytes_new(buffer, length));
}
END

FUNCTION(B_LENGTH, ARG_OBJ(bytes))
{
    GET_INTERNAL(BytesPrivate*, byte_data, bytes);
    return JS_int(byte_data->length);
}
END

FUNCTION(B_FILL, ARG_OBJ(bytes), ARG_INT(from), ARG_INT(to), ARG_INT(value))
{
    GET_INTERNAL(BytesPrivate*, byte_data, bytes);
    
    if (!byte_data->buffer)
        THROW("B_FILL: NULL buffer");
    
    if (from > to || from < 0 || to >= byte_data->length)
        THROW("B_FILL: tried to fill beyond bounds");
    
    memset(byte_data->buffer + from, value, to - from);
    
    return JS_undefined;
}
END

FUNCTION(B_COPY, ARG_OBJ(src), ARG_INT(srcOffset), ARG_OBJ(dst), ARG_INT(dstOffset), ARG_INT(length))
{
    GET_INTERNAL(BytesPrivate*, src_data, src);
    GET_INTERNAL(BytesPrivate*, dst_data, dst);

    if (srcOffset + length > src_data->length || dstOffset + length > dst_data->length)
        THROW("B_COPY: tried to copy beyond bounds");

    memcpy(dst_data->buffer + dstOffset, src_data->buffer + srcOffset, length);
    
    return JS_undefined;
}
END

FUNCTION(B_GET, ARG_OBJ(bytes), ARG_INT(index))
{
    GET_INTERNAL(BytesPrivate*, bytes_data, bytes);
    
    if (index >= bytes_data->length)
        THROW("B_GET: tried get beyond bounds");
    
    unsigned char b = bytes_data->buffer[index];
    //printf("index=%d b=%u", index, b);
    //printf("[%s][%p]\n", bytes_data->buffer, bytes_data->buffer);
    
    return JS_int((int)b);
}
END

FUNCTION(B_SET, ARG_OBJ(bytes), ARG_INT(index), ARG_INT(value))
{
    GET_INTERNAL(BytesPrivate*, bytes_data, bytes);
    
    if (index >= bytes_data->length) 
        THROW("B_SET: tried set beyond bounds");
    if (value < 0 || value >= 256)
        THROW("B_SET: tried to set out of byte range");
    
    bytes_data->buffer[index] = (unsigned char)value;
    
    return JS_undefined;
}
END

FUNCTION(B_DECODE_DEFAULT, ARG_OBJ(bytes), ARG_INT(offset), ARG_INT(length))
{
    GET_INTERNAL(BytesPrivate*, bytes_data, bytes);
    return JS_str_utf8((char *)(bytes_data->buffer + offset), length);
}
END

FUNCTION(B_ENCODE_DEFAULT, ARG_UTF8(string))
{
    int length = strlen(string);
    
    unsigned char* buffer = (unsigned char*)calloc(length, sizeof(char));
    if (!buffer)
        THROW("B_ENCODE_DEFAULT: Couldn't alloc buffer");
    
    memcpy(buffer, string, length);
    
    return Bytes_new(context, buffer, length);//Persistent<Object>::New(Bytes_new(buffer, length));
}
END

FUNCTION(B_TRANSCODE, ARG_OBJ(src), ARG_INT(offset), ARG_INT(length), ARG_UTF8(sourceCodec), ARG_UTF8(targetCodec))
{
    GET_INTERNAL(BytesPrivate*, src_data, src);
    
    int dst_length = length*10; // FIXME!!!!!!
    char *dst = (char*)calloc(dst_length, 1);
    if (!dst)
        THROW("B_TRANSCODE: malloc fail");

    iconv_t cd = iconv_open(targetCodec, sourceCodec);
    if (cd == (iconv_t)-1)
        THROW("B_TRANSCODE: iconv_open fail");

    char *src_buf = (char*)(src_data->buffer + offset), *dst_buf = dst;
    size_t src_bytes_left = (size_t)length, dst_bytes_left = (size_t)dst_length, converted=0;
    
    while (dst_bytes_left > 0)
    {
        if (src_bytes_left == 0)
            break;
        if ((converted = iconv(cd, &src_buf, &src_bytes_left, &dst_buf, &dst_bytes_left)) == (size_t)-1)
        {
            if (errno != EINVAL)
            {
                perror("B_TRANSCODE");
                THROW("B_TRANSCODE: iconv error");
            }
        }
    }
    
    if (dst_bytes_left >= sizeof (wchar_t))
        *((wchar_t *) dst_buf) = L'\0';
    
    if (iconv_close(cd))
        THROW("B_TRANSCODE: iconv_close error");
    
    if (src_bytes_left > 0)
        THROW("B_TRANSCODE: buffer not big enough");
        
    //Handle<Object> bytes = Persistent<Object>::New(Bytes_new((unsigned char *)dst, dst_length - dst_bytes_left));

    return Bytes_new(context, (unsigned char *)dst, dst_length - dst_bytes_left);//bytes;
}
END

NARWHAL_MODULE(binary_platform)
{
    JSObjectRef impljs = require("binary-engine.js");
    
    EXPORTS("B_LENGTH", JS_fn(B_LENGTH));
    EXPORTS("B_ALLOC", JS_fn(B_ALLOC));
    EXPORTS("B_FILL", JS_fn(B_FILL));
    EXPORTS("B_COPY", JS_fn(B_COPY));
    EXPORTS("B_GET", JS_fn(B_GET));
    EXPORTS("B_SET", JS_fn(B_SET));
    
    //EXPORTS("B_DECODE", JS_fn(B_DECODE));
    EXPORTS("B_DECODE", OBJECT_GET(impljs, "B_DECODE"));
    //EXPORTS("B_ENCODE", JS_fn(B_ENCODE));
    EXPORTS("B_ENCODE", OBJECT_GET(impljs, "B_ENCODE"));
    
    EXPORTS("B_DECODE_DEFAULT", JS_fn(B_DECODE_DEFAULT));
    EXPORTS("B_ENCODE_DEFAULT", JS_fn(B_ENCODE_DEFAULT));
    
    EXPORTS("B_TRANSCODE", JS_fn(B_TRANSCODE));

    EXPORTS("DEFAULT_CODEC", JS_str_utf8("UTF-8", strlen("UTF-8")));
}
END_NARWHAL_MODULE

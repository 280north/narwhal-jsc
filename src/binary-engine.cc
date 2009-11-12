#include <narwhal.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>
#include <errno.h>

#include <binary-engine.h>

DESTRUCTOR(Bytes_finalize)
{    
    GET_INTERNAL(BytesPrivate *, data, object);
    DEBUG("freeing bytes=[%p]\n", data);
    if (data) {
        if (data->buffer)
            free(data->buffer);
        data->buffer = NULL;
        free(data);
    }
}
END

#ifdef NARWHAL_JSC

JSClassRef Bytes_class(JSContextRef _context)
{
    static JSClassRef jsClass;
    if (!jsClass)
    {
        JSClassDefinition definition = kJSClassDefinitionEmpty;
        definition.finalize = Bytes_finalize;

        jsClass = JSClassCreate(&definition);
    }
    return jsClass;
}

JSObjectRef Bytes_new(JSContextRef _context, JSValueRef *_exception, char* buffer, int length)
{
    BytesPrivate *data = (BytesPrivate*)malloc(sizeof(BytesPrivate));
    if (!data) return NULL;
    
    data->length = length;
    data->buffer = buffer;
    
    return JSObjectMake(_context, Bytes_class(_context), data);
}

#elif NARWHAL_V8

OBJECT(Bytes_new, 2, char* buffer, size_t length)
{
    BytesPrivate *data = (BytesPrivate*)malloc(sizeof(BytesPrivate));
    
    data->length = length;
    data->buffer = buffer;
    
    SET_INTERNAL(self, data);
    
    Persistent<Object> _weak_handle = Persistent<Object>::New(self);
    _weak_handle.MakeWeak(NULL, &Bytes_finalize);

    return self;
}
END

#endif

FUNCTION(B_ALLOC, ARG_INT(length))
{
    char *buffer = (char*)calloc(length, sizeof(char));
    if (!buffer)
        THROW("B_ALLOC: Couldn't alloc buffer");

    return CALL(Bytes_new, buffer, length);
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

    char* buffer = (char*)calloc(length, sizeof(char));
    if (!buffer)
        THROW("B_ENCODE_DEFAULT: Couldn't alloc buffer");

    memcpy(buffer, string, length);

    return CALL(Bytes_new, buffer, length);
}
END

int transcode(char *src, size_t srcLength, char **dstOut, size_t *dstLengthOut, const char *srcCodec, const char *dstCodec)
{
    /*
    printf("src=%p srcLength=%d srcCodec=%s dstCodec=%s\n", src, srcLength, srcCodec, dstCodec);
    for (size_t i = 0; i < srcLength; i++)
        printf("%02x ",(unsigned char)src[i]);
    printf("\n");
    //*/
    
    int dst_length = srcLength*10; // FIXME!!!!!!
    char *dst = (char*)calloc(dst_length, 1);
    if (!dst) {
        perror("transcode");
        return 0;
    }

    iconv_t cd = iconv_open(dstCodec, srcCodec);
    if (cd == (iconv_t)-1) {
        perror("transcode");
        return 0;
    }

    char *src_buf = src, *dst_buf = dst;
    size_t src_bytes_left = (size_t)srcLength, dst_bytes_left = (size_t)dst_length, converted=0;

    while (dst_bytes_left > 0)
    {
        if (src_bytes_left == 0)
            break;
        if ((converted = iconv(cd, &src_buf, &src_bytes_left, &dst_buf, &dst_bytes_left)) == (size_t)-1)
        {
            if (errno != EINVAL)
            {
                perror("transcode");
                return 0;
            }
        }
    }

    if (dst_bytes_left >= sizeof (wchar_t))
        *((wchar_t *) dst_buf) = L'\0';

    if (iconv_close(cd)) {
        perror("transcode");
        return 0;
    }

    if (src_bytes_left > 0) {
        perror("transcode");
        return 0;
    }

    *dstOut = dst;
    *dstLengthOut = (dst_length - dst_bytes_left);

    return 1;
}

FUNCTION(B_DECODE, ARG_OBJ(bytes), ARG_INT(offset), ARG_INT(srcLength), ARG_UTF8(codec))
{
    GET_INTERNAL(BytesPrivate*, src_data, bytes);

    char *dst;
    size_t dstLength;

    if (!transcode((char *)(src_data->buffer + offset), srcLength, &dst, &dstLength, codec, UTF_16_ENCODING))
        THROW("B_DECODE: iconv error");

    NWString string = JS_str_utf16(dst, dstLength);
    free(dst);

    return string;
}
END

FUNCTION(B_ENCODE, ARG_STR(string), ARG_UTF8(codec))
{
    char *src, *dst;
    size_t srcLength, dstLength;
    
    if (!GET_UTF16(string, &src, &srcLength))
        THROW("BLAHHHHH");

    if (!transcode(src, srcLength, &dst, &dstLength, UTF_16_ENCODING, codec))
        THROW("B_ENCODE: iconv error");
    
    free(src);

    return CALL(Bytes_new, dst, dstLength);
}
END

FUNCTION(B_TRANSCODE, ARG_OBJ(srcBytes), ARG_INT(srcOffset), ARG_INT(srcLength), ARG_UTF8(srcCodec), ARG_UTF8(dstCodec))
{
    GET_INTERNAL(BytesPrivate*, src_data, srcBytes);

    char *dst;
    size_t dstLength;

    char *src = (char *)(src_data->buffer + srcOffset);
    if (!transcode(src, srcLength, &dst, &dstLength, srcCodec, dstCodec))
        THROW("B_TRANSCODE: iconv error");

    return CALL(Bytes_new, dst, dstLength);
}
END

NARWHAL_MODULE(binary_engine)
{
    EXPORTS("B_LENGTH", JS_fn(B_LENGTH));
    EXPORTS("B_ALLOC", JS_fn(B_ALLOC));
    EXPORTS("B_FILL", JS_fn(B_FILL));
    EXPORTS("B_COPY", JS_fn(B_COPY));
    EXPORTS("B_GET", JS_fn(B_GET));
    EXPORTS("B_SET", JS_fn(B_SET));

    EXPORTS("B_DECODE", JS_fn(B_DECODE));
    EXPORTS("B_ENCODE", JS_fn(B_ENCODE));

    EXPORTS("B_DECODE_DEFAULT", JS_fn(B_DECODE_DEFAULT));
    EXPORTS("B_ENCODE_DEFAULT", JS_fn(B_ENCODE_DEFAULT));

    EXPORTS("B_TRANSCODE", JS_fn(B_TRANSCODE));

    EXPORTS("DEFAULT_CODEC", JS_str_utf8("UTF-8", strlen("UTF-8")));
}
END_NARWHAL_MODULE

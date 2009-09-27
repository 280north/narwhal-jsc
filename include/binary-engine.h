#ifndef __BINARY_ENGINE__
#define __BINARY_ENGINE__

typedef struct __BytesPrivate BytesPrivate;

struct __BytesPrivate {
    char *buffer;
    size_t length;
};

JSObjectRef Bytes_new(JSContextRef _context, JSValueRef *_exception, char* buffer, int length);

#endif

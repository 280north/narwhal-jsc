#ifndef __BINARY_ENGINE__
#define __BINARY_ENGINE__

typedef struct __BytesPrivate BytesPrivate;

struct __BytesPrivate {
    char *buffer;
    size_t length;
};

#endif

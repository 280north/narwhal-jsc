#ifndef __BINARY_ENGINE__
#define __BINARY_ENGINE__

typedef struct __BytesPrivate BytesPrivate;

struct __BytesPrivate {
    unsigned char* buffer;
    unsigned int length;
};

#endif

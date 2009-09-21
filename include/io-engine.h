#ifndef __IO_ENGINE__
#define __IO_ENGINE__

typedef struct __IOPrivate IOPrivate;

struct __IOPrivate {
    int input;
    int output;
};


typedef struct __TextInputStreamPrivate TextInputStreamPrivate;

struct __TextInputStreamPrivate {
    int input;
    iconv_t cd;
    
    char *inBuffer;
    size_t inBufferSize;
    size_t inBufferUsed;
};

#endif

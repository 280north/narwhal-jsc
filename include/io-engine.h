#ifndef __IO_ENGINE__
#define __IO_ENGINE__

#include <iconv.h>

typedef struct __IOPrivate {
    int input;
    int output;
} IOPrivate;

typedef struct __TextInputStreamPrivate {
    int input;
    iconv_t cd;
    
    char *inBuffer;
    size_t inBufferSize;
    size_t inBufferUsed;
} TextInputStreamPrivate;

extern "C" JSClassRef IO_class(JSContextRef _context);
extern "C" JSClassRef TextInputStream_class(JSContextRef _context);

#endif

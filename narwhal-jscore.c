#include <narwhal.h>

int main(int argc, char *argv[], char *envp[])
{
    JSGlobalContextRef _context = JSGlobalContextCreate(NULL);

    JSValueRef exception = NULL;
    JSValueRef *_exception = &exception;

    CALL(narwhal, argc, argv, envp, 1);

    int code = !!(*_exception);

    return code;
}

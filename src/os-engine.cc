#include <narwhal.h>
#include <stdlib.h>
#include <unistd.h>

FUNCTION(OS_Exit)
{
    if (ARGC == 0)
    {
        exit(0);
    }
    else if (ARGC == 1)
    {
        ARGN_INT(code, 0);
        exit(code);
    }
    
    THROW("os.exit() takes 0 or 1 arguments.");
}
END

FUNCTION(OS_Sleep, ARG_INT(seconds))
{
    // TODO: higher resolution sleep?
    
    while (seconds > 0)
        seconds = sleep(seconds);
    
    return JS_undefined;
}
END

FUNCTION(OS_SystemImpl, ARG_UTF8(command))
{
    return JS_int(system(command));
}
END

NARWHAL_MODULE(os_engine)
{
    EXPORTS("exit", JS_fn(OS_Exit));
    EXPORTS("sleep", JS_fn(OS_Sleep));
    EXPORTS("systemImpl", JS_fn(OS_SystemImpl));
    
    require("os-engine.js");

}
END_NARWHAL_MODULE

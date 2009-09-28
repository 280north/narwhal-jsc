#include <narwhal.h>
#include <stdlib.h>
#include <unistd.h>

JSObjectRef IO;

FUNCTION(OS_exit)
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

FUNCTION(OS_sleep, ARG_INT(seconds))
{
    // TODO: higher resolution sleep?
    
    while (seconds > 0)
        seconds = sleep(seconds);
    
    return JS_undefined;
}
END

FUNCTION(OS_systemImpl, ARG_UTF8(command))
{
    return JS_int(system(command));
}
END

typedef struct __PopenPrivate {
    pid_t pid;
} PopenPrivate;

FUNCTION(Popen_wait)
{
    GET_INTERNAL(PopenPrivate*, data, THIS);
    
    int status;
    
    pid_t pid = waitpid(data->pid, &status, WUNTRACED);
    if (!pid) {
        perror("waitpid");
    }
    else {
        if (WIFEXITED(status)) {
            return JS_int(WEXITSTATUS(status));
        } else {
            printf("WIFSIGNALED=%d\n", WIFSIGNALED(status));
            printf("WIFSTOPPED=%d\n", WIFSTOPPED(status));
        }
    }
    
    return JS_null;
}
END

FUNCTION(OS_popenImpl, ARG_UTF8(command))
{
    int oldstdin  = dup(STDIN_FILENO); // Save current stdin
    int oldstdout = dup(STDOUT_FILENO); // Save stdout
    int oldstderr = dup(STDERR_FILENO); // Save stdout
    
    int infd[2];
    int outfd[2];
    int errfd[2];

    pipe(infd); // From where parent is going to read
    pipe(outfd); // Where the parent is going to write to
    pipe(errfd); // Where the parent is going to write to

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    dup2(outfd[0], STDIN_FILENO); // Make the read end of outfd pipe as stdin
    dup2(infd[1], STDOUT_FILENO); // Make the write end of infd as stdout
    dup2(errfd[1], STDERR_FILENO); // Make the write end of infd as stdout

    PopenPrivate *data = (PopenPrivate *)malloc(sizeof(PopenPrivate));
    if (!data)
        THROW("OOM");

    data->pid = fork();
    if(!data->pid)
    {
        char *argv[] = {
            "/bin/sh",
            "-c",
            command,
            NULL
        };
        
        close(infd[0]);
        close(infd[1]);
        close(outfd[0]);
        close(outfd[1]);
        close(errfd[0]);
        close(errfd[1]);

        execv(argv[0], argv);
    }
    else
    {
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        
        dup2(oldstdin, STDIN_FILENO);
        dup2(oldstdout, STDOUT_FILENO);
        dup2(oldstderr, STDERR_FILENO);

        close(outfd[0]);
        close(infd[1]);
        close(errfd[1]);

        JSObjectRef obj = JSObjectMake(_context, Custom_class(_context), data);
        
        ARGS_ARRAY(stdinArgs, JS_int(-1), JS_int(outfd[0]));
        JSObjectRef stdinObj = CALL_AS_CONSTRUCTOR(IO, 2, stdinArgs);
        HANDLE_EXCEPTION(true, true);
        SET_VALUE(obj, "stdin", stdinObj);
        HANDLE_EXCEPTION(true, true);
        
        ARGS_ARRAY(stdoutArgs, JS_int(infd[0]), JS_int(-1));
        JSObjectRef stdoutObj = CALL_AS_CONSTRUCTOR(IO, 2, stdoutArgs);
        HANDLE_EXCEPTION(true, true);
        SET_VALUE(obj, "stdout", stdoutObj);
        HANDLE_EXCEPTION(true, true);
        
        ARGS_ARRAY(stderrArgs, JS_int(errfd[0]), JS_int(-1));
        JSObjectRef stderrObj = CALL_AS_CONSTRUCTOR(IO, 2, stderrArgs);
        HANDLE_EXCEPTION(true, true);
        SET_VALUE(obj, "stderr", stderrObj);
        HANDLE_EXCEPTION(true, true);
        
        SET_VALUE(obj, "wait", JS_fn(Popen_wait));
        HANDLE_EXCEPTION(true, true);
        
        return obj;
    }
}
END
    
NARWHAL_MODULE(os_engine)
{
    EXPORTS("exit", JS_fn(OS_exit));
    EXPORTS("sleep", JS_fn(OS_sleep));
    EXPORTS("systemImpl", JS_fn(OS_systemImpl));
    EXPORTS("popenImpl", JS_fn(OS_popenImpl));
    
    require("os-engine.js");
    
    JSObjectRef io = require("io");
    HANDLE_EXCEPTION(true, true);
    
    IO = GET_OBJECT(io, "IO");
    HANDLE_EXCEPTION(true, true);
}
END_NARWHAL_MODULE

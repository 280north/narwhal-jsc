#include <narwhal.h>
#include <stdlib.h>
#include <unistd.h>

NWObject IO;

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
    pid_t   pid;
    int     infd;
    int     outfd;
    int     errfd;
} PopenPrivate;

DESTRUCTOR(Popen_finalize)
{    
    GET_INTERNAL(PopenPrivate *, data, object);
    DEBUG("Popen_finalize=[%p]\n", data);
    if (data) {
        if (data->infd >= 0) close(data->infd);
        if (data->outfd >= 0) close(data->outfd);
        if (data->errfd >= 0) close(data->errfd);
        free(data);
    }
}
END

JSClassRef Popen_class(JSContextRef _context)
{
    static JSClassRef jsClass;
    if (!jsClass)
    {
        JSClassDefinition definition = kJSClassDefinitionEmpty;
        definition.className = "Popen";
        definition.finalize = Popen_finalize;

        jsClass = JSClassCreate(&definition);
    }
    return jsClass;
}

JSObjectRef Popen_new(JSContextRef _context, JSValueRef *_exception, pid_t pid, int infd, int outfd, int errfd)
{
    PopenPrivate *data = (PopenPrivate*)malloc(sizeof(PopenPrivate));
    if (!data) return NULL;
    
    data->pid   = pid;
    data->infd  = infd;
    data->outfd = outfd;
    data->errfd = errfd;
    
    return JSObjectMake(_context, Popen_class(_context), data);
}

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
    // save stdin, stdout, stderr
    int oldstdin  = dup(STDIN_FILENO);
    int oldstdout = dup(STDOUT_FILENO);
    int oldstderr = dup(STDERR_FILENO);
    
    if (oldstdin < 0 || oldstdout < 0 || oldstderr < 0)
        THROW("popen error (dup): %s", strerror(errno));
    
    int outfd[2];
    int infd[2];
    int errfd[2];
    
    // create pipes for subprocess's stdin, stdout, stderr
    if (pipe(outfd) < 0) // From where parent is going to read
        THROW("popen error (pipe): %s", strerror(errno));
    if (pipe(infd) < 0) // Where the parent is going to write to
        THROW("popen error (pipe): %s", strerror(errno));
    if (pipe(errfd) < 0) // Where the parent is going to write to
         THROW("popen error (pipe): %s", strerror(errno));
    
    // close existing stdin, stdout, stderr
    if (close(STDIN_FILENO) < 0)
        THROW("popen error (close): %s", strerror(errno));
    if (close(STDOUT_FILENO) < 0)
        THROW("popen error (close): %s", strerror(errno));
    if (close(STDERR_FILENO) < 0)
        THROW("popen error (close): %s", strerror(errno));
    
    // set correct endpoint of pipes to stdin, stdout, stderr
    if (dup2(infd[0], STDIN_FILENO)  < 0) // Make the read end of infd pipe as stdin
        THROW("popen error (dup2): %s", strerror(errno));
    if (dup2(outfd[1],  STDOUT_FILENO) < 0) // Make the write end of outfd as stdout
        THROW("popen error (dup2): %s", strerror(errno));
    if (dup2(errfd[1], STDERR_FILENO) < 0) // Make the write end of outfd as stdout
        THROW("popen error (dup2): %s", strerror(errno));
    
    if (close(infd[0]) < 0)
        THROW("popen error (close): %s", strerror(errno));
    if (close(outfd[1]) < 0)
        THROW("popen error (close): %s", strerror(errno));
    if (close(errfd[1]) < 0)
        THROW("popen error (close): %s", strerror(errno));
        
    pid_t pid = fork();
    if(!pid)
    {
        // In the forked process:
        char *argv[] = {
            "/bin/sh",
            "-c",
            command,
            NULL
        };
        
        if (close(infd[1]) < 0)
            THROW("popen error (close): %s", strerror(errno));
        if (close(outfd[0]) < 0)
            THROW("popen error (close): %s", strerror(errno));
        if (close(errfd[0]) < 0)
            THROW("popen error (close): %s", strerror(errno));
    
        execv(argv[0], argv);
    }
    else
    {
        // In the forking process:
        if (close(STDIN_FILENO) < 0)
            THROW("popen error (close): %s", strerror(errno));
        if (close(STDOUT_FILENO) < 0)
            THROW("popen error (close): %s", strerror(errno));
        if (close(STDERR_FILENO) < 0)
            THROW("popen error (close): %s", strerror(errno));
        
        // restore stdin, stdout, stderr
        if (dup2(oldstdin, STDIN_FILENO) < 0)
            THROW("popen error (dup2): %s", strerror(errno));
        if (dup2(oldstdout, STDOUT_FILENO) < 0)
            THROW("popen error (dup2): %s", strerror(errno));
        if (dup2(oldstderr, STDERR_FILENO) < 0)
           THROW("popen error (dup2): %s", strerror(errno));
    
        NWObject obj = CALL(Popen_new, pid, infd[1], outfd[0], errfd[0]);
        
        ARGS_ARRAY(stdinArgs, JS_int(-1), JS_int(infd[1]));
        NWObject stdinObj = CALL_AS_CONSTRUCTOR(IO, 2, stdinArgs);
        HANDLE_EXCEPTION(true, true);
        SET_VALUE(obj, "stdin", stdinObj);
        HANDLE_EXCEPTION(true, true);
        
        ARGS_ARRAY(stdoutArgs, JS_int(outfd[0]), JS_int(-1));
        NWObject stdoutObj = CALL_AS_CONSTRUCTOR(IO, 2, stdoutArgs);
        HANDLE_EXCEPTION(true, true);
        SET_VALUE(obj, "stdout", stdoutObj);
        HANDLE_EXCEPTION(true, true);
        
        ARGS_ARRAY(stderrArgs, JS_int(errfd[0]), JS_int(-1));
        NWObject stderrObj = CALL_AS_CONSTRUCTOR(IO, 2, stderrArgs);
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
    
    NWObject io = require("io");
    HANDLE_EXCEPTION(true, true);
    
    IO = PROTECT_OBJECT(GET_OBJECT(io, "IO"));
    HANDLE_EXCEPTION(true, true);
}
END_NARWHAL_MODULE

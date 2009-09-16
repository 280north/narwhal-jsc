#include <narwhal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

// FIXME: all string args are leaking...

extern JSObjectRef Exports;
extern JSObjectRef Require;
extern JSObjectRef Require;
extern JSObjectRef Exports;
extern JSObjectRef Module;
extern JSObjectRef System;
extern JSObjectRef Print;
extern JSContextRef context;
extern void print(const char * string);
extern JSObjectRef require(const char *id);

JSObjectRef io = NULL;

FUNCTION(F_cwd)
{
    ARG_COUNT(0);
    
    char *cwd = getcwd(NULL, 0);
    if (!cwd)
        THROW("Couldn't get cwd.");
        
    JSValueRef cwdValue = JS_str_utf8(cwd, strlen(cwd));
    free(cwd);
    return cwdValue;
}
END

FUNCTION(F_canonical, ARG_UTF8(path))
{
    ARG_COUNT(1);
    printf("Warning: canonical not implemented");
    return ARGV(0);
}
END

FUNCTION(F_mtime, ARG_UTF8(path))
{
    ARG_COUNT(1);
    
    THROW("NYI F_mtime");
    
	struct stat stat_info;
    int ret = stat(path, &stat_info);
    
    return JS_undefined;
}
END

FUNCTION(F_size, ARG_UTF8(path))
{
    ARG_COUNT(1);
    
    THROW("NYI F_chown");
    
	struct stat stat_info;
    int ret = stat(path, &stat_info);
    
    return JS_int(stat_info.st_size);
}
END

FUNCTION(F_stat, ARG_UTF8(path))
{
    ARG_COUNT(1);

	struct stat stat_info;
    int ret = stat(path, &stat_info);
    
    return JS_undefined;
}
END

FUNCTION(F_exists, ARG_UTF8(path))
{
    ARG_COUNT(1);

	struct stat stat_info;
    int ret = stat(path, &stat_info);
    
    return JS_undefined;
}
END

FUNCTION(F_linkExists, ARG_UTF8(path))
{
    ARG_COUNT(1);

	struct stat stat_info;
    int ret = stat(path, &stat_info);

    return JS_bool(ret != -1 && (S_ISREG(stat_info.st_mode) || S_ISLNK(stat_info.st_mode)));
}
END

FUNCTION(F_isDirectory, ARG_UTF8(path))
{
    ARG_COUNT(1);

	struct stat stat_info;
    int ret = stat(path, &stat_info);

    return JS_bool(ret != -1 && S_ISDIR(stat_info.st_mode));
}
END
    
FUNCTION(F_isFile, ARG_UTF8(path))
{
    ARG_COUNT(1);

	struct stat stat_info;
    int ret = stat(path, &stat_info);

    return JS_bool(ret != -1 && S_ISREG(stat_info.st_mode));
}
END

FUNCTION(F_isLink, ARG_UTF8(path))
{
    ARG_COUNT(1);

	struct stat stat_info;
    int ret = stat(path, &stat_info);

    return JS_bool(ret != -1 && S_ISLNK(stat_info.st_mode));
}
END

FUNCTION(F_isReadable, ARG_UTF8(path))
{
    ARG_COUNT(1);
    
    return JS_bool(access(path, R_OK) == 0);
}
END

FUNCTION(F_isWritable, ARG_UTF8(path))
{
    ARG_COUNT(1);
    
    return JS_bool(access(path, W_OK) == 0);
}
END

FUNCTION(F_chmod, ARG_UTF8(path), ARG_INT(mode))
{
    ARG_COUNT(2);
    
    chmod(path, mode);
    
    return JS_undefined;
}
END

FUNCTION(F_chown, ARG_UTF8(path), ARG_UTF8(owner), ARG_UTF8(group))
{
    ARG_COUNT(3);
    
    THROW("NYI F_chown");
    return JS_undefined;
}
END

FUNCTION(F_FileIO, ARG_UTF8(path))
{
    JSObjectRef IO = GET_OBJECT(io, "IO");
    JSObjectRef mode_fn = GET_OBJECT(Exports, "mode");
    
    ARGS_ARRAY(argv, ARGV(1));
    JSObjectRef modeObject = JS_obj(CALL_AS_FUNCTION(mode_fn, NULL, 1, argv));
    
    int readFlag = GET_BOOL(modeObject, "read");
    int writeFlag = GET_BOOL(modeObject, "write");
    int appendFlag = GET_BOOL(modeObject, "append");
    int updateFlag = GET_BOOL(modeObject, "update");
    
    printf("path=%s readFlag=%d writeFlag=%d readFlag=%d readFlag=%d\n", path, readFlag, writeFlag, appendFlag, updateFlag);
    
    int oflag = 0;
    
    if (readFlag)   oflag = oflag | O_RDONLY;
    if (writeFlag)  oflag = oflag | O_WRONLY;
    if (appendFlag) oflag = oflag | O_APPEND;
    if (updateFlag) oflag = oflag | O_RDONLY;
    
    if (updateFlag) {
        THROW("Updating IO not yet implemented.");
    } else if (writeFlag || appendFlag) {
        int fd = open(path, oflag | O_CREAT, 0777);
        printf("fd=%d\n", fd);
        ARGS_ARRAY(argv, JS_int(-1), JS_int(fd));
        return JS_obj(CALL_AS_CONSTRUCTOR(IO, 2, argv));
    } else if (readFlag) {
        int fd = open(path, oflag);
        printf("fd=%d\n", fd);
        ARGS_ARRAY(argv, JS_int(fd), JS_int(-1));
        return JS_obj(CALL_AS_CONSTRUCTOR(IO, 2, argv));
    } else {
        THROW("Files must be opened either for read, write, or update mode.");
    }
    
    return JS_undefined;
}
END


NARWHAL_MODULE(file_engine)
{
    Exports = require("./file");
    io = require("io");
    
    EXPORTS("FileIO", JS_fn(F_FileIO));
    
    EXPORTS("cwd", JS_fn(F_cwd));
    EXPORTS("canonical", JS_fn(F_canonical));
    //EXPORTS("mtime", JS_fn(F_mtime));
    EXPORTS("size", JS_fn(F_size));
    EXPORTS("stat", JS_fn(F_stat));
    EXPORTS("exists", JS_fn(F_exists));
    EXPORTS("linkExists", JS_fn(F_linkExists));
    EXPORTS("isDirectory", JS_fn(F_isDirectory));
    EXPORTS("isFile", JS_fn(F_isFile));
    EXPORTS("isLink", JS_fn(F_isLink));
    EXPORTS("isReadable", JS_fn(F_isReadable));
    EXPORTS("isWritable", JS_fn(F_isWritable));
    EXPORTS("chmod", JS_fn(F_chmod));
    EXPORTS("chown", JS_fn(F_chown));
    //EXPORTS("link", JS_fn(F_link));
    //EXPORTS("symlink", JS_fn(F_symlink));
    //EXPORTS("rename", JS_fn(F_rename));
    //EXPORTS("move", JS_fn(F_move));
    //EXPORTS("remove", JS_fn(F_remove));
    //EXPORTS("mkdir", JS_fn(F_mkdir));
    //EXPORTS("mkdirs", JS_fn(F_mkdirs));
    //EXPORTS("rmdir", JS_fn(F_rmdir));
    //EXPORTS("list", JS_fn(F_list));
    //EXPORTS("touch", JS_fn(F_touch));
}
END_NARWHAL_MODULE

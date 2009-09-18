#include <narwhal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <utime.h>

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

FUNCTION(F_canonical, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);
    
    // FIXME: http://insanecoding.blogspot.com/2007/11/pathmax-simply-isnt.html
    char resolved_name[PATH_MAX];

    char *canon = realpath(path, resolved_name);
    if (!canon)
        THROW("Canonical failed!");

    return JS_str_utf8(canon, strnlen(canon, PATH_MAX));
}
END

FUNCTION(F_mtime, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);
    
    THROW("NYI mtime");
    
	struct stat stat_info;
    int ret = stat(path, &stat_info);
    
    return JS_undefined;
}
END

FUNCTION(F_size, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);
    
	struct stat stat_info;
    int ret = stat(path, &stat_info);
    
    return JS_int(stat_info.st_size);
}
END

FUNCTION(F_stat, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);

	struct stat stat_info;
    int ret = stat(path, &stat_info);
    
    return JS_undefined;
}
END

FUNCTION(F_exists, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);

	struct stat stat_info;
    int ret = stat(path, &stat_info);
    
    return JS_bool(ret != -1);
}
END

FUNCTION(F_linkExists, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);

	struct stat stat_info;
    int ret = stat(path, &stat_info);

    return JS_bool(ret != -1 && (S_ISREG(stat_info.st_mode) || S_ISLNK(stat_info.st_mode)));
}
END

FUNCTION(F_isDirectory, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);

	struct stat stat_info;
    int ret = stat(path, &stat_info);

    return JS_bool(ret != -1 && S_ISDIR(stat_info.st_mode));
}
END
    
FUNCTION(F_isFile, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);

	struct stat stat_info;
    int ret = stat(path, &stat_info);

    return JS_bool(ret != -1 && S_ISREG(stat_info.st_mode));
}
END

FUNCTION(F_isLink, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);

	struct stat stat_info;
    int ret = stat(path, &stat_info);

    return JS_bool(ret != -1 && S_ISLNK(stat_info.st_mode));
}
END

FUNCTION(F_isReadable, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);
    
    return JS_bool(access(path, R_OK) == 0);
}
END

FUNCTION(F_isWritable, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);
    
    return JS_bool(access(path, W_OK) == 0);
}
END

FUNCTION(F_chmod, ARG_UTF8_CAST(path), ARG_INT(mode))
{
    ARG_COUNT(2);
    
    if (chmod(path, mode) < 0)
        THROW("failed to chmod %s", path);
    
    return JS_undefined;
}
END

FUNCTION(F_chown, ARG_UTF8_CAST(path), ARG_UTF8_CAST(owner), ARG_UTF8_CAST(group))
{
    ARG_COUNT(3);
    
    THROW("NYI chown");
    
    return JS_undefined;
}
END

FUNCTION(F_link, ARG_UTF8_CAST(source), ARG_UTF8_CAST(target))
{
    ARG_COUNT(1);
    
    THROW("NYI link");

    return JS_undefined;
}
END

FUNCTION(F_symlink, ARG_UTF8_CAST(source), ARG_UTF8_CAST(target))
{
    ARG_COUNT(2);
    
    if (symlink(source, target) < 0)
        THROW("failed to symlink %s to %s", source, target);

    return JS_undefined;
}
END

FUNCTION(F_rename, ARG_UTF8_CAST(source), ARG_UTF8_CAST(target))
{
    ARG_COUNT(2);

    if (rename(source, target) < 0)
        THROW("failed to rename %s to %s", source, target);

    return JS_undefined;
}
END

//FUNCTION(F_move, ARG_UTF8_CAST(source), ARG_UTF8_CAST(target))
//{
//    ARG_COUNT(2);
//
//    THROW("NYI move");
//
//    return JS_undefined;
//}
//END
    
FUNCTION(F_remove, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);
    
    if (unlink(path) < 0)
        THROW("failed to delete %s: %s", path, strerror(errno));

    return JS_undefined;
}
END

FUNCTION(F_mkdir, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);
    
    if (mkdir(path, 0777) < 0) {
        // TODO: Should this fail
        THROW("failed to make directory %s: %s", path, strerror(errno));
    }

    return JS_undefined;
}
END

//FUNCTION(F_mkdirs, ARG_UTF8_CAST(path))
//{
//    ARG_COUNT(1);
//
//    THROW("NYI mkdirs");
//
//    return JS_undefined;
//}
//END

FUNCTION(F_rmdir, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);

    if (rmdir(path) < 0)
        THROW("failed to remove the directory %s: %s", path, strerror(errno));

    return JS_undefined;
}
END
    
FUNCTION(F_list, ARG_UTF8_CAST(path))
{
    DIR *dp;
    struct dirent *dirp;

    if((dp  = opendir(strlen(path) == 0 ? "." : path)) == NULL) {
        THROW("No such directory");
    }

    JSObjectRef array = JSObjectMakeArray(context, 0, NULL, _exception);

    int index = 0;
    while ((dirp = readdir(dp)) != NULL) {
        if (strcmp(".", dirp->d_name) && strcmp("..", dirp->d_name))
            JSObjectSetPropertyAtIndex(context, array, index++, JS_str_utf8(dirp->d_name, dirp->d_namlenq), _exception);
    }
    closedir(dp);

    return array;
}
END

FUNCTION(F_touch, ARG_UTF8_CAST(path), ARG_DOUBLE(mtime))
{
    ARG_COUNT(2);

    struct utimbuf times;
    times.actime = (time_t)mtime;
    times.modtime = (time_t)mtime;
    
    if (utime(path, &times) < 0)
        THROW("unable to set mtime of %s: %s", path, strerror(errno));

    return JS_undefined;
}
END

FUNCTION(F_FileIO, ARG_UTF8_CAST(path))
{
    JSObjectRef IO = GET_OBJECT(io, "IO");
    JSObjectRef mode_fn = GET_OBJECT(Exports, "mode");
    
    ARGS_ARRAY(argv, ARGV(1));
    JSObjectRef modeObject = JS_obj(CALL_AS_FUNCTION(mode_fn, NULL, 1, argv));
    
    int readFlag = GET_BOOL(modeObject, "read");
    int writeFlag = GET_BOOL(modeObject, "write");
    int appendFlag = GET_BOOL(modeObject, "append");
    int updateFlag = GET_BOOL(modeObject, "update");
    
    DEBUG("path=%s readFlag=%d writeFlag=%d readFlag=%d readFlag=%d\n", path, readFlag, writeFlag, appendFlag, updateFlag);
    
    int oflag = 0;
    
    if (readFlag)   oflag = oflag | O_RDONLY;
    if (writeFlag)  oflag = oflag | O_WRONLY;
    if (appendFlag) oflag = oflag | O_APPEND;
    if (updateFlag) oflag = oflag | O_RDONLY;
    
    if (updateFlag) {
        THROW("Updating IO not yet implemented.");
    } else if (writeFlag || appendFlag) {
        int fd = open(path, oflag | O_CREAT, 0777);
        DEBUG("fd=%d\n", fd);
        ARGS_ARRAY(argv, JS_int(-1), JS_int(fd));
        return JS_obj(CALL_AS_CONSTRUCTOR(IO, 2, argv));
    } else if (readFlag) {
        int fd = open(path, oflag);
        DEBUG("fd=%d\n", fd);
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
    EXPORTS("link", JS_fn(F_link));
    EXPORTS("symlink", JS_fn(F_symlink));
    EXPORTS("rename", JS_fn(F_rename));
    EXPORTS("move", JS_fn(F_rename));
    //EXPORTS("move", JS_fn(F_move));
    EXPORTS("remove", JS_fn(F_remove));
    EXPORTS("mkdir", JS_fn(F_mkdir));
    //EXPORTS("mkdirs", JS_fn(F_mkdirs));
    EXPORTS("rmdir", JS_fn(F_rmdir));
    EXPORTS("listImpl", JS_fn(F_list));
    //EXPORTS("list", JS_fn(F_list));
    EXPORTS("touchImpl", JS_fn(F_touch));
    //EXPORTS("touch", JS_fn(F_touch));
    
    JSObjectRef file_engine_js = require("file-engine.js");
    EXPORTS("mkdirs", GET_VALUE(file_engine_js, "mkdirs"));
    EXPORTS("touch", GET_VALUE(file_engine_js, "touch"));
}
END_NARWHAL_MODULE

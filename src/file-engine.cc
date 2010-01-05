#include <narwhal.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

#define PATH_OR_DOT(path) (path[0] == '\0' ? "." : path)

NWObject io;

FUNCTION(FILE_cwd)
{
    ARG_COUNT(0);
    
    char *cwd = getcwd(NULL, 0);
    if (!cwd)
        THROW("Couldn't get cwd.");
        
    NWValue cwdValue = JS_str_utf8(cwd, strlen(cwd));
    free(cwd);
    
    return cwdValue;
}
END

FUNCTION(FILE_canonicalImpl, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);
    
    // FIXME: http://insanecoding.blogspot.com/2007/11/pathmax-simply-isnt.html
    char resolved_name[PATH_MAX];

    char *canon = realpath(PATH_OR_DOT(path), resolved_name);
    if (!canon)
        return JS_str_utf8(path, strlen(path));

    return JS_str_utf8(canon, strlen(canon));
}
END

FUNCTION(FILE_mtime, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);
    
	struct stat stat_info;
    int ret = stat(PATH_OR_DOT(path), &stat_info);
    if (ret < 0)
        THROW("mtime: %s", strerror(errno));
    
    long long timestamp = ((long long)(stat_info.st_mtime)) * 1000;

    return JS_date(timestamp);
}
END

FUNCTION(FILE_size, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);
    
	struct stat stat_info;
    int ret = stat(PATH_OR_DOT(path), &stat_info);
    
    return JS_int(stat_info.st_size);
}
END

FUNCTION(FILE_stat, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);

	struct stat stat_info;
    int ret = stat(PATH_OR_DOT(path), &stat_info);
    
    return JS_undefined;
}
END

FUNCTION(FILE_exists, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);

	struct stat stat_info;
    int ret = stat(PATH_OR_DOT(path), &stat_info);
    
    return JS_bool(ret != -1);
}
END

FUNCTION(FILE_linkExists, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);

	struct stat stat_info;
    int ret = stat(PATH_OR_DOT(path), &stat_info);

    return JS_bool(ret != -1 && (S_ISREG(stat_info.st_mode) || S_ISLNK(stat_info.st_mode)));
}
END

FUNCTION(FILE_isDirectory, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);

	struct stat stat_info;
    int ret = stat(PATH_OR_DOT(path), &stat_info);

    return JS_bool(ret != -1 && S_ISDIR(stat_info.st_mode));
}
END
    
FUNCTION(FILE_isFile, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);

	struct stat stat_info;
    int ret = stat(PATH_OR_DOT(path), &stat_info);

    return JS_bool(ret != -1 && S_ISREG(stat_info.st_mode));
}
END

FUNCTION(FILE_isLink, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);

	struct stat stat_info;
    int ret = lstat(PATH_OR_DOT(path), &stat_info);

    return JS_bool(ret != -1 && S_ISLNK(stat_info.st_mode));
}
END

FUNCTION(FILE_isReadable, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);
    
    return JS_bool(access(PATH_OR_DOT(path), R_OK) == 0);
}
END

FUNCTION(FILE_isWritable, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);
    
    return JS_bool(access(PATH_OR_DOT(path), W_OK) == 0);
}
END

FUNCTION(FILE_chmod, ARG_UTF8_CAST(path), ARG_INT(mode))
{
    ARG_COUNT(2);
    
    if (chmod(PATH_OR_DOT(path), mode) < 0)
        THROW("failed to chmod %s", path);
    
    return JS_undefined;
}
END

FUNCTION(FILE_chown, ARG_UTF8_CAST(path), ARG_UTF8_CAST(owner), ARG_UTF8_CAST(group))
{
    ARG_COUNT(3);
    
    THROW("NYI chown");
    
    return JS_undefined;
}
END

FUNCTION(FILE_link, ARG_UTF8_CAST(source), ARG_UTF8_CAST(target))
{
    ARG_COUNT(1);
    
    THROW("NYI link");

    return JS_undefined;
}
END

FUNCTION(FILE_symlink, ARG_UTF8_CAST(source), ARG_UTF8_CAST(target))
{
    ARG_COUNT(2);
    
    if (symlink(PATH_OR_DOT(source), PATH_OR_DOT(target)) < 0)
        THROW("failed to symlink %s to %s", source, target);

    return JS_undefined;
}
END

FUNCTION(FILE_renameImpl, ARG_UTF8_CAST(source), ARG_UTF8_CAST(target))
{
    ARG_COUNT(2);

    if (rename(PATH_OR_DOT(source), PATH_OR_DOT(target)) < 0)
        THROW("failed to rename %s to %s", source, target);

    return JS_undefined;
}
END

//FUNCTION(FILE_move, ARG_UTF8_CAST(source), ARG_UTF8_CAST(target))
//{
//    ARG_COUNT(2);
//
//    THROW("NYI move");
//
//    return JS_undefined;
//}
//END
    
FUNCTION(FILE_remove, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);
    
    if (unlink(PATH_OR_DOT(path)) < 0)
        THROW("failed to delete %s: %s", path, strerror(errno));

    return JS_undefined;
}
END

FUNCTION(FILE_mkdir, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);
    
    if (mkdir(PATH_OR_DOT(path), 0777) < 0) {
        // TODO: Should this fail
        THROW("failed to make directory %s: %s", path, strerror(errno));
    }

    return JS_undefined;
}
END

//FUNCTION(FILE_mkdirs, ARG_UTF8_CAST(path))
//{
//    ARG_COUNT(1);
//
//    THROW("NYI mkdirs");
//
//    return JS_undefined;
//}
//END

FUNCTION(FILE_rmdir, ARG_UTF8_CAST(path))
{
    ARG_COUNT(1);

    if (rmdir(PATH_OR_DOT(path)) < 0)
        THROW("failed to remove the directory %s: %s", path, strerror(errno));

    return JS_undefined;
}
END
    
FUNCTION(FILE_list, ARG_UTF8_CAST(path))
{
    DIR *dp;
    struct dirent *dirp;

    if((dp  = opendir(PATH_OR_DOT(path))) == NULL) {
        THROW("%s", strerror(errno));
    }

    NWObject array = JS_array(0, NULL);

    int index = 0;
    while ((dirp = readdir(dp)) != NULL) {
        if (strcmp(".", dirp->d_name) && strcmp("..", dirp->d_name))
            SET_VALUE_AT_INDEX(array, index++, JS_str_utf8(dirp->d_name, dirp->d_namlen));
    }
    closedir(dp);

    return array;
}
END

FUNCTION(FILE_touchImpl, ARG_UTF8_CAST(path), ARG_DOUBLE(mtime))
{
    ARG_COUNT(2);

    struct utimbuf times;
    times.actime = (time_t)(mtime/1000.0);
    times.modtime = (time_t)(mtime/1000.0);
    
    if (utime(PATH_OR_DOT(path), &times) < 0)
        THROW("unable to set mtime of %s: %s", path, strerror(errno));

    return JS_undefined;
}
END

FUNCTION(FILE_FileIO, ARG_UTF8_CAST(path))
{
    NWObject IO = GET_OBJECT(io, "IO");
    NWObject mode_fn = GET_OBJECT(NW_Exports, "mode");
    
    ARGS_ARRAY(argv, ARGV(1));
    NWObject modeObject = TO_OBJECT(CALL_AS_FUNCTION(mode_fn, JS_GLOBAL, 1, argv));
    
    int readFlag = GET_BOOL(modeObject, "read");
    int writeFlag = GET_BOOL(modeObject, "write");
    int appendFlag = GET_BOOL(modeObject, "append");
    int updateFlag = GET_BOOL(modeObject, "update");
    
    DEBUG("path=%s readFlag=%d writeFlag=%d readFlag=%d readFlag=%d\n", path, readFlag, writeFlag, appendFlag, updateFlag);
    
    int oflag = 0;
    
    if (readFlag && writeFlag)
        oflag = oflag | O_RDWR;
    else if (writeFlag)
        oflag = oflag | O_WRONLY;
    else if (readFlag)
        oflag = oflag | O_RDONLY;
        
    if (appendFlag)
        oflag = oflag | O_APPEND;
    // if (!updateFlag)
    //     oflag = oflag | O_TRUNC;
    
    if (updateFlag) {
        THROW("Updating IO not yet implemented.");
    } else if (writeFlag || appendFlag) {
        oflag = oflag | O_CREAT;
        if (!(oflag & O_APPEND))
            oflag = oflag | O_TRUNC;

        int fd = open(path, oflag, 0644);
        DEBUG("fd=%d\n", fd);
        if (fd < 0)
            THROW("%s", strerror(errno));

        ARGS_ARRAY(argv, JS_int(-1), JS_int(fd));
        return TO_OBJECT(CALL_AS_CONSTRUCTOR(IO, 2, argv));
    } else if (readFlag) {

        int fd = open(path, oflag);
        DEBUG("fd=%d\n", fd);
        if (fd < 0)
            THROW("%s", strerror(errno));

        ARGS_ARRAY(argv, JS_int(fd), JS_int(-1));
        return CALL_AS_CONSTRUCTOR(IO, 2, argv);
    } else {
        THROW("Files must be opened either for read, write, or update mode.");
    }
    
    return JS_undefined;
}
END


NARWHAL_MODULE(file_engine)
{
    NW_Exports = PROTECT_OBJECT(NW_require("./file"));
    io = PROTECT_OBJECT(NW_require("io"));
    
    EXPORTS("FileIO", JS_fn(FILE_FileIO));
    
    EXPORTS("cwd", JS_fn(FILE_cwd));
    //EXPORTS("canonical", JS_fn(FILE_canonical));
    EXPORTS("canonicalImpl", JS_fn(FILE_canonicalImpl));
    EXPORTS("mtime", JS_fn(FILE_mtime));
    EXPORTS("size", JS_fn(FILE_size));
    EXPORTS("stat", JS_fn(FILE_stat));
    EXPORTS("exists", JS_fn(FILE_exists));
    EXPORTS("linkExists", JS_fn(FILE_linkExists));
    EXPORTS("isDirectory", JS_fn(FILE_isDirectory));
    EXPORTS("isFile", JS_fn(FILE_isFile));
    EXPORTS("isLink", JS_fn(FILE_isLink));
    EXPORTS("isReadable", JS_fn(FILE_isReadable));
    EXPORTS("isWritable", JS_fn(FILE_isWritable));
    EXPORTS("chmod", JS_fn(FILE_chmod));
    EXPORTS("chown", JS_fn(FILE_chown));
    EXPORTS("link", JS_fn(FILE_link));
    EXPORTS("symlink", JS_fn(FILE_symlink));
    //EXPORTS("rename", JS_fn(FILE_rename));
    //EXPORTS("move", JS_fn(FILE_rename));
    EXPORTS("renameImpl", JS_fn(FILE_renameImpl));
    //EXPORTS("move", JS_fn(FILE_move));
    EXPORTS("remove", JS_fn(FILE_remove));
    EXPORTS("mkdir", JS_fn(FILE_mkdir));
    //EXPORTS("mkdirs", JS_fn(FILE_mkdirs));
    EXPORTS("rmdir", JS_fn(FILE_rmdir));
    //EXPORTS("listImpl", JS_fn(FILE_list));
    EXPORTS("list", JS_fn(FILE_list));
    EXPORTS("touchImpl", JS_fn(FILE_touchImpl));
    //EXPORTS("touch", JS_fn(FILE_touch));
    
    NWObject file_engine_js = NW_require("file-engine.js");
    EXPORTS("mkdirs", GET_VALUE(file_engine_js, "mkdirs"));
    EXPORTS("touch", GET_VALUE(file_engine_js, "touch"));
    EXPORTS("rename", GET_VALUE(file_engine_js, "rename"));
    EXPORTS("move", GET_VALUE(file_engine_js, "rename"));
    EXPORTS("canonical", GET_VALUE(file_engine_js, "canonical"));
}
END_NARWHAL_MODULE

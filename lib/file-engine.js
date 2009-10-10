var file = require("file");

exports.mkdirs = function(path) {
    var components = file.Path(path).split();
    for (var i = 0; i < components.length; i++) {
        var dir = file.join.apply(null, components.slice(0, i+1));
        if (!file.isDirectory(dir))
            file.mkdir(dir);
    }
}

exports.touch = function (path, mtime) {
    if (mtime === undefined || mtime === null)
        mtime = new Date();
        
    if (!file.exists(path))
        file.write(path, "");
        
    return file.touchImpl(path, mtime.getTime());
};

exports.rename = function(source, target) {
    source = file.path(source);
    target = source.resolve(target);
    
    return file.renameImpl(source, target);
};

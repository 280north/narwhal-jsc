(function (evalGlobal, global) {
    var debug = true;

    var prefix = ENV['NARWHAL_HOME'];
    var enginePrefix = ENV['NARWHAL_ENGINE_HOME'];

    var _isFile = isFile, _read = read, _print = print;
    delete read, isFile, print;
    
    function NativeLoader() {
        var loader = {};
        var factories = {};
        
        loader.reload = function(topId, path) {
            factories[topId] = requireNative(topId, path);
        }
        
        loader.load = function(topId, path) {
            if (!factories.hasOwnProperty(topId))
                loader.reload(topId, path);
            return factories[topId];
        }
        
        return loader;
    };

    var narwhal = eval(_read(prefix + "/narwhal.js"));
    
    narwhal({
        global: global,
        evalGlobal: evalGlobal,
        engine: 'jsc',
        engines: ['jsc', 'c', 'default'],
        prefix: prefix,
        prefixes: [enginePrefix, prefix],
        debug: debug,
        print: function (string) { _print(String(string)); },
        evaluate: function (text) {
            return eval("(function(require,exports,module,system,print){" + text + "/**/\n})");
        },
        fs: {
            read: function(path) { return _read(path); },
            isFile: function(path) { return _isFile(path); }
        },
        loaders: [[".dylib", NativeLoader()]],
        os : "darwin",
        debug: ENV['NARWHAL_DEBUG'],
        verbose: ENV['NARWHAL_VERBOSE']
    });

})(function () {
    return eval(arguments[0]);
}, this);

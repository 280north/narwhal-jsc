(function bootstrap(evalGlobal, global) {
    var env = ENV;
    var args = ARGS;

    function envEnabled(name) {
        return env[name] && parseInt(env[name]) !== 0;
    }

    var profiling = false;
    if (typeof _inspector !== "undefined" && envEnabled("NARWHAL_PROFILE")) {
        profiling = true;
        _inspector.startProfilingJavaScript_();
    }

    if (typeof _inspector !== "undefined" && envEnabled("NARWHAL_DEBUGGER")) {
        _inspector.show_();
        _inspector.startDebuggingJavaScript_();
    }

    var runShell = envEnabled("NARWHAL_RUN_SHELL") && args.length <= 1;

    var prefix = env['NARWHAL_HOME'];
    var enginePrefix = env['NARWHAL_ENGINE_HOME'];

    var debug = envEnabled("NARWHAL_DEBUG");
    var verbose = envEnabled("NARWHAL_DEBUG");

    var _isFile = isFile, _read = read, _print = print;
    delete read, isFile, print;

    function NativeLoader() {
        var loader = {};
        var factories = {};

        loader.reload = function reload(topId, path) {
            factories[topId] = requireNative(topId, path);
        }

        loader.load = function load(topId, path) {
            if (!factories.hasOwnProperty(topId))
                loader.reload(topId, path);
            return factories[topId];
        }

        return loader;
    };

    var sourceURLTag = "\n//@ sourceURL="+prefix+"/narwhal.js";
    var narwhal = evalGlobal(_read(prefix + "/narwhal.js") + "/**/" + sourceURLTag);
    narwhal.displayName = "narwhal";

    try {
        narwhal({
            global: global,
            evalGlobal: evalGlobal,
            engine: 'jsc',
            engines: ['jsc', 'c', 'default'],
            prefix: prefix,
            prefixes: [enginePrefix, prefix],
            print: function print(string) { _print(String(string)); },
            evaluate: function evaluate(text, fileName, lineNumber) {
                var sourceURLTag = "\n//@ sourceURL=" + fileName;
                //return new Function("require", "exports", "module", "system", "print", text+"/**/"+sourceURLTag);
                return eval("(function(require,exports,module,system,print){" + text + "/**/\n})"+sourceURLTag);
            },
            fs: {
                read: function read(path) { return _read(path); },
                isFile: function isFile(path) { return _isFile(path); }
            },
            loaders: [[".dylib", NativeLoader()]],
            os : "darwin",
            debug: debug,
            verbose: verbose
        });

    } catch (e) {
        if (e && (e.line || e.sourceURL))
            print("Error on line " + (e.line || "[unknown]") + " of file " + (e.sourceURL || "[unknown]"));
        throw e;
    }

    if (profiling) {
        _inspector.stopProfilingJavaScript_();
    }

    if (runShell) {
        print("Narwhal version "+env["NARWHAL_VERSION"]+", JavaScriptCore engine");
        while (true) {
            try {
                var line = require("readline").readline("> ");
                var result = evalGlobal(line);
                print(result);
            } catch (e) {
                print(e);
            }
        }
    }

})(function evalGlobal() {
    return eval(arguments[0]);
}, this);

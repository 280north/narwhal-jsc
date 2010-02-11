var exports = require("interpreter");

var FILE = require("file");

var Context = exports.Context = function() {
    var self = this;
    
    self._context = exports.JSGlobalContextCreate();
    self._context.toString = function() { return "[object Context]"; };
    self.global = exports.JSContextGetGlobalObject(self._context);
    self.global.toString = function() { return "[object Global]"; };
    
    this.eval = function(source) {
        source = source || "";
        var sourceURL = findSourceURL(source) || "<eval>";
    
        return exports.JSEvaluateScript(
            self._context,
            source,
            self.global,
            sourceURL,
            1
        );
    };
    
    self.evalFile = function(sourceURL) {
        var source = FILE.read(sourceURL, { charset : "UTF-8" });
        
        return exports.JSEvaluateScript(
            self._context,
            source,
            self.global,
            sourceURL,
            1
        );
    };
    
    self.Function = function() {
        var args = Array.prototype.slice.call(arguments);
        var body = args.pop() || "";
        var source = "(function("+args.join(",")+"){"+body+"/**/\n})";
        var sourceURL = findSourceURL(body) || "<function>";
        
        return exports.JSEvaluateScript(
            self._context,
            source,
            self.global,
            sourceURL,
            1
        );
    };
    
    return self;
};

function findSourceURL(source) {
    // based on https://bugs.webkit.org/show_bug.cgi?id=25475#c4
    var match = source.match(/.*\s*\/\/\s*@\s*sourceURL\s*=\s*(\S+)\s*/);
    if (match)
        return match[1];
    return null;
}

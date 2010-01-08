var exports = require("os-engine");

var IO = require("io");
var OS = require("os");

exports.system = function (command) {
    if (Array.isArray(command)) {
        command = command.map(OS.enquote).join(" ");
    }
    
    return exports.systemImpl(command);
};

exports.popen = function (command, options) {
    options = options || {};

    if (Array.isArray(command)) {
        command = command.map(OS.enquote).join(" ");
    }
    
    var result = exports.popenImpl(command);
    
    result.stdin = new IO.TextOutputStream(result.stdin,
        options.buffering, options.lineBuffering, options.charset);
    result.stdout = new IO.TextInputStream(result.stdout, options.buffering,
        options.lineBuffering, options.charset);
    result.stderr = new IO.TextInputStream(result.stderr,
        options.buffering, options.lineBuffering, options.charset);
    
    return result;
};

exports.exit = function(status) {
    exports.exitImpl(status << 0);
}
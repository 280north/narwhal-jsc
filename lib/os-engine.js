var exports = require("os-engine");

var io = require("io");

exports.system = function (command) {
    if (Array.isArray(command)) {
        command = command.map(function (arg) {
            return require("os").enquote(arg);
        }).join(" ");
    }
    return exports.systemImpl(command);
};

exports.popen = function (command) {
    if (Array.isArray(command)) {
        command = command.map(function (arg) {
            return require("os").enquote(arg);
        }).join(" ");
    }
    
    var result = exports.popenImpl(command);
    
    result.stdin = new io.TextOutputStream(result.stdin);
    result.stdout = new io.TextInputStream(result.stdout);
    result.stderr = new io.TextInputStream(result.stderr);
    
    return result;
};
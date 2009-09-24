var exports = require("os-engine");

exports.system = function (command) {
    if (Array.isArray(command)) {
        command = command.map(function (arg) {
            return require("os").enquote(arg);
        }).join(" ");
    }
    return exports.systemImpl(command);
};
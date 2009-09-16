
var io = require("./io");

exports.stdin  = new io.TextInputStream(new io.IO(io.STDIN_FILENO, -1));
exports.stdout = new io.TextOutputStream(new io.IO(-1, io.STDOUT_FILENO));
exports.stderr = new io.TextOutputStream(new io.IO(-1, io.STDERR_FILENO));

exports.args = ARGS;
delete ARGS;

exports.env = ENV;
delete ENV;

exports.fs = require('./file');

// default logger
var Logger = require("logger").Logger;
exports.log = new Logger(exports.stdout);

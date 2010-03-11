
// -- tlrobinson Tom Robinson

var FILE = require("file");
var OS = require("os");

var ByteIO = require("io").ByteIO;
var ByteString = require("binary").ByteString;

// this fails on
// exports.open = function (url, mode, options) {
//     var req = new XMLHttpRequest();
//     req.open("GET", url, false);
//     req.overrideMimeType('text/plain; charset=x-user-defined'); // so we can read as binary
//     req.send();
//
//     // FIXME
//     if (req.status >= 300)
//         throw new Error("HTTP error: " + req.status);
//
//     var length = req.responseText.length;
//     var byteArray = Array(length);
//     for (var i = 0; i < length; i++)
//         byteArray[i] = req.responseText.charCodeAt(i) & 0xFF;
//     var bytes = new ByteString(byteArray);
//
//     return new ByteIO(bytes);
// };

exports.open = function (url, mode, options) {
    var p = OS.popen(["curl", "-vv", "-L", url]);

    p.stdin.close();

    var line;
    while (line = p.stderr.readLine()) {
        var match;
        if (match = line.match(/< HTTP\/([^\s]+)\s+(\d+)\s*(.*)/)) {
            var version = match[1];
            var status = parseInt(match[2], 10);
            var statusText = match[3];

            if (status < 300)
                break;
            else if (status >= 400)
                throw new Error("HTTP status: " + status);
        }
    }

    // FIXME: for some reason if we return p.stdout.raw directly sometimes the file is truncated.
    var data = p.stdout.raw.read();

    p.stdout.close();
    p.stderr.close();

    return new ByteIO(data);
};

exports.read = function (url) {
    var stream = exports.open(url);
    try {
        return stream.read();
    } finally {
        stream.close();
    }
};

// TODO resolve the source as a file URL
exports.copy = function (source, target, mode) {
    mode = mode || 'b';
    return FILE.path(target).write(exports.read(source, mode), mode);
};

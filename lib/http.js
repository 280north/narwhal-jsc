
// -- tlrobinson Tom Robinson

var FILE = require("file");
var OS = require("os");

var ByteString = require("binary").ByteString;

exports.open = function (url, mode, options) {
    mode = mode || "b";
    options = options || {};

    var headers = [];
    for (var name in options.headers) {
        headers.push("-H", name+": "+options.headers[name]);
    }

    var method = options.method || "GET";
    var p = OS.popen(["curl", "-s", "-vv", "--data-binary", "@-", "-L", "-X", method].concat(headers, url));

    var state = 0;

    var request = {
        status: null,
        headers: {},
        read : function() {
            if (state === 0) {
                state = 1;
                p.stdin.close();
                var line;
                while (line = p.stderr.readLine()) {
                    var match;
                    if (state === 1 && (match = line.match(/^< HTTP\/([^\s]+)\s+(\d+)\s*(.*)/))) {
                        var version = match[1];
                        var status = parseInt(match[2], 10);
                        var statusText = match[3];

                        if (status < 300) {
                            state = 2;
                            request.status = status;
                            request.statusText = statusText;
                        }
                        else if (status >= 400) {
                            throw new Error("HTTP status: " + status);
                        }
                    } else if (state === 2) {
                        var match;
                        if (match = line.match(/^<\s*([^:]+):\s+(.+)/)) {
                            var name = match[1];
                            var value = match[2];
                            request.headers[name] = value;
                        } else {
                            break;
                        }
                    }
                }
            }
            return p.stdout.raw.read.apply(p.stdout.raw, arguments);
        },
        write : function() {
            p.stdin.raw.write.apply(p.stdin.raw, arguments);
            return this;
        },
        close : function() {
            p.stdout.raw.close();
            p.stdin.raw.close();
            return this;
        }
    };
    return request;
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

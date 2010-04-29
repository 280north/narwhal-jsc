
// -- tlrobinson Tom Robinson

var FILE = require("file");
var OS = require("os");
var IO = require("io").IO;


exports.open = function (url, mode, options) {
    mode = mode || "b";
    options = options || {};

    options.headers = options.headers || {};
    options.method = options.method || "GET";

    var command = ["curl", "-s", "-vv", "--data-binary", "@-", "-L", "-X", options.method];

    var hasContentType = false;
    var hasContentLength = false;

    for (var name in options.headers) {
        command.push("-H", name+": "+options.headers[name]);
        if (name.toLowerCase() === "content-length")
            hasContentType = true;
        if (name.toLowerCase() === "content-length")
            hasContentLength = true;
    }

    // unset Content-Length and Content-Type if they weren't explicitly set.
    if (!hasContentLength)
        command.push("-H", "Content-Length:");
    if (!hasContentType)
        command.push("-H", "Content-Type:");

    command.push(url);

    var p = OS.popen(command);

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
            var result = p.stdout.raw.read.apply(p.stdout.raw, arguments);
            // FIXME: is there a better way to detect EOF?
            if (arguments.length === 0 && result.length === 0) {
                var exitCode = p.wait();
                if (exitCode > 0) {
                    throw new Error(CURL_ERRORS[exitCode] || "Unknown curl error");
                }
            }
            return result;
        },
        write : function() {
            p.stdin.raw.write.apply(p.stdin.raw, arguments);
            return this;
        },
        flush : function() {
            p.stdin.raw.flush.apply(p.stdin.raw, arguments);
            return this;
        },
        close : function() {
            p.stdout.raw.close();
            p.stdin.raw.close();
            return this;
        },
        copy : IO.prototype.copy
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

// taken from curl manpage (http://curl.haxx.se)
var CURL_ERRORS = {
    "1": "Unsupported protocol. This build of curl has no support for this protocol.",
    "2": "Failed to initialize.",
    "3": "URL malformed. The syntax was not correct.",
    "5": "Couldn't resolve proxy. The given proxy host could not be resolved.",
    "6": "Couldn't resolve host. The given remote host was not resolved.",
    "7": "Failed to connect to host.",
    "18": "Partial file. Only a part of the file was transferred.",
    "22": "HTTP page not retrieved. The requested url was not found or returned another error with the HTTP error code being 400 or above.  This  return  code only appears if -f/--fail is used.",
    "23": "Write error. Curl couldn't write data to a local filesystem or similar.",
    "26": "Read error. Various reading problems.",
    "27": "Out of memory. A memory allocation request failed.",
    "28": "Operation timeout. The specified time-out period was reached according to the conditions.",
    "33": "HTTP range error. The range \"command\" didn't work.",
    "34": "HTTP post error. Internal post-request generation error.",
    "35": "SSL connect error. The SSL handshaking failed.",
    "37": "FILE couldn't read file. Failed to open the file. Permissions?",
    "42": "Aborted by callback. An application told curl to abort the operation.",
    "43": "Internal error. A function was called with a bad parameter.",
    "45": "Interface error. A specified outgoing interface could not be used.",
    "47": "Too many redirects. When following redirects, curl hit the maximum amount.",
    "51": "The peer's SSL certificate or SSH MD5 fingerprint was not ok.",
    "52": "The server didn't reply anything, which here is considered an error.",
    "53": "SSL crypto engine not found.",
    "54": "Cannot set SSL crypto engine as default.",
    "55": "Failed sending network data.",
    "56": "Failure in receiving network data.",
    "58": "Problem with the local certificate.",
    "59": "Couldn't use specified SSL cipher.",
    "60": "Peer certificate cannot be authenticated with known CA certificates.",
    "61": "Unrecognized transfer encoding.",
    "63": "Maximum file size exceeded.",
    "65": "Sending the data requires a rewind that failed.",
    "66": "Failed to initialise SSL Engine.",
    "67": "The user name, password, or similar was not accepted and curl failed to log in.",
    "75": "Character conversion failed.",
    "76": "Character conversion functions required.",
    "77": "Problem with reading the SSL CA cert (path? access rights?).",
    "78": "The resource referenced in the URL does not exist.",
    "79": "An unspecified error occurred during the SSH session.",
    "80": "Failed to shut down the SSL connection.",
    "82": "Could not load CRL file, missing or wrong format.",
    "83": "Issuer check failed."
};

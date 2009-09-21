var exports = require("io-engine");
///*
exports.IO.prototype.copy = function (output, mode, options) {
    while (true) {
        var buffer = this.read(null);
        if (!buffer.length)
            break;
        output.write(buffer);
    }
    output.flush();
    return this;
};
//*/
var TextIOWrapper = exports.TextIOWrapper = function (raw, mode, lineBuffering, buffering, charset, options) {
    //print("wrappin': " + [raw, mode, lineBuffering, buffering, charset, options].join(","))
    
    if (mode.update) {
        return new exports.TextIOStream(raw, lineBuffering, buffering, charset, options);
    } else if (mode.write || mode.append) {
        return new exports.TextOutputStream(raw, lineBuffering, buffering, charset, options);
    } else if (mode.read) {
        return new exports.TextInputStream(raw, lineBuffering, buffering, charset, options);
    } else {
        throw new Error("file must be opened for read, write, or append mode.");
    }
};

///*
var TextInputStream = exports.TextInputStream/* = exports.TextInputStream || function (raw, lineBuffering, buffering, charset, options) {
    print(Array.prototype.slice.call(arguments).join(","));
    this.raw = raw;
    this.charset = charset;
}*/

TextInputStream.prototype._readLine = TextInputStream.prototype._readLine || function () {
    var buffer = [],
        last;
    
    do {
        last = this.read(1);
        buffer.push(last);
    } while (last !== "\n" && last !== "");
    
    return buffer.join("").slice(0, -1);
};

TextInputStream.prototype.readLine = TextInputStream.prototype.readLine || function () {
    var line = this._readLine();
    return line ? line + "\n" : "";
}

TextInputStream.prototype.next = TextInputStream.prototype.next || function () {
    var line = this._readLine();
    if (!line.length)
        throw StopIteration;
    return line;
};

TextInputStream.prototype.iterator = TextInputStream.prototype.iterator || function () {
    return this;
};

TextInputStream.prototype.forEach = TextInputStream.prototype.forEach || function (block, context) {
    var line;
    while (true) {
        try {
            line = this.next();
        } catch (exception) {
            break;
        }
        block.call(context, line);
    }
};

TextInputStream.prototype.input = TextInputStream.prototype.input || function () {
    throw "NYI";
};

TextInputStream.prototype.readLines = TextInputStream.prototype.readLines || function () {
    var lines = [];
    do {
        var line = this.readLine();
        if (line.length)
            lines.push(line);
    } while (line.length);
    return lines;
};

TextInputStream.prototype.read = TextInputStream.prototype.read || function () {
    print("this.charset="+this.charset)
    return this.raw.read().decodeToString(this.charset);
};

TextInputStream.prototype.readInto = TextInputStream.prototype.readInto || function (buffer) {
    throw "NYI";
};

TextInputStream.prototype.copy = TextInputStream.prototype.copy || function (output, mode, options) {
    do {
        var line = this.readLine();
        output.write(line).flush();
    } while (line.length);
    return this;
};

TextInputStream.prototype.close = TextInputStream.prototype.close || function () {
    this.raw.close();
};
//*/

var TextOutputStream = exports.TextOutputStream = function (raw, lineBuffering, buffering, charset, options) {
    this.raw = raw;
    this.charset = charset;
}

TextOutputStream.prototype.write = function (str) {
    this.needsNewline = !(/\n$/).test(str);
    this.raw.write(str.toByteString(this.charset))
    return this;
};

TextOutputStream.prototype.writeLine = function (line) {
    this.write(line + "\n"); // todo recordSeparator
    return this;
};

TextOutputStream.prototype.writeLines = function (lines) {
    lines.forEach(this.writeLine, this);
    return this;
};

TextOutputStream.prototype.print = function () {
    this.write(Array.prototype.join.call(arguments, " ") + "\n");
    this.flush();
    // todo recordSeparator, fieldSeparator
    return this;
};

TextOutputStream.prototype.flush = function () {
    this.raw.flush();
    return this;
};

TextOutputStream.prototype.close = function () {
    //if (this.needsNewline) this.writeLine("");
        
    this.raw.close();
    return this;
};

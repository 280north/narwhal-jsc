var exports = require("io-engine"),
    ByteString = require("binary").ByteString,
    binary_engine = require("binary-engine"),
    B_ALLOC = binary_engine.B_ALLOC,
    B_COPY = binary_engine.B_COPY;

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

var ByteIO = exports.ByteIO = function(binary) {
    this.buffer = [];
    this.length = 0;
    
    if (binary) this.write(binary);
}

ByteIO.prototype.toByteString = function() {
    var result = this.read();

    // replace the buffer with the collapsed ByteString
    this.buffer = [result];
    this.length = result.length;

    return result;
}

ByteIO.prototype.decodeToString = function(charset) {
    return charset ? 
        this.toByteString().decodeToString(charset) :
        this.toByteString().decodeToString();
}

ByteIO.prototype.read = function(length) {
    if (typeof length !== "number") length = this.length;
    
    if (length <= 0)
        return new ByteString();
    
    var bytes = B_ALLOC(length),
        position = 0;
    
    while (position < length && this.length > 0) {
        var segment = this.buffer[0];
        
        var len = Math.min(segment._length, length - position);

        B_COPY(segment._bytes, segment._offset, bytes, position, len);
        
        position += len;
        this.length -= len;
        
        if (len < segment._length) {
            this.buffer[0] = segment.slice(len);
        } else {
            this.buffer.shift();
        }
    }
    
    return new ByteString(bytes, 0, position);
}

ByteIO.prototype.write = function(object) {
    var segment = object.toByteString();
    
    this.buffer.push(segment);
    this.length += segment.length;
    
    return this;
}

ByteIO.prototype.flush = function() {
    return this;
}

ByteIO.prototype.close = function() {
}

var TextIOWrapper = exports.TextIOWrapper = function (raw, mode, lineBuffering, buffering, charset, options) {
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
    return (typeof line === "string") ? line + "\n" : "";
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


var TextOutputStream = exports.TextOutputStream = function (raw, lineBuffering, buffering, charset, options) {
    this.raw = raw;
    this.charset = charset;
}

TextOutputStream.prototype.write = function (str) {
    this.raw.write(str.toByteString(this.charset));
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
    this.raw.close();
    return this;
};

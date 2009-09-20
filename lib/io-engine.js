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

var TextInputStream = exports.TextInputStream = function (raw, lineBuffering, buffering, charset, options) {
    this.raw = raw;
    this.charset = charset;
}

TextInputStream.prototype.readLine = function () {
    throw "NYI";
};

TextInputStream.prototype.next = function () {
    throw "NYI";
};

TextInputStream.prototype.iterator = function () {
    throw "NYI";
};

TextInputStream.prototype.forEach = function (block, _context) {
    throw "NYI";
};

TextInputStream.prototype.input = function () {
    throw "NYI";
};

TextInputStream.prototype.readLines = function () {
    return this.read().match(/^.+(\r\n|\r|\n|$)/mg);
};

TextInputStream.prototype.read = function () {
    return this.raw.read().decodeToString(this.charset);
};

TextInputStream.prototype.readInto = function (buffer) {
    throw "NYI";
};

TextInputStream.prototype.copy = function (output, mode, options) {
    throw "NYI";
};

TextInputStream.prototype.close = function () {
    this.raw.close();
};


var TextOutputStream = exports.TextOutputStream = function (raw, lineBuffering, buffering, charset, options) {
    this.raw = raw;
    this.charset = charset;
}

TextOutputStream.prototype.write = function (str) {
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
    this.raw.close();
    return this;
};

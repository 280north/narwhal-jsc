exports.TextIOWrapper = function (raw, mode, lineBuffering, buffering, charset, options) {
    print("wrappin': " + [raw, mode, lineBuffering, buffering, charset, options].join(","))
    for (var i in raw)
        print(" --> " + i)
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

exports.TextInputStream = function (raw, lineBuffering, buffering, charset, options) {
    var self = this;

    self.raw = raw;

    self.readLine = function () {
        throw "NYI";
    };

    self.next = function () {
        throw "NYI";
    };

    self.iterator = function () {
        throw "NYI";
    };

    self.forEach = function (block, context) {
        throw "NYI";
    };

    self.input = function () {
        throw "NYI";
    };

    self.readLines = function () {
        throw "NYI";
    };

    self.read = function () {
        return raw.read().decodeToString(charset);
    };

    self.readInto = function (buffer) {
        throw "NYI";
    };

    self.copy = function (output, mode, options) {
        throw "NYI";
    };

    self.close = function () {
        raw.close();
    };

    return Object.create(self);
};

exports.TextOutputStream = function (raw, lineBuffering, buffering, charset, options) {
    var self = this;

    self.raw = raw;

    self.write = function (str) {
        raw.write(str.toByteString(charset))
        return self;
    };

    self.writeLine = function (line) {
        self.write(line + "\n"); // todo recordSeparator
        return self;
    };

    self.writeLines = function (lines) {
        lines.forEach(self.writeLine);
        return self;
    };

    self.print = function () {
        self.write(Array.prototype.join.call(arguments, " ") + "\n");
        self.flush();
        // todo recordSeparator, fieldSeparator
        return self;
    };

    self.flush = function () {
        raw.flush();
        return self;
    };

    self.close = function () {
        raw.close();
        return self;
    };

    return Object.create(self);
};
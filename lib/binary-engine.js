var impl = require("binary-engine");

exports.B_DECODE = function(bytes, offset, length, codec) {
    var newBytes = (codec === impl.DEFAULT_CODEC) ? bytes : impl.B_TRANSCODE(bytes, offset, length, codec, impl.DEFAULT_CODEC);
    return impl.B_DECODE_DEFAULT(newBytes, 0, impl.B_LENGTH(newBytes));
}

exports.B_ENCODE = function(string, codec) {
    var bytes = impl.B_ENCODE_DEFAULT(string);
    return (codec === impl.DEFAULT_CODEC) ? bytes : impl.B_TRANSCODE(bytes, 0, impl.B_LENGTH(bytes), impl.DEFAULT_CODEC, codec);
}

var mongoose = require("mongoose");

exports.run = function(app, options) {
    mongoose.mg_start(String(options.port || "8080"), app);
}

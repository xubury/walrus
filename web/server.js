var express = require("express");
var path = require("path");

const app = express();
const port = process.env.PORT || 8080;

// TODO:filesystem emu
app.get("/file/:name", function(req, res) {
    // console.log(req.params.name);
    res.send("world");
})

app.get("/", function (req, res) {
    res.sendFile(path.join(__dirname, "res/index.html"));
});

app.get("/favicon.ico", function (req, res) {
    res.sendFile(path.join(__dirname, "res/favicon.ico"));
});

app.get("/wasm", function (req, res) {
    res.sendFile(path.join(__dirname, "res/main.wasm"));
});

app.get("/wasm.js", function (req, res) {
    res.sendFile(path.join(__dirname, "src/wasm.js"));
});

app.get("/sys.js", function (req, res) {
    res.sendFile(path.join(__dirname, "src/sys.js"));
});

app.get("/gl.js", function (req, res) {
    res.sendFile(path.join(__dirname, "src/gl.js"));
});

const { address } = require("ip");

app.listen(port, function () {
    console.log("Server started at:");
    console.log("http://localhost:" + port);
    console.log("http://" + address("private") + ":" + port);
    console.log("http://" + address("public") + ":" + port);
});

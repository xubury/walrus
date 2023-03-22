var express = require("express");
var bodyParser = require("body-parser");
var path = require("path");
var fs = require("fs");

const app = express();
const port = process.env.PORT || 8080;

// create application/json parser
app.use(bodyParser.json());

const dir = fs.openSync(path.join(__dirname, "files"))

var fd_cursors = [];
var fd_payloads = [];
app.get("/fd_open", function (req, res) {
    const filename = path.join(__dirname, "files", req.query.filename);
    try {
        const fd = fs.openSync(filename);
        console.log("[TRACE] fd_open on fd:" + fd);
        res.send(JSON.stringify({ fd: fd }));
        fd_cursors[fd] = 0;
        fs.read(fd, function (err, bytesRead, buffer) {
            fd_payloads[fd] = buffer;
        });
    } catch (error) {
        console.log(error);
        res.status(500).send(JSON.stringify({ error: error }));
    }
});

app.get("/fd_read", function (req, res) {
    const fd = Number(req.query.fd);
    const length = Number(req.query.length);
    console.log("[TRACE] fd_read on fd: " + fd + " length: " + length);

    var payload = fd_payloads[fd];
    var cursor = fd_cursors[fd];
    var read = Math.min(length, payload.length - cursor);
    payload = payload.subarray(cursor, cursor + read);
    fd_cursors[fd] += read;
    res.send(JSON.stringify({payload:payload}));
});

app.get("/fd_close", function (req, res) {
    const fd = Number(req.query.fd);
    console.log("[TRACE] fd_close on fd: " + fd);
    fs.closeSync(fd);
});

app.get("/fd_seek", function (req, res) {
    const fd = Number(req.query.fd);
    const offset = Number(req.query.offset);
    const whence = Number(req.query.whence);
    console.log(
        "[TRACE] fd_seek on fd: " +
            fd +
            " offset: " +
            offset +
            " whence: " +
            whence
    );
    if (whence == 0) fd_cursors[fd] = offset; //set
    if (whence == 1) fd_cursors[fd] += offset; //cur
    if (whence == 2) fd_cursors[fd] = fd_payloads[fd].length - offset; //end
    res.sendStatus(200);
});

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

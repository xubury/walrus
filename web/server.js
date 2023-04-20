var express = require("express");
var bodyParser = require("body-parser");
var path = require("path");
var fs = require("fs");
const args = require('yargs').argv;

const app = express();
const port = process.env.PORT || 8080;

// create application/json parser
app.use(bodyParser.json());

const dir = path.join(__dirname, args.dir);
fs.openSync(dir);

function sendError(res, error)
{
    console.log(error);
    res.status(500).send(JSON.stringify({ error: error }));
}
app.get("/fd_open", function (req, res) {
    const filename = path.join(dir, req.query.filename);
    try {
        fs.open(filename, 'r', function(err, fd) {
            if (err)  {
                sendError(res, err);
                return;
            }

            console.log("[TRACE] fd_open on fd: " + fd);

            fs.fstat(fd, function(err, stats) {
                if (err)  {
                    sendError(res, err);
                    fs.close(fd);
                    return;
                }

                var buffer = Buffer.alloc(stats.size);
                fs.read(fd, buffer, 0, buffer.length, 0, function(err, bytesRead){
                    if (err)  {
                        sendError(res, err);
                        fs.close(fd);
                        return;
                    }

                    res.send(JSON.stringify({ fd : fd, payload: buffer }));
                });
            });
        });
    } catch (error) {
        sendError(res, error);
    }
});

app.get("/fd_close", function (req, res) {
    const fd = Number(req.query.fd);
    console.log("[TRACE] fd_close on fd: " + fd);
    fs.closeSync(fd);
    res.sendStatus(200)
});

function getWasm(filename)
{
    filename = path.join(__dirname, "res", filename);
    return filename;
}

app.get("/", function (req, res) {
    if (req.query.wasm != null && fs.existsSync(getWasm(req.query.wasm))) {
        fs.readFile(path.join(__dirname, "res/index.html"), function(err, buffer) {
            if (err == null) {
                var str = String(buffer);
                str = str.replace("%WASM%", req.query.wasm);
                res.setHeader('Content-Type', 'text/html');
                res.send(Buffer.from(str));
            } else {
                res.status(404).send(err);
            }
        })
    } else {
        res.status(404).send("Invalid wasm!");
    }
});

app.get("/favicon.ico", function (req, res) {
    res.sendFile(path.join(__dirname, "res/favicon.ico"));
});

app.get("/wasm", function (req, res) {
    res.sendFile(getWasm(req.query.filename));
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

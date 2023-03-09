import express from 'express';
import path from 'path';


const __dirname = path.resolve()

const app = express();
const port = process.env.PORT || 8080;

// Node.js built-in wasi
// import { WASI } from "wasi";
// import {argv, env } from 'node:process';
// const wasi = new WASI({
//       args: argv,
//       env,
//       preopens: {
//               '/sandbox': path.join(__dirname, "res"),
//             },
// });

app.get('/', function(req, res) {
    res.sendFile(path.join(__dirname, 'views/webasm_demo.html'));
});

app.get('/webasm_demo.wasm', function(req, res) {
  res.sendFile(path.join(__dirname, 'wasm/webasm_demo.wasm'));
});

app.get('/wasm.js', function(req, res) {
  res.sendFile(path.join(__dirname, '/src/wasm.js'));
});

app.get('/sys.js', function(req, res) {
  res.sendFile(path.join(__dirname, '/src/sys.js'));
});

app.get('/favicon.ico', function(req, res) {
  res.sendFile(path.join(__dirname, '/res/favicon.ico'));
});

app.listen(port, function() {
    console.log('Server started at http://localhost:' + port);
});

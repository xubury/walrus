import { initSys } from "/sys.js";
import { importGl } from "/gl.js";

var res = await fetch("/wasm");
var wasmBytes = await res.arrayBuffer();
await initSys(wasmBytes, loadLibraries);

// TODO:filesystem emu
res = await fetch("/file/hello");
var enc = new TextDecoder("utf-8");
console.log(enc.decode(await res.arrayBuffer()))

function loadLibraries(env) {
    console.log("[WASMJS] loadLibraies");
    importGl(env);
}

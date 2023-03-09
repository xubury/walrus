import { initSys } from "/sys.js";

var res = await fetch(WA.filename)
var wasmBytes = await res.arrayBuffer()
await initSys(wasmBytes, loadLibraries)

function loadLibraries(env) {
    // TODO: import gl function here
    console.log("[WASMJS] loadLibraies");
}

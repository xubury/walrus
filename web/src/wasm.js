import { initSys } from "/sys.js";
import { importGl } from "/gl.js";

var res = await fetch("/wasm");
var wasmBytes = await res.arrayBuffer();
await initSys(wasmBytes, loadLibraries);

function loadLibraries(env) {
    console.log("[WASMJS] loadLibraies");
    importGl(env);
}

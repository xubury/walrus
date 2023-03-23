import { initSys } from "/sys.js";
import { importGl } from "/gl.js";


const res = await fetch("/wasm");
const wasmBytes = await res.arrayBuffer();
initSys(wasmBytes, loadLibraries);

function loadLibraries(env) {
    console.log("[WAJS] loadLibraies");
    importGl(env);
}

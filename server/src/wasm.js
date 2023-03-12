import { initSys } from "/sys.js";
import { initGl as importGlLibrary } from "/gl.js";

var res = await fetch(WA.filename);
var wasmBytes = await res.arrayBuffer();
await initSys(wasmBytes, loadLibraries);

function loadLibraries(env) {
    console.log("[WASMJS] loadLibraies");
    importGlLibrary(env);
}

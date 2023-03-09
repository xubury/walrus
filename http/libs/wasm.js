import { initSys } from "./sys.js";

fetch(WA.filename)
    .then((res) => res.arrayBuffer())
    .then((wasmBytes) => initSys(wasmBytes, loadLibraries));

function loadLibraries(env) {
    // TODO: import gl function here
    console.log("[WASMJS] loadLibraies");
}

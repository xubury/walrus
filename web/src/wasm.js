import { initSys } from "/sys.js";
import { importGl } from "/gl.js";

const res = await fetch("/wasm?" + new URLSearchParams({filename: WA.filename}));
if (res.ok) {
    const wasmBytes = await res.arrayBuffer();
    await initSys(wasmBytes, loadLibraries);

}

function loadLibraries(env) {
    console.log("[WAJS] loadLibraies");
    importGl(env);
}

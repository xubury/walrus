import { initSys } from "./sys.js";

fetch(WA.filename)
    .then((res) => res.arrayBuffer())
    .then(function (wasmBytes) {
        return initSys(wasmBytes, loadLibraries);
    })
    .catch(function (err) {
        // On an exception, if the err is 'abort' the error was already processed in the abort function above
        if (err !== "abort")
            abort(
                "BOOT",
                "WASM instiantate error: " +
                    err +
                    (err.stack ? "\n" + err.stack : "")
            );
    });

function loadLibraries(env) {
    // TODO: import gl function here
    console.log("[WASMJS] loadLibraies");
}

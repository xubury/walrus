document.addEventListener("DOMContentLoaded", main, false);

function wasmLoad(fileName) {
    var memory = new WebAssembly.Memory({ initial: 100, maximum: 1000 });
    heap = new Uint8Array(memory.buffer);

    var imports = {
        env: {
            console_log: function (arg) {
                console.log(arg);
            },
            memory: memory,
        },
    };

    var request = new XMLHttpRequest();
    request.open("GET", fileName);
    request.responseType = "arraybuffer";
    request.send();

    request.onload = function () {
        var wasmSource = request.response;
        var wasmModule = new WebAssembly.Module(wasmSource);
        var wasmInstance = new WebAssembly.Instance(wasmModule, imports);
        wasm = wasmInstance.exports;

        wasmLoadDone();
        console.log(memory);
    }; // XMLHttpRequest.onload()
} // loadWasm()

//===============================================================
function main() {
    wasmLoad("webasm_demo.wasm");
}

function wasmLoadDone() {
    wasm.run();
}

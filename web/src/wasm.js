import { initSys } from "/sys.js";
import { importGl } from "/gl.js";

function getMod(event)
{
    const KEYMOD_ALT   = 1 << 1;
    const KEYMOD_CTRL  = 1 << 2;
    const KEYMOD_SHIFT = 1 << 3;
    var mod = 0 
    if (event.altKey) {
        mod |= KEYMOD_ALT
    }
    if (event.ctrlKey) {
        mod |= KEYMOD_CTRL;
    }
    if (event.shiftKey) {
        mod |= KEYMOD_SHIFT
    }
    return mod;
}


const res = await fetch("/wasm");
const wasmBytes = await res.arrayBuffer();
await initSys(wasmBytes, loadLibraries);

if (WA.wasm.__on_mouse_move) {
    WA.canvas.addEventListener("mousemove", (event) => {
        const mod = getMod(event);
        WA.wasm.__on_mouse_move(event.clientX, event.clientY, event.movementX, event.movementY, mod)
    })
}


if (WA.wasm.__on_mouse_click) {
    WA.canvas.addEventListener("click", (event) => {
        const mod = getMod(event);
        WA.wasm.__on_mouse_click(event.button, event.clientX, event.clientY, event.movementX, event.movementY, mod)
    })
    WA.canvas.addEventListener("auxclick", (event) => {
        const mod = getMod(event);
        WA.wasm.__on_mouse_click(event.button, event.clientX, event.clientY, event.movementX, event.movementY, mod)
    })
}

if (WA.wasm.__on_mouse_down) {
    WA.canvas.addEventListener("mousedown", (event) => {
        const mod = getMod(event);
        WA.wasm.__on_mouse_down(event.button, event.clientX, event.clientY, event.movementX, event.movementY, mod)
    })
}
if (WA.wasm.__on_mouse_up) {
    WA.canvas.addEventListener("mouseup", (event) => {
        const mod = getMod(event);
        WA.wasm.__on_mouse_up(event.button, event.clientX, event.clientY, event.movementX, event.movementY, mod)
    })
}

function loadLibraries(env) {
    console.log("[WAJS] loadLibraies");
    importGl(env);
}

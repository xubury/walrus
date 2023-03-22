import { initSys } from "/sys.js";
import { importGl } from "/gl.js";

async function fd_open(filename) {
    // open a file, return a fd
    const res = await fetch(
        "/fd_open?" + new URLSearchParams({ filename: filename })
    );
    const body = await res.json();
    const fd = body.fd;
    const err = body.error;
    if (fd == null) {
        console.log(err);
        fd = 0;
    }
    return fd;
}

async function fd_read(fd, length) {
    var res = await fetch(
        "/fd_read?" +
            new URLSearchParams({
                fd: fd,
                length: length,
            })
    );
    var body = await res.json();
    console.log(new Uint8Array(body.payload.data));
}

// close file with a fd
async function fd_close(fd) {
    await fetch("/fd_close?" + new URLSearchParams({ fd: fd }));
}

async function fd_seek(fd, offset, whence) {
    await fetch(
        "/fd_seek?" +
            new URLSearchParams({ fd: fd, offset: offset, whence: whence })
    );
}

const res = await fetch("/wasm");
const wasmBytes = await res.arrayBuffer();
await initSys(wasmBytes, loadLibraries);

function loadLibraries(env) {
    console.log("[WAJS] loadLibraies");
    importGl(env);
}

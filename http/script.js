document.addEventListener("DOMContentLoaded", main, false);

var WA = {};

// Some global state variables and max heap definition
var HEAP32, HEAPU8, HEAPU16, HEAPU32, HEAPF32;
var WASM_MEMORY,
    WASM_HEAP,
    WASM_HEAP_MAX = 256 * 1024 * 1024; //max 256MB
// Define print and error functions if not yet defined by the outer html file
WA.print =
    WA.print ||
    function (msg) {
        console.log(msg);
    };
WA.error =
    WA.error ||
    function (code, msg) {
        WA.print("[ERROR] " + code + ": " + msg + "\n");
    };

// Set the array views of various data types used to read/write to the wasm memory from JavaScript
function MemorySetBufferViews() {
    var buf = WASM_MEMORY.buffer;
    HEAP32 = new Int32Array(buf);
    HEAPU8 = new Uint8Array(buf);
    HEAPU16 = new Uint16Array(buf);
    HEAPU32 = new Uint32Array(buf);
    HEAPF32 = new Float32Array(buf);
}

// Reads a string from the wasm memory heap to javascript (decoded as UTF8)
function ReadHeapString(ptr, length) {
    if (length === 0 || !ptr) return "";
    for (var hasUtf = 0, t, i = 0; !length || i != length; i++) {
        t = HEAPU8[(ptr + i) >> 0];
        if (t == 0 && !length) break;
        hasUtf |= t;
    }
    if (!length) length = i;
    if (hasUtf & 128) {
        for (
            var r = HEAPU8,
                o = ptr,
                p = ptr + length,
                F = String.fromCharCode,
                e,
                f,
                i,
                n,
                C,
                t,
                a,
                g = "";
            ;

        ) {
            if (o == p || ((e = r[o++]), !e)) return g;
            128 & e
                ? ((f = 63 & r[o++]),
                  192 != (224 & e)
                      ? ((i = 63 & r[o++]),
                        224 == (240 & e)
                            ? (e = ((15 & e) << 12) | (f << 6) | i)
                            : ((n = 63 & r[o++]),
                              240 == (248 & e)
                                  ? (e =
                                        ((7 & e) << 18) |
                                        (f << 12) |
                                        (i << 6) |
                                        n)
                                  : ((C = 63 & r[o++]),
                                    248 == (252 & e)
                                        ? (e =
                                              ((3 & e) << 24) |
                                              (f << 18) |
                                              (i << 12) |
                                              (n << 6) |
                                              C)
                                        : ((t = 63 & r[o++]),
                                          (e =
                                              ((1 & e) << 30) |
                                              (f << 24) |
                                              (i << 18) |
                                              (n << 12) |
                                              (C << 6) |
                                              t)))),
                        65536 > e
                            ? (g += F(e))
                            : ((a = e - 65536),
                              (g += F(55296 | (a >> 10), 56320 | (1023 & a)))))
                      : (g += F(((31 & e) << 6) | f)))
                : (g += F(e));
        }
    }
    // split up into chunks, because .apply on a huge string can overflow the stack
    for (var ret = "", curr; length > 0; ptr += 1024, length -= 1024)
        ret += String.fromCharCode.apply(
            String,
            HEAPU8.subarray(ptr, ptr + Math.min(length, 1024))
        );
    return ret;
}

// Defines syscall emulation functions in the env and wasi object that get passed to the wasm module
function SYSCALLS_WASM_IMPORTS(env, wasi) {
    var PAYLOAD_CURSOR = 0;
    var PAYLOAD = new Uint8Array(0);

    // sys_open call to open a file (can only be used to open payload here)
    env.__sys_open = function (pPath, flags, pMode) {
        // Opening just resets the seek cursor to 0
        PAYLOAD_CURSOR = 0;
        //var pathname = ReadHeapString(pPath); //read the file name passed to open
        //console.log('__sys_open open - path: ' + pathname + ' - flags: ' + flags + ' - mode: ' + HEAPU32[pMode>>2]);
        return 9; //return dummy file number
    };

    wasi.fd_fdstat_get = function (fd, bufPtr) {
        return 0;
    };

    // fd_read call to read from a file (reads from payload)
    wasi.fd_read = function (fd, iov, iovcnt, pOutResult) {
        for (var ret = 0, i = 0; i < iovcnt; i++) {
            // Process list of IO commands
            var ptr = HEAPU32[(iov + i * 8) >> 2];
            var len = HEAPU32[(iov + (i * 8 + 4)) >> 2];
            var curr = Math.min(len, PAYLOAD.length - PAYLOAD_CURSOR);
            //console.log('fd_read - fd: ' + fd + ' - iov: ' + iov + ' - iovcnt: ' + iovcnt + ' - ptr: ' + ptr + ' - len: ' + len + ' - reading: ' + curr + ' (from ' + PAYLOAD_CURSOR + ' to ' + (PAYLOAD_CURSOR + curr) + ')');

            // Write the requested data onto the heap and advance the seek cursor
            HEAPU8.set(
                PAYLOAD.subarray(PAYLOAD_CURSOR, PAYLOAD_CURSOR + curr),
                ptr
            );
            PAYLOAD_CURSOR += curr;

            ret += curr;
            if (curr < len) break; // nothing more to read
        }

        // Write the amount of data actually read to the result pointer
        HEAPU32[pOutResult >> 2] = ret;
        //console.log('fd_read -     ret: ' + ret);
        return 0; // no error
    };

    // fd_seek call to seek in a file (seeks in payload)
    wasi.fd_seek = function (fd, offset_low, offset_high, whence, pOutResult) {
        // Move seek cursor according to fseek behavior
        if (whence == 0) PAYLOAD_CURSOR = offset_low; //set
        if (whence == 1) PAYLOAD_CURSOR += offset_low; //cur
        if (whence == 2) PAYLOAD_CURSOR = PAYLOAD.length - offset_low; //end
        if (PAYLOAD_CURSOR < 0) PAYLOAD_CURSOR = 0;
        if (PAYLOAD_CURSOR > PAYLOAD.length) PAYLOAD_CURSOR = PAYLOAD.length;

        // Write the result back (write only lower 32-bit of 64-bit number)
        HEAPU32[(pOutResult + 0) >> 2] = PAYLOAD_CURSOR;
        HEAPU32[(pOutResult + 4) >> 2] = 0;
        //console.log('fd_seek - fd: ' + fd + ' - offset_high: ' + offset_high + ' - offset_low: ' + offset_low + ' - pOutResult: ' + pOutResult + ' - whence: ' +whence + ' - seek to: ' + PAYLOAD_CURSOR);
        return 0; // no error
    };

    // fd_write call to write to a file/pipe (can only be used to write to stdout here)
    wasi.fd_write = function (fd, iov, iovcnt, pOutResult) {
        for (var ret = 0, str = "", i = 0; i < iovcnt; i++) {
            // Process list of IO commands, read passed strings from heap
            var ptr = HEAPU32[(iov + i * 8) >> 2];
            var len = HEAPU32[(iov + (i * 8 + 4)) >> 2];
            if (len < 0) return -1;
            ret += len;
            str += ReadHeapString(ptr, len);
            console.log(str)
            // console.log('fd_write - fd: ' + fd + ' - ['+i+'][len:'+len+']: ' + ReadHeapString(ptr, len).replace(/\n/g, '\\n'));
        }

        // Print the passed string and write the number of bytes read to the result pointer
        // WA.print(str);
        HEAPU32[pOutResult >> 2] = ret;
        return 0; // no error
    };

    // fd_close to close a file (no real file system emulation, so this does nothing)
    wasi.fd_close = function (fd) {
        //console.log('fd_close - fd: ' + fd);
        return 0; // no error
    };

    // sys_fcntl64 and sys_ioctl set file and IO modes/flags which are not emulated here
    env.__sys_fcntl64 = env.__sys_ioctl = function (fd, param) {
        return 0; // no error
    };
}

// Set up the env and wasi objects that contains the functions passed to the wasm module
var env = {
        // sbrk gets called to increase the size of the memory heap by an increment
        sbrk: function (increment) {
            var heapOld = WASM_HEAP,
                heapNew = heapOld + increment,
                heapGrow = heapNew - WASM_MEMORY.buffer.byteLength;
            //console.log('[SBRK] Increment: ' + increment + ' - HEAP: ' + heapOld + ' -> ' + heapNew + (heapGrow > 0 ? ' - GROW BY ' + heapGrow + ' (' + (heapGrow>>16) + ' pages)' : ''));
            if (heapNew > WASM_HEAP_MAX) abort("MEM", "Out of memory");
            if (heapGrow > 0) {
                WASM_MEMORY.grow((heapGrow + 65535) >> 16);
                MemorySetBufferViews();
            }
            WASM_HEAP = heapNew;
            return heapOld | 0;
        },

        // Functions querying the system time
        time: function (ptr) {
            var ret = (Date.now() / 1000) | 0;
            if (ptr) HEAPU32[ptr >> 2] = ret;
            return ret;
        },
        gettimeofday: function (ptr) {
            var now = Date.now();
            HEAPU32[ptr >> 2] = (now / 1000) | 0;
            HEAPU32[(ptr + 4) >> 2] = ((now % 1000) * 1000) | 0;
        },

        // Various functions thet can be called from wasm that abort the program
        __assert_fail: function (condition, filename, line, func) {
            abort(
                "CRASH",
                "Assert " +
                    ReadHeapString(condition) +
                    ", at: " +
                    (filename ? ReadHeapString(filename) : "unknown filename"),
                line,
                func ? ReadHeapString(func) : "unknown function"
            );
        },
        __cxa_uncaught_exception: function () {
            abort("CRASH", "Uncaught exception!");
        },
        __cxa_pure_virtual: function () {
            abort("CRASH", "pure virtual");
        },
        abort: function () {
            abort("CRASH", "Abort called");
        },
        longjmp: function () {
            abort("CRASH", "Unsupported longjmp called");
        },
    },
    wasi = {};

// Functions that do nothing in this wasm context
env.setjmp = env.__cxa_atexit = env.__lock = env.__unlock = function () {};

// Math functions
env.ceil = env.ceilf = Math.ceil;
env.exp = env.expf = Math.exp;
env.floor = env.floorf = Math.floor;
env.log = env.logf = Math.log;
env.pow = env.powf = Math.pow;
env.cos = env.cosf = Math.cos;
env.sin = env.sinf = Math.sin;
env.tan = env.tanf = Math.tan;
env.acos = env.acosf = Math.acos;
env.asin = env.asinf = Math.asin;
env.sqrt = env.sqrtf = Math.sqrt;
env.atan = env.atanf = Math.atan;
env.atan2 = env.atan2f = Math.atan2;
env.fabs = env.fabsf = env.abs = Math.abs;
env.round = env.roundf = env.rint = env.rintf = Math.round;

// Find the start point of the stack and the heap to calculate the initial memory requirements
var wasmDataEnd = 64,
    wasmStackTop = 4096,
    wasmHeapBase = 65536;
// Set the initial wasm memory size to [DATA] + [STACK] + [256KB HEAP] (can be grown with sbrk)
var wasmMemInitial = (((wasmHeapBase + 65535) >> 16) << 16) + 256 * 1024;
WASM_HEAP = wasmHeapBase;
WASM_MEMORY = env.memory = new WebAssembly.Memory({
    initial: wasmMemInitial >> 16,
    maximum: WASM_HEAP_MAX >> 16,
});

MemorySetBufferViews();

// Extend the objects with the syscall IO emulation
SYSCALLS_WASM_IMPORTS(env, wasi);

function wasmLoad(fileName) {
    // var heap = new Uint8Array(env.memory.buffer);

    fetch(fileName)
        .then((res) => res.arrayBuffer())
        .then(function (wasmBytes) {
            "use strict";
            wasmBytes = new Uint8Array(wasmBytes);
            WebAssembly.instantiate(wasmBytes, {
                env: env,
                wasi_unstable: wasi,
                wasi_snapshot_preview1: wasi,
                wasi: wasi,
            }).then(function (output) {
                WA.wasm = output.instance.exports;
                WA.wasm.run();
            });
        });
} // loadWasm()
//===============================================================
function main() {
    wasmLoad("webasm_demo.wasm");
}

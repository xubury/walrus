// Some global state variables and max heap definition
export var ABORT = false; // program crash signal
var WASM_MEMORY, WASM_HEAP
const WASM_HEAP_MAX = 256*1024*1024; //max 256MB

const __WASI_CLOCKID_REALTIME =  0;
const __WASI_CLOCKID_MONOTONIC = 1;// Unimplemented
const __WASI_CLOCKID_PROCESS_CPUTIME_ID =  2;// Unimplemented

const WASI_ESUCCESS = 0;
const WASI_EBADF = 8;
const WASI_EINVAL = 28;
const WASI_EIO = 29;
const WASI_ENOSYS = 52;
// Find the start point of the stack and the heap to calculate the initial memory requirements
var wasmDataEnd = 64, wasmStackTop = 4096, wasmHeapBase = 65536;

// A generic abort function that if called stops the execution of the program and shows an error
export function abort(code, msg) {
    ABORT = true;
    WA.error("[%s] %s", code, msg);
    throw "abort";
}

export function getCallbackFromWasm(func)
{
    if (!WA.wasm) return null;

    return WA.wasm.__indirect_function_table.get(func)
}

export function getHeap()
{
    return new DataView(WASM_MEMORY.buffer);
}

// Puts a string from javascript onto the wasm memory heap (encoded as UTF8) (max_length is optional)
export function writeHeapString(str, ptr, max_length)
{
    var HEAPU8 = new Uint8Array(WASM_MEMORY.buffer);
	for (var e=str,r=HEAPU8,f=ptr,i=(max_length?max_length:HEAPU8.length),a=f,t=f+i-1,b=0;b<e.length;++b)
	{
		var k=e.charCodeAt(b);
		if(55296<=k&&k<=57343&&(k=65536+((1023&k)<<10)|1023&e.charCodeAt(++b)),k<=127){if(t<=f)break;r[f++]=k;}
		else if(k<=2047){if(t<=f+1)break;r[f++]=192|k>>6,r[f++]=128|63&k;}
		else if(k<=65535){if(t<=f+2)break;r[f++]=224|k>>12,r[f++]=128|k>>6&63,r[f++]=128|63&k;}
		else if(k<=2097151){if(t<=f+3)break;r[f++]=240|k>>18,r[f++]=128|k>>12&63,r[f++]=128|k>>6&63,r[f++]=128|63&k;}
		else if(k<=67108863){if(t<=f+4)break;r[f++]=248|k>>24,r[f++]=128|k>>18&63,r[f++]=128|k>>12&63,r[f++]=128|k>>6&63,r[f++]=128|63&k;}
		else{if(t<=f+5)break;r[f++]=252|k>>30,r[f++]=128|k>>24&63,r[f++]=128|k>>18&63,r[f++]=128|k>>12&63,r[f++]=128|k>>6&63,r[f++]=128|63&k;}
	}
	return r[f]=0,f-a;
}

// Reads a string from the wasm memory heap to javascript (decoded as UTF8)
export function readHeapString(ptr, length)
{
    var HEAPU8 = new Uint8Array(WASM_MEMORY.buffer);
	if (length === 0 || !ptr) return '';
	for (var hasUtf = 0, t, i = 0; !length || i != length; i++)
	{
		t = HEAPU8[((ptr)+(i))>>0];
		if (t == 0 && !length) break;
		hasUtf |= t;
	}
	if (!length) length = i;
	if (hasUtf & 128)
	{
		for(var r=HEAPU8,o=ptr,p=ptr+length,F=String.fromCharCode,e,f,i,n,C,t,a,g='';;)
		{
			if(o==p||(e=r[o++],!e)) return g;
			128&e?(f=63&r[o++],192!=(224&e)?(i=63&r[o++],224==(240&e)?e=(15&e)<<12|f<<6|i:(n=63&r[o++],240==(248&e)?e=(7&e)<<18|f<<12|i<<6|n:(C=63&r[o++],248==(252&e)?e=(3&e)<<24|f<<18|i<<12|n<<6|C:(t=63&r[o++],e=(1&e)<<30|f<<24|i<<18|n<<12|C<<6|t))),65536>e?g+=F(e):(a=e-65536,g+=F(55296|a>>10,56320|1023&a))):g+=F((31&e)<<6|f)):g+=F(e);
		}
	}
	// split up into chunks, because .apply on a huge string can overflow the stack
	for (var ret = '', curr; length > 0; ptr += 1024, length -= 1024)
		ret += String.fromCharCode.apply(String, HEAPU8.subarray(ptr, ptr + Math.min(length, 1024)));
	return ret;
}


export async function initSys(wasmBytes, libLoader)
{
    "use strict";
    wasmBytes = new Uint8Array(wasmBytes);
    console.log("[WAJS] compile start");

    // This code goes through the wasm file sections according the binary encoding description
    //     https://webassembly.org/docs/binary-encoding/
    for (let i = 8, sectionEnd, type, length; i < wasmBytes.length; i = sectionEnd)
    {
        // Get() gets the next single byte, GetLEB() gets a LEB128 variable-length number
        function Get() { return wasmBytes[i++]; }
        function GetLEB() { for (var s=i,r=0,n=128; n&128; i++) r|=((n=wasmBytes[i])&127)<<((i-s)*7); return r; }
        type = GetLEB(), length = GetLEB(), sectionEnd = i + length;
        if (type < 0 || type > 11 || length <= 0 || sectionEnd > wasmBytes.length) break;
        if (type == 6)
        {
            //Section 6 'Globals', llvm places the heap base pointer into the first value here
            let count = GetLEB(), gtype = Get(), mutable = Get(), opcode = GetLEB(), offset = GetLEB(), endcode = GetLEB();
            wasmHeapBase = offset;
        }
        if (type == 11)
        {
            //Section 11 'Data', contains data segments which the end of the last entry will indicate the start of the stack area
            for (let count = GetLEB(), j = 0; j != count && i < sectionEnd; j++)
            {
                let dindex = Get(), dopcode = GetLEB(), doffset = GetLEB(), dendcode = GetLEB(), dsize = GetLEB();
                wasmDataEnd = (doffset + dsize);
                wasmStackTop = (wasmDataEnd+15)>>4<<4;
                i += dsize;
            }
        }
    }
    // Validate the queried pointers
    if (wasmDataEnd <= 0 || wasmHeapBase <= wasmStackTop) abort('BOOT', 'Invalid memory layout (' + wasmDataEnd + '/' + wasmStackTop + '/' + wasmHeapBase + ')');

    // Set the initial wasm memory size to [DATA] + [STACK] + [256KB HEAP] (can be grown with sbrk)
    var wasmMemInitial = ((wasmHeapBase+65535)>>16<<16) + (256 * 1024);
    WASM_HEAP = wasmHeapBase;
    WASM_MEMORY = env.memory = new WebAssembly.Memory({initial: wasmMemInitial>>16, maximum: WASM_HEAP_MAX>>16 });

    if (libLoader) {
        libLoader(env);
    }

    try {
        const output = await WebAssembly.instantiate(wasmBytes, {
            env: env,
            wasi_unstable: wasi,
            wasi_snapshot_preview1: wasi,
            wasi: wasi,
        });
        console.log("[WAJS] compile finish");
        WA.wasm = output.instance.exports;


        // C++ global ctor
        if (WA.wasm.__wasm_call_ctors)
            WA.wasm.__wasm_call_ctors();

        if (WA.wasm._start) {
            console.log("[WAJS] wasm main start");
            WA.wasm._start();
            console.log("[WAJS] wasm main exit");
        }

    } catch (err) {
        // On an exception, if the err is 'abort' the error was already processed in the abort function above
        if (err !== 'abort')
            abort('BOOT', 'WASM instiantate error: ' + err + (err.stack ? "\n" + err.stack : ''));
    }
}

// Set up the env and wasi objects that contains the functions passed to the wasm module
var env =
{
	// sbrk gets called to increase the size of the memory heap by an increment
	sbrk: function(increment)
	{
		var heapOld = WASM_HEAP, heapNew = heapOld + increment, heapGrow = heapNew - WASM_MEMORY.buffer.byteLength;
		//console.log('[SBRK] Increment: ' + increment + ' - HEAP: ' + heapOld + ' -> ' + heapNew + (heapGrow > 0 ? ' - GROW BY ' + heapGrow + ' (' + (heapGrow>>16) + ' pages)' : ''));
		if (heapNew > WASM_HEAP_MAX) abort('MEM', 'Out of memory');
		if (heapGrow > 0) { WASM_MEMORY.grow((heapGrow+65535)>>16); }
		WASM_HEAP = heapNew;
		return heapOld|0;
	},

	// Various functions thet can be called from wasm that abort the program
	__assert_fail:  function(condition, filename, line, func) { abort('CRASH', 'Assert ' + readHeapString(condition) + ', at: ' + (filename ? readHeapString(filename) : 'unknown filename'), line, (func ? readHeapString(func) : 'unknown function')); },
	__cxa_uncaught_exception: function() { abort('CRASH', 'Uncaught exception!'); },
	__cxa_pure_virtual: function() { abort('CRASH', 'pure virtual'); },
	abort: function() { abort('CRASH', 'Abort called'); },
	longjmp: function() { abort('CRASH', 'Unsupported longjmp called'); },


    wajs_set_main_loop: function(engine_loop) {
        engine_loop = getCallbackFromWasm(engine_loop);
        var drawFunc = function () {
            if (ABORT) return;
            window.requestAnimationFrame(drawFunc);
            engine_loop();
        };

        window.requestAnimationFrame(drawFunc);
    },

    wajs_set_shutdown : function(engine_shutdown) {
        engine_shutdown = getCallbackFromWasm(engine_shutdown);
        window.addEventListener("beforeunload", (event)=>{
            engine_shutdown();
        })
    }

}, wasi = {};

// Extend the objects with the syscall IO emulation
SYSCALLS_WASM_IMPORTS(env, wasi);

// Defines syscall emulation functions in the env and wasi object that get passed to the wasm module
function SYSCALLS_WASM_IMPORTS(env, wasi)
{
	// Function to decode Base64 encoded string to a byte array
	function Base64Decode(B)
	{
		var T=new Uint8Array(128),i,C=function(o){return T[B.charCodeAt(i+o)];};
		for (i=0;i<64;i++) T['ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'.charCodeAt(i)]=i;T[45]=62;T[95]=63;
		var L=B.length,PH=(B[L-2]==='='?2:(B[L-1]==='='?1:0)),a=new Uint8Array(L*3/4-PH),n=0,j=(PH>0?L-4:L),t=0;
		for (i=0;i<j;i+=4) { t = (C(0)<<18)|(C(1)<<12)|(C(2)<<6)|C(3); a[n++]=(t>>16)&255;a[n++]=(t>>8)&255;a[n++]=t&255; }
		if (PH===1) { t=(C(0)<<10)|(C(1)<<4)|(C(2)>>2); a[n]=(t>>8)&255;a[n+1]=t&255; }
		else if (PH===2) a[n]=((C(0)<<2)|(C(1)>>4))&255;
		return a;
	}

	// For the file open/file reading emulation, keep a seek cursor and the file data
	var PAYLOAD_CURSOR = 0;
	var PAYLOAD = (WA.payload ? Base64Decode(WA.payload) : new Uint8Array(0));
	delete WA.payload;

    const argvs = ['wasm.exe', 'hello'];

    wasi.args_get = function(argv, argv_buf) {
        var heap = getHeap();
        for (const arg of argvs) {
            heap.setUint32(argv, argv_buf, true)
            argv_buf += writeHeapString(arg, argv_buf)
            heap.setUint8(argv_buf, 0, true)
            argv_buf += 1;
            argv += 4;
        }
        heap.setUint32(argv, 0, true)
        return WASI_ESUCCESS;
    };

    wasi.args_sizes_get = function(argc, argvBufSize) {
        var heap = getHeap();
        heap.setUint32(argc, argvs.length, true)
        var length = 0;
        for (const arg of argvs) {
            length += arg.length;
        }
        heap.setUint32(argvBufSize, length, true)
        return WASI_ESUCCESS;
    };

    // filesystem
    const rootDir = '/'
    var fds = [];

	wasi.fd_read = function(fd, iov, iovcnt, pOutResult)
	{
        if (fds[fd] == null || fds[fd].data == null || fds[fd].filename == null) {
            return WASI_EBADF;
        }
        var heap = getHeap();

        for (var ret = 0, i = 0; i < iovcnt; i++)
        {
            // Process list of IO commands
            var ptr = heap.getUint32(iov + 8 * i + 0, true);
            var len = heap.getUint32(iov + 8 * i + 4, true);
            var read = Math.min(len, fds[fd].data.length - fds[fd].pos);

            // Write the requested data onto the heap and advance the seek cursor
            var sub = fds[fd].data.subarray(fds[fd].pos, fds[fd].pos + read);
            new Uint8Array(WASM_MEMORY.buffer).set(sub, ptr);

            fds[fd].pos += read;
            ret += read;
        }

        // Write the amount of data actually read to the result pointer
        heap.setUint32(pOutResult, ret, true);
        return WASI_ESUCCESS;
	};

    wasi.fd_seek = function(fd, offset_low, offset_high, whence, pOutResult)
    {
        if (fds[fd] == null || fds[fd].data == null) {
            return WASI_EBADF;
        }
        var heap = getHeap();
        // Move seek cursor according to fseek behavior
        if (whence == 0) fds[fd].pos = offset_low; //set
        if (whence == 1) fds[fd].pos += offset_low; //cur
        if (whence == 2) fds[fd].pos = fds[fd].data.length - offset_low; //end
        if (fds[fd].pos < 0) fds[fd].pos = 0;
        if (fds[fd].pos > fds[fd].data.length) fds[fd].pos = fds[fd].data.length;

        // Write the result back (write only lower 32-bit of 64-bit number)
        heap.setUint32(pOutResult, fds[fd].pos, true);
        heap.setUint32(pOutResult + 4, 0, true);
        return WASI_ESUCCESS; // no error
    };

	// fd_write call to write to a file/pipe (can only be used to write to stdout here)
    wasi.fd_write = function(fd, iov, iovcnt, pOutResult)
    {
        var heap = getHeap();
        for (var ret = 0, str = '', i = 0; i < iovcnt; i++)
        {
            // Process list of IO commands, read passed strings from heap
            var ptr = heap.getUint32(iov + 8 * i + 0, true);
            var len = heap.getUint32(iov + 8 * i + 4, true);
            if (len < 0) return -1;
            ret += len;
            str += readHeapString(ptr, len);
        }

        // Print the passed string and write the number of bytes read to the result pointer
        if (fd == 1) {
            WA.print(str);
        } else if (fd == 2) {
            WA.error(str);
        }
        heap.setUint32(pOutResult, ret, true);
        return WASI_ESUCCESS; // no error
    };

	wasi.fd_close = function(fd)
	{
        if (fds[fd] == null) {
            return WASI_EBADF;
        }
        fds[fd].close();
        fds[fd] = null;
		return WASI_ESUCCESS; // no error
	};

    wasi.fd_fdstat_get = function(fd, stat)
    {
        if (fds[fd]) {
            WA.print("[WAJS] fd_fdstat_get fd:%d filename:%s", fd, fds[fd].filename);
        }
        var heap = getHeap();
        heap.setUint8(stat + 0, fds[fd] != null ? 3 : 4);
        heap.setUint16(stat + 2, 0, true);
        heap.setUint32(stat + 8, 0, true);
        heap.setUint32(stat + 12, 0, true);
        heap.setUint32(stat + 16, 0, true);
        heap.setUint32(stat + 20, 0, true);
        return WASI_ESUCCESS;
    };

    wasi.fd_fdstat_set_flags = function(fd, flags) 
    {
        return WASI_ENOSYS;
    };

    wasi.fd_prestat_get = function(fd, ptr)
    {
        // WA.print("[WAJS] fd_prestat_get fd:%d ptr:%d", fd, ptr);
        if (fd == 3) {
            var heap = getHeap();
            heap.setUint8(ptr, 0);
            heap.setUint32(ptr + 4, rootDir.length, true);
            return WASI_ESUCCESS;
        }
        return WASI_EBADF;
    };

    wasi.fd_prestat_dir_name = function(fd, pathptr, pathlen)
    {
        // WA.print("[WAJS] fd_prestat_dir_name fd: %d pathptr: %d pathlen: %d", fd, pathptr, pathlen);
        if (rootDir.length != pathlen) {
            return WASI_EINVAL;
        }
        writeHeapString(rootDir, pathptr)
        return WASI_ESUCCESS;
    };



    wasi.path_open = function(parent_fd, dirflags, path, path_len, oflags, fs_rights_base, fs_rights_inheriting, fdflags, opened_fd)
    {
        const filename = readHeapString(path, path_len);
        var heap = getHeap()

        var xhr = new XMLHttpRequest();
        xhr.open('GET', "/fd_open?" + new URLSearchParams({filename : filename}), false);
        var code = WASI_ESUCCESS;
        xhr.onload = function() {
            if (xhr.status != 200)
            {
                code = WASI_EIO;
                return;
            }
            const json = JSON.parse(xhr.response);
            const fd = json.fd;
            var file = {};
            file.close = function close()
            {
                if (this.fd == null) return;
                var xhr = new XMLHttpRequest();
                xhr.open('GET', "/fd_close?" + new URLSearchParams({fd : this.fd}));
                xhr.send();
            };
            file.fd = fd;
            file.filename = filename;
            file.pos = 0;
            file.data = new Uint8Array(json.payload.data);
            fds[fd] = file;

            heap.setUint32(opened_fd, fd, true);
        };
        xhr.onerror = function() {
            WA.print(`[WAJS] Network Error`);
            code = WASI_EIO;
        };
        xhr.send();

        return code;
    };

    wasi.proc_exit = function(code)
    {
        abort(code, ":proc_exit");
    };

    // Functions querying the system time
    wasi.clock_time_get =  function(clk_id, pre, ptime)
    { 
        var now
        if (clk_id == __WASI_CLOCKID_REALTIME) {
           now = Date.now();
        } else {
            return WASI_ENOSYS;
        }
        var nsec = Math.round(now * 1000 * 1000);
        var heap = getHeap();
        heap.setInt32(ptime + 4, (nsec / Math.pow(2, 32)) >>> 0, true);
        heap.setInt32(ptime + 0, (nsec >>> 0), true);
        return WASI_ESUCCESS;
    };

}

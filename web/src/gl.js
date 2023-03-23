import * as sys from "./sys.js";

var glCtx;
var glCounter = 1;
var glPrograms = [];
var glShaders = [];
var glTextures = [];

const GL_INFO_LOG_LENGTH = 0x8b84

function getNewId(table) {
    var ret = glCounter++;
    for (var i = table.length; i < ret; i++) table[i] = null;
    return ret;
}

function genObjects(n, buffers, createFunction, table) {
    var heap = sys.getHeap()
    for (var i = 0; i < n; ++i) {
        var buffer = glCtx[createFunction]();
        var id = buffer && getNewId(table);
        if (buffer) {
            buffer.name = id;
            table[id] = buffer;
        } else {
            sys.abort(
                "GL_INVALID_OPERATION",
                "Fail to create gl object, gl context may be lost!"
            );
        }
        heap.setInt32(buffers + i * 4, id, true);
    }
}

function deleteObjects(n, buffers, destroyFunction, table) {
    var heap = sys.getHeap()
    for (var i = 0; i < n; ++i) {
        var id = heap.getInt32(buffers + i * 4, true);
        var object = table[id];
        if (!object) continue;
        glCtx[destroyFunction](object);
        table[id] = null;
    }
}

function getSource(count, string, length) {
    var source = "";
    var heap = sys.getHeap()
    for (var i = 0; i < count; ++i) {
        var frag;
        if (length) {
            var len = heap.getInt32(length + i  * 4);
            if (len < 0)
                frag = sys.ReadHeapString(heap.getInt32(string + i * 4, true));
            else
                frag = sys.ReadHeapString(
                    heap.getInt32(string + i * 4, true),
                    len
                );
        } else {
            frag = sys.ReadHeapString(heap.getInt32(string + i * 4, true));
        }
        source += frag;
    }
    return source;
}

function setupGlContext(canvas, attr) {
    var attr = {
        majorVersion: 4,
        minorVersion: 5,
        antialias: true,
        alpha: false,
    };
    var errorInfo = "";
    let onContextCreationError = function (event) {
        errorInfo = event.statusMessage || errorInfo;
    };
    try {
        glCtx = canvas.getContext("webgl2", attr);
        if (!glCtx) throw "Could not create context";
    } finally {
        canvas.removeEventListener(
            "webglcontextcreationerror",
            onContextCreationError,
            false
        );
    }

    return true;
}

// Init gl interface
export function importGl(env) {
    console.log("importGl()");

    Object.assign(env, {
        wajsSetupGlCanvas: function (width, height) {
            var cnvs = WA.canvas;
            cnvs.width = width;
            cnvs.height = height;
            cnvs.height = cnvs.clientHeight;
            cnvs.width = cnvs.clientWidth;

            if (!setupGlContext(cnvs)) return;

            var drawFunc = function () {
                if (sys.ABORT) return;
                window.requestAnimationFrame(drawFunc);
                WA.wasm.__wajsGlDraw();
            };

            window.requestAnimationFrame(drawFunc);
        },

        glGetError: function () {
            return glCtx.getError();
        },

        glDisable: function (cap) {
            glCtx.disable(cap);
        },

        glClear: function (bitmask) {
            glCtx.clear(bitmask);
        },

        glClearColor: function (x0, x1, x2, x3) {
            glCtx.clearColor(x0, x1, x2, x3);
        },

        glViewport: function (x, y, w, h) {
            glCtx.viewport(x, y, w, h);
        },

        glCreateProgram: function () {
            var id = getNewId(glPrograms);
            var program = glCtx.createProgram();
            program.name = id;
            glPrograms[id] = program;
            return id;
        },

        glCreateShader: function (shaderType) {
            var id = getNewId(glShaders);
            glShaders[id] = glCtx.createShader(shaderType);
            return id;
        },

        glShaderSource: function (shader, count, string, length) {
            var source = getSource(count, string, length);
            glCtx.shaderSource(glShaders[shader], source);
        },

        glCompileShader: function (shader) {
            glCtx.compileShader(glShaders[shader]);
        },

        glGetShaderiv: function (shader, param, ptr) {
            var ret = 0;
            if (param == GL_INFO_LOG_LENGTH) {
                ret = glCtx.getShaderInfoLog(glShaders[shader]).length;
            } else {
                ret = glCtx.getShaderParameter(glShaders[shader], param);
            }
            var heap = sys.getHeap()
            heap.setInt32(ptr, ret, true);
        },

        glGetShaderInfoLog: function (shader, bufSize, logSize, ptr) {
            var log = glCtx.getShaderInfoLog(glShaders[shader]);
            if (logSize) {
                var heap = sys.getHeap()
                heap.setInt32(logSize, log.length, true);
            }
            sys.WriteHeapString(log, ptr, bufSize);
        },

        glGetProgramiv: function (program, param, ptr) {
            var ret = 0;
            if (param == GL_INFO_LOG_LENGTH) {
                ret = glCtx.getProgramInfoLog(glPrograms[program]).length;
            } else {
                ret = glCtx.getProgramParameter(glPrograms[program], param);
            }

            var heap = sys.getHeap();
            heap.setInt32(ptr, ret, true);
        },

        glGetProgramInfoLog: function (program, bufSize, logSize, ptr) {
            var log = glCtx.getProgramInfoLog(glPrograms[program]);
            if (logSize) {
                var heap = sys.getHeap()
                heap.setInt32(logSize, log.length, true);
            }
            sys.WriteHeapString(log, ptr, bufSize);
        },

        glAttachShader: function (program, shader) {
            glCtx.attachShader(glPrograms[program], glShaders[shader]);
        },

        glLinkProgram: function (program) {
            glCtx.linkProgram(glPrograms[program]);
            // TODO: uniforms
            // GLprogramInfos[program] = null; // uniforms no longer keep the same names after linking
            // populateUniformTable(program);
        },
        glUseProgram: function (program) {
            glCtx.useProgram(program ? glPrograms[program] : null);
        },

        glDrawArrays: function (mode, first, count) {
            glCtx.drawArrays(mode, first, count);
        },

        glGenTextures: function (n, buffers) {
            genObjects(n, buffers, "createTexture", glTextures);
        },

        glDeleteTextures: function (n, buffers) {
            deleteObjects(n, buffers, "deleteTexture", glTextures);
        },
    });
}

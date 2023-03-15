import * as sys from "./sys.js";

var glCtx;
var glCounter = 1;
var glPrograms = [];
var glShaders = [];

function getNewId(table) {
    var ret = glCounter++;
    for (var i = table.length; i < ret; i++) table[i] = null;
    return ret;
}

function getSource(count, string, length) {
    var source = "";
    for (var i = 0; i < count; ++i) {
        var frag;
        if (length) {
            var len = sys.HEAP32[(length + i * 4) >> 2];
            if (len < 0)
                frag = sys.ReadHeapString(sys.HEAP32[(string + i * 4) >> 2]);
            else
                frag = sys.ReadHeapString(
                    sys.HEAP32[(string + i * 4) >> 2],
                    len
                );
        } else {
            frag = sys.ReadHeapString(sys.HEAP32[(string + i * 4) >> 2]);
        }
        source += frag;
    }
    return source;
}

function setupGlContext(canvas, attr) {
    var attr = {
        majorVersion: 3,
        minorVersion: 0,
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
            var ret = glCtx.getShaderParameter(glShaders[shader], param);
            sys.HEAP32[ptr >> 2] = ret;
        },

        glGetShaderInfoLog: function (shader, bufSize, logSize, ptr) {
            var log = glCtx.getShaderInfoLog(glShaders[shader]);
            if (logSize) {
                sys.HEAP32[logSize >> 2] = log.length;
            }
            sys.WriteHeapString(log, ptr, bufSize);
        },

        glAttachShader: function (program, shader) {
            glCtx.attachShader(glPrograms[program], glShaders[shader]);
        },

        glLinkProgram: function (program) {
            glCtx.linkProgram(glPrograms[program]);
            // GLprogramInfos[program] = null; // uniforms no longer keep the same names after linking
            // populateUniformTable(program);
        },
        glUseProgram: function (program) {
            glCtx.useProgram(program ? glPrograms[program] : null);
        },

        glDrawArrays: function (mode, first, count) {
            glCtx.drawArrays(mode, first, count);
        },
    });
}

import * as sys from "./sys.js";

var glCtx;
var glCounter = 1;
var glPrograms = [];
var glProgramInfos = [];
var glUniforms = [];
var glShaders = [];
var glTextures = [];
var glBuffers = [];
var glRenderBuffers = [];
var glVertexArrays = [];

const GL_INFO_LOG_LENGTH = 0x8b84
const GL_UNPACK_ALIGNMENT = 4;

var GL_MINI_TEMP_BUFFER_SIZE = 256;
var GL_miniTempBuffer = null;
var GL_miniTempBufferViews = [0];
GL_miniTempBuffer = new Float32Array(GL_MINI_TEMP_BUFFER_SIZE);
for (var i = 0; i < GL_MINI_TEMP_BUFFER_SIZE; i++) GL_miniTempBufferViews[i] = GL_miniTempBuffer.subarray(0, i+1);

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
                frag = sys.readHeapString(heap.getInt32(string + i * 4, true));
            else
                frag = sys.readHeapString(
                    heap.getInt32(string + i * 4, true),
                    len
                );
        } else {
            frag = sys.readHeapString(heap.getInt32(string + i * 4, true));
        }
        source += frag;
    }
    return source;
}

function setupGlContext(canvas, attr) {
    var attr = {
        majorVersion: 3,
        minorVersion: 2,
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

function webGlGetTexPixelData3D(type, format, width, height, depth, pixels)
{
    var sizePerPixel;
    var numChannels;
    var heap = sys.getHeap();
    
    var HEAPU8 = new Uint8Array(heap.buffer);
    var HEAPF32 = new Float32Array(heap.buffer);
    var HEAPU16 = new Uint16Array(heap.buffer);
    var HEAPU32 = new Uint32Array(heap.buffer);
    switch(format)
    {
        case 0x1906: case 0x1909: case 0x1902: numChannels = 1; break; //GL_ALPHA, GL_LUMINANCE, GL_DEPTH_COMPONENT
        case 0x190A: numChannels = 2; break; //GL_LUMINANCE_ALPHA
        case 0x1907: case 0x8C40: numChannels = 3; break; //GL_RGB, GL_SRGB_EXT
        case 0x1908: case 0x8C42: numChannels = 4; break; //GL_RGBA, GL_SRGB_ALPHA_EXT
        default: WA.error("Invalid type: %d", type); return null; //GL_INVALID_ENUM
    }
    switch (type)
    {
        case 0x1401: sizePerPixel = numChannels*1; break; //GL_UNSIGNED_BYTE
        case 0x1403: case 0x8D61: sizePerPixel = numChannels*2; break; //GL_UNSIGNED_SHORT, GL_HALF_FLOAT_OES
        case 0x1405: case 0x1406: sizePerPixel = numChannels*4; break; //GL_UNSIGNED_INT, GL_FLOAT
        case 0x84FA: sizePerPixel = 4; break; //GL_UNSIGNED_INT_24_8_WEBGL/GL_UNSIGNED_INT_24_8
        case 0x8363: case 0x8033: case 0x8034: sizePerPixel = 2; break; //GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_4_4_4_4, GL_UNSIGNED_SHORT_5_5_5_1
        default: WA.error("Invalid type: %d", type); return null; //GL_INVALID_ENUM
    }

    function roundedToNextMultipleOf(x, y) { return Math.floor((x + y - 1) / y) * y; }
    var plainRowSize = width * sizePerPixel;
    var alignedRowSize = roundedToNextMultipleOf(plainRowSize, GL_UNPACK_ALIGNMENT);
    var bytes = (height <= 0 ? 0 : ((height - 1) * alignedRowSize + plainRowSize)) * depth;

    switch(type)
    {
        case 0x1401: return HEAPU8.subarray((pixels),(pixels+bytes)); //GL_UNSIGNED_BYTE
        case 0x1406: return HEAPF32.subarray((pixels)>>2,(pixels+bytes)>>2); //GL_FLOAT
        case 0x1405: case 0x84FA: return HEAPU32.subarray((pixels)>>2,(pixels+bytes)>>2); //GL_UNSIGNED_INT, GL_UNSIGNED_INT_24_8_WEBGL/GL_UNSIGNED_INT_24_8
        case 0x1403: case 0x8363: case 0x8033: case 0x8034: case 0x8D61: return HEAPU16.subarray((pixels)>>1,(pixels+bytes)>>1); //GL_UNSIGNED_SHORT, GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_4_4_4_4, GL_UNSIGNED_SHORT_5_5_5_1, GL_HALF_FLOAT_OES
        default: WA.error("Invalid type: %d", type); return null; //GL_INVALID_ENUM
    }
}

function webGlGetTexPixelData(type, format, width, height, pixels)
{
    var sizePerPixel;
    var numChannels;
    var heap = sys.getHeap();
    
    var HEAPU8 = new Uint8Array(heap.buffer);
    var HEAPF32 = new Float32Array(heap.buffer);
    var HEAPU16 = new Uint16Array(heap.buffer);
    var HEAPU32 = new Uint32Array(heap.buffer);
    switch(format)
    {
        case 0x1906: case 0x1909: case 0x1902: numChannels = 1; break; //GL_ALPHA, GL_LUMINANCE, GL_DEPTH_COMPONENT
        case 0x190A: numChannels = 2; break; //GL_LUMINANCE_ALPHA
        case 0x1907: case 0x8C40: numChannels = 3; break; //GL_RGB, GL_SRGB_EXT
        case 0x1908: case 0x8C42: numChannels = 4; break; //GL_RGBA, GL_SRGB_ALPHA_EXT
        default: WA.error("Invalid type: %d", type); return null; //GL_INVALID_ENUM
    }
    switch (type)
    {
        case 0x1401: sizePerPixel = numChannels*1; break; //GL_UNSIGNED_BYTE
        case 0x1403: case 0x8D61: sizePerPixel = numChannels*2; break; //GL_UNSIGNED_SHORT, GL_HALF_FLOAT_OES
        case 0x1405: case 0x1406: sizePerPixel = numChannels*4; break; //GL_UNSIGNED_INT, GL_FLOAT
        case 0x84FA: sizePerPixel = 4; break; //GL_UNSIGNED_INT_24_8_WEBGL/GL_UNSIGNED_INT_24_8
        case 0x8363: case 0x8033: case 0x8034: sizePerPixel = 2; break; //GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_4_4_4_4, GL_UNSIGNED_SHORT_5_5_5_1
        default: WA.error("Invalid type: %d", type); return null; //GL_INVALID_ENUM
    }

    function roundedToNextMultipleOf(x, y) { return Math.floor((x + y - 1) / y) * y; }
    var plainRowSize = width * sizePerPixel;
    var alignedRowSize = roundedToNextMultipleOf(plainRowSize, GL_UNPACK_ALIGNMENT);
    var bytes = (height <= 0 ? 0 : ((height - 1) * alignedRowSize + plainRowSize));

    switch(type)
    {
        case 0x1401: return HEAPU8.subarray((pixels),(pixels+bytes)); //GL_UNSIGNED_BYTE
        case 0x1406: return HEAPF32.subarray((pixels)>>2,(pixels+bytes)>>2); //GL_FLOAT
        case 0x1405: case 0x84FA: return HEAPU32.subarray((pixels)>>2,(pixels+bytes)>>2); //GL_UNSIGNED_INT, GL_UNSIGNED_INT_24_8_WEBGL/GL_UNSIGNED_INT_24_8
        case 0x1403: case 0x8363: case 0x8033: case 0x8034: case 0x8D61: return HEAPU16.subarray((pixels)>>1,(pixels+bytes)>>1); //GL_UNSIGNED_SHORT, GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_4_4_4_4, GL_UNSIGNED_SHORT_5_5_5_1, GL_HALF_FLOAT_OES
        default: WA.error("Invalid type: %d", type); return null; //GL_INVALID_ENUM
    }
}

function populateUniformTable(program)
{
    var p = glPrograms[program];
    glProgramInfos[program] = {
        uniforms : {},
        maxUniformLength: 0,
        maxUniformBlockNameLength : 0,
    };

    var ptable = glProgramInfos[program];
    var utable = ptable.uniforms;

    var numUniforms = glCtx.getProgramParameter(p, glCtx.ACTIVE_UNIFORMS);
    for (var i = 0; i < numUniforms; ++i) {
        var u = glCtx.getActiveUniform(p, i);
        var name = u.name;
        ptable.maxUniformLength = Math.max(ptable.maxUniformLength, name.length + 1);

        // strip off array specifier
        if (name.indexOf(']',name.length - 1) !== -1) {
            var ls = name.lastIndexOf('[');
            name = name.slice(0, ls);
        }

        var loc = glCtx.getUniformLocation(p, name);
        if (loc != null) {
            var id = getNewId(glUniforms);
            utable[name] = [u.size, id];
            glUniforms[id] = loc;
            for (var j = 1; j < u.size; ++j) {
                var n = name + '[' + j + ']';
                loc = glCtx.getUniformLocation(p, n);
                id = getNewId(glUniforms);
                glUniforms[id] = loc;
            }
        }
    }
}

// Init gl interface
export function importGl(env) 
{
    console.log("importGl()");

    Object.assign(env, {
        wajs_setup_gl_context: function () {
            if (!setupGlContext(WA.canvas)) return;
        },

        glGetError: function () {
            return glCtx.getError();
        },

        glEnable: function(cap) {
            glCtx.enable(cap);
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

        glColorMask: function(r, g, b, a) {
            glCtx.colorMask(r, g, b, a)
        },

        glClearDepth: function(depth) {
            glCtx.clearDepth(depth)
        },

        glDepthMask: function(flag) {
            glCtx.depthMask(flag);
        },

        glClearStencil: function(flag) {
            glCtx.clearStencil(flag)
        },

        glBindImageTexture: function(unit, texture, level, layered, layer, access, format) {
            // webgl2.0 not supported yet
            // glCtx.bindImageTexture(unit, texture, level, layered, layer, access, format)
        },

        glCreateProgram: function () {
            var id = getNewId(glPrograms);
            var program = glCtx.createProgram();
            program.name = id;
            glPrograms[id] = program;
            return id;
        },

        glDeleteProgram: function (id) {
            glCtx.deleteProgram(glPrograms[id]);
            return id;
        },

        glCreateShader: function (shaderType) {
            var id = getNewId(glShaders);
            glShaders[id] = glCtx.createShader(shaderType);
            return id;
        },

        glDeleteShader: function (id) {
            glCtx.deleteShader(glShaders[id]);
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

        glGetActiveUniform: function(program, index, bufSize, length, size, type, name) {
            var heap = sys.getHeap()
            var info = glCtx.getActiveUniform(glPrograms[program], index) 
            if (length) {
                heap.setBigUint64(length, info.length, true);
            }
            if (size) {
                heap.setInt32(size, info.size, true);
            }
            if (type) {
                heap.setInt32(type, info.type, true);
            }
            if (name) {
                sys.writeHeapString(info.name, name, bufSize)
            }
        },

        glGetShaderInfoLog: function (shader, bufSize, logSize, ptr) {
            var log = glCtx.getShaderInfoLog(glShaders[shader]);
            if (logSize) {
                var heap = sys.getHeap()
                heap.setInt32(logSize, log.length, true);
            }
            sys.writeHeapString(log, ptr, bufSize);
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
            sys.writeHeapString(log, ptr, bufSize);
        },

        glAttachShader: function (program, shader) {
            glCtx.attachShader(glPrograms[program], glShaders[shader]);
        },

        glLinkProgram: function (program) {
            glCtx.linkProgram(glPrograms[program]);
            glProgramInfos[program] = null; // uniforms no longer keep the same names after linking
            populateUniformTable(program);
        },

        glUseProgram: function (program) {
            glCtx.useProgram(program ? glPrograms[program] : null);
        },

        glGetUniformLocation : function(program, name) {
            name = sys.readHeapString(name);

            var arrayOffset = 0;
            if (name.indexOf(']', name.length - 1) !== -1) {
                var ls = name.lastIndexOf('[');
                var arrayIdx = name.slice(ls + 1, -1);
                if (arrayIdx.length > 0) {
                    arrayOffset = parseInt(arrayIdx);
                    if (arrayOffset < 0) return -1;
                }
                name = name.slice(0, ls);
            }

            var ptable = glProgramInfos[program];
            if (!ptable) return -1;

            var utable = ptable.uniforms;
            var uniformInfo = utable[name];
            if (uniformInfo && arrayOffset < uniformInfo[0]) {
                return uniformInfo[1] + arrayOffset;
            }
            return -1;
        },

        glUniform1f: function(loc, v0) { glCtx.uniform1f(glUniforms[loc], v0); },
        glUniform1fv : function(loc, count, value) { 
            var heap = sys.getHeap()
            var HEAPF32 = new Float32Array(heap.buffer);
            glCtx.uniform1fv(glUniforms[loc], HEAPF32, value >> 2, count); 
        },
        glUniform1i : function(loc, v0) {
            glCtx.uniform1i(glUniforms[loc], v0);
        },
        glUniform1iv: function(loc, count, value) {
            var heap = sys.getHeap()
            var HEAP32 = new Int32Array(heap.buffer);
            glCtx.uniform1iv(glUniforms[loc], v0, HEAP32, value>>2, count);
        },
        glUniform1ui : function(loc, v0) { glCtx.uniform1ui(glUniforms[loc], v0); },
        glUniform1uiv: function(loc, count, value) {
            var heap = sys.getHeap()
            var HEAPU32 = new Uint32Array(heap.buffer);
            glCtx.uniform1uiv(glUniforms[loc], v0, HEAPU32, value >> 2, count);
        },
        glUniform2f : function(loc, v0, v1) { glCtx.uniform2f(glUniforms[loc], v0, v1); },
        glUniform2fv : function(loc, count, value) { 
            var heap = sys.getHeap()
            var HEAPF32 = new Float32Array(heap.buffer);
            glCtx.uniform2fv(glUniforms[loc], HEAPF32, value >> 2, count * 2); 
        },
        glUniform3f : function(loc, v0, v1, v2) { glCtx.uniform3f(glUniforms[loc], v0, v1, v2); },
        glUniform3fv : function(loc, count, value) { 
            var heap = sys.getHeap()
            var HEAPF32 = new Float32Array(heap.buffer);
            glCtx.uniform3fv(glUniforms[loc], HEAPF32, value >> 2, count * 3); 
        },
        glUniform4fv : function(loc, count, value) { 
            var heap = sys.getHeap()
            var HEAPF32 = new Float32Array(heap.buffer);
            glCtx.uniform4fv(glUniforms[loc], HEAPF32, value >> 2, count * 4); 
        },
        glUniformMatrix3fv: function(loc, count, transpose, value) {
            var heap = sys.getHeap()
            var HEAPF32 = new Float32Array(heap.buffer);
            glCtx.uniformMatrix3fv(glUniforms[loc], !!transpose, HEAPF32, value>>2, count*9);
        },
        glUniformMatrix4fv: function(loc, count, transpose, value) {
            var heap = sys.getHeap()
            var HEAPF32 = new Float32Array(heap.buffer);
            glCtx.uniformMatrix4fv(glUniforms[loc], !!transpose, HEAPF32, value>>2, count*16);
        },

        glDrawArrays: function (mode, first, count) {
            glCtx.drawArrays(mode, first, count);
        },

        glDrawArraysInstanced: function (mode, first, count, instance) {
            glCtx.drawArraysInstanced(mode, first, count, instance);
        },

        glDrawElements: function(mode, count, type, offset) {
            glCtx.drawElements(mode, count, type, offset);
        },

        glDrawElementsInstanced: function(mode, count, type, offset, instance) {
            glCtx.drawElementsInstanced(mode, count, type, offset, instance);
        },

        glGenTextures: function (n, buffers) {
            genObjects(n, buffers, "createTexture", glTextures);
        },

        glDeleteTextures: function (n, buffers) {
            deleteObjects(n, buffers, "deleteTexture", glTextures);
        },

        glBindTexture: function(target, texture) {
            glCtx.bindTexture(target, glTextures[texture]);
        },

        glActiveTexture : function(texture) {
            glCtx.activeTexture(texture);
        },

        glTexImage2D : function(target, level, internalformat, width, height, border, format, type, data) {
            var pixelData = null;
            if (data) {
                pixelData = webGlGetTexPixelData(type, format, width, height, data);
            }
            glCtx.texImage2D(target, level, internalformat, width, height, border, format, type, pixelData);
        },

        glTexSubImage2D : function(target, level, x, y, width, height, format, type, data) {
            var pixelData = null;
            if (data) {
                pixelData = webGlGetTexPixelData(type, format, width, height, data);
            }
            glCtx.texSubImage2D(target, level, x, y, width, height, format, type, pixelData);
        },

        glTexSubImage3D : function(target, level, x, y, z, width, height, depth, format, type, data) {
            var pixelData = null;
            if (data) {
                pixelData = webGlGetTexPixelData3D(type, format, width, height, depth, data);
            }
            glCtx.texSubImage3D(target, level, x, y, z, width, height, depth, format, type, pixelData);
        },

        glTexStorage3D: function(target, levels, internal, width, height, depth) {
            glCtx.texStorage3D(target, levels, internal, width, height, depth)
        },

        glTexStorage3DMultisample: function(target, msaa, levels, internal, width, height, depth) {
            glCtx.texStorage3DMultisample(target, msaa, levels, internal, width, height, depth)
        },

        glTexStorage2D: function(target, levels, internal, width, height) {
            glCtx.texStorage2D(target, levels, internal, width, height)
        },

        glTexStorage2DMultisample: function(target, msaa, levels, internal, width, height) {
            glCtx.texStorage2DMultisample(target, msaa, levels, internal, width, height)
        },

        glGenRenderbuffers: function(n, buffers) {
            genObjects(n, buffers, "createRenderbuffers", glRenderBuffers);
        },

        glDeleteRenderbuffers: function(n, buffers) {
            deleteObjects(n, buffers, "deleteRenderbuffers", glRenderBuffers);
        },

        glBindRenderbuffer: function(target, buffer) {
            glCtx.bindRenderbuffer(target, buffer);
        },

        glRenderbufferStorage: function(target, internal, width, height) {
            glCtx.renderbufferStorage(target, internal, width, height);
        },

        glRenderbufferStorageMultisample: function(target, msaa, internal, width, height) {
            glCtx.renderbufferStorageMultisample(target, msaa, internal, width, height);
        },

        glPixelStorei: function(pname, v) {
            glCtx.pixelStorei(pname, v);
        },

        glGenerateMipmap : function(target) {
            glCtx.generateMipmap(target); 
        },

        glGenBuffers: function(n, buffers) {
            genObjects(n, buffers, "createBuffer", glBuffers);
        },

        glDeleteBuffers: function(n, buffers) {
            deleteObjects(n, buffers, "deleteBuffer", glBuffers);
        },

        glBindBuffer: function(target, buffer) {
            glCtx.bindBuffer(target, glBuffers[buffer]);
        },

        glBufferData: function(target, size, data, usage) {
            if (data != null) {
                var heap = sys.getHeap();
                var HEAPU8 = new Uint8Array(heap.buffer);
                glCtx.bufferData(target, HEAPU8.subarray(data, data + size), usage)
            } 
            else {
                glCtx.bufferData(target, size, usage)
            }
        },
        glBufferSubData: function(target, offset, size, data) {
            var heap = sys.getHeap();
            var HEAPU8 = new Uint8Array(heap.buffer);
            glCtx.bufferSubData(target, offset, HEAPU8.subarray(data, data + size))
        },

        glGenVertexArrays: function(n, vaos) {
            genObjects(n, vaos, "createVertexArray", glVertexArrays);
        },

        glDeleteVertexArrays: function(n, vaos) {
            deleteObjects(n, vaos, "deleteVertexArray", glVertexArrays);
        },

        glBindVertexArray : function(vao) {
            glCtx.bindVertexArray(glVertexArrays[vao]);
        },

        glEnableVertexAttribArray: function(index) { glCtx.enableVertexAttribArray(index); },
        glDisableVertexAttribArray: function(index) { glCtx.disableVertexAttribArray(index); },

        glVertexAttribIPointer:  function(index, size, type, stride, ptr) { 
            glCtx.vertexAttribIPointer(index, size, type, stride, ptr); 
        },

        glVertexAttribPointer:  function(index, size, type, normalized, stride, ptr) { 
            glCtx.vertexAttribPointer(index, size, type, !!normalized, stride, ptr); 
        },

        glVertexAttribDivisor: function(attr, divisor) {
            glCtx.vertexAttribDivisor(attr, divisor);
        },

        glBlendFuncSeparate: function(src, dst, srca, dsta) {
            glCtx.blendFuncSeparate(src, dst, srca, dsta);
        },

        glBlendColor: function(r, g, b, a) {
            glCtx.blendColor(r, g, b, a)
        },

        glDepthFunc: function(depthFunc) {
            glCtx.depthFunc(depthFunc)
        }
    });
}

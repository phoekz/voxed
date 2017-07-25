#pragma once

#include "common/base.h"

// clang-format off

#define GL_DEPTH_BUFFER_BIT 0x00000100u
#define GL_STENCIL_BUFFER_BIT 0x00000400u
#define GL_COLOR_BUFFER_BIT 0x00004000u
#define GL_FALSE 0x00000000u
#define GL_TRUE 0x00000001u
#define GL_POINTS 0x00000000u
#define GL_LINES 0x00000001u
#define GL_LINE_STRIP 0x00000003u
#define GL_TRIANGLES 0x00000004u
#define GL_TRIANGLE_STRIP 0x00000005u
#define GL_LEQUAL 0x00000203u
#define GL_SRC_ALPHA 0x00000302u
#define GL_ONE_MINUS_SRC_ALPHA 0x00000303u
#define GL_CCW 0x00000901u
#define GL_CULL_FACE 0x00000b44u
#define GL_DEPTH_TEST 0x00000b71u
#define GL_BLEND 0x00000be2u
#define GL_SCISSOR_TEST 0x00000c11u
#define GL_UNPACK_ROW_LENGTH 0x00000cf2u
#define GL_TEXTURE_2D 0x00000de1u
#define GL_UNSIGNED_BYTE 0x00001401u
#define GL_UNSIGNED_SHORT 0x00001403u
#define GL_UNSIGNED_INT 0x00001405u
#define GL_FLOAT 0x00001406u
#define GL_RGBA 0x00001908u
#define GL_NEAREST 0x00002600u
#define GL_LINEAR 0x00002601u
#define GL_TEXTURE_MAG_FILTER 0x00002800u
#define GL_TEXTURE_MIN_FILTER 0x00002801u
#define GL_TEXTURE_WRAP_S 0x00002802u
#define GL_TEXTURE_WRAP_T 0x00002803u
#define GL_RGBA8 0x00008058u
#define GL_CLAMP_TO_EDGE 0x0000812fu
#define GL_TEXTURE0 0x000084c0u
#define GL_FUNC_ADD 0x00008006u
#define GL_ARRAY_BUFFER 0x00008892u
#define GL_ELEMENT_ARRAY_BUFFER 0x00008893u
#define GL_STATIC_DRAW 0x000088e4u
#define GL_FRAGMENT_SHADER 0x00008b30u
#define GL_VERTEX_SHADER 0x00008b31u
#define GL_COMPILE_STATUS 0x00008b81u
#define GL_LINK_STATUS 0x00008b82u
#define GL_SRGB8_ALPHA8 0x00008c43u
#define GL_FRAMEBUFFER_SRGB 0x00008db9u
#define GL_SHADER_STORAGE_BUFFER 0x000090d2u

extern void(*glFrontFace)(vx::u32 mode);
extern void(*glScissor)(vx::i32 x, vx::i32 y, vx::i32 width, vx::i32 height);
extern void(*glTexImage2D)(vx::u32 target, vx::i32 level, vx::i32 internalformat, vx::i32 width, vx::i32 height, vx::i32 border, vx::u32 format, vx::u32 type, void* pixels);
extern void(*glClear)(vx::u32 mask);
extern void(*glClearColor)(float red, float green, float blue, float alpha);
extern void(*glClearStencil)(vx::i32 s);
extern void(*glClearDepth)(double depth);
extern void(*glDepthMask)(vx::u8 flag);
extern void(*glDisable)(vx::u32 cap);
extern void(*glEnable)(vx::u32 cap);
extern void(*glBlendFunc)(vx::u32 sfactor, vx::u32 dfactor);
extern void(*glDepthFunc)(vx::u32 func);
extern void(*glPixelStorei)(vx::u32 pname, vx::i32 param);
extern void(*glViewport)(vx::i32 x, vx::i32 y, vx::i32 width, vx::i32 height);
extern void(*glBindTexture)(vx::u32 target, vx::u32 texture);
extern void(*glDeleteTextures)(vx::i32 n, vx::u32* textures);
extern void(*glGenTextures)(vx::i32 n, vx::u32* textures);
extern void(*glActiveTexture)(vx::u32 texture);
extern void(*glBlendEquation)(vx::u32 mode);
extern void(*glBindBuffer)(vx::u32 target, vx::u32 buffer);
extern void(*glDeleteBuffers)(vx::i32 n, vx::u32* buffers);
extern void(*glGenBuffers)(vx::i32 n, vx::u32* buffers);
extern void(*glBufferData)(vx::u32 target, vx::iptr size, void* data, vx::u32 usage);
extern void(*glBufferSubData)(vx::u32 target, vx::iptr offset, vx::iptr size, void* data);
extern void(*glAttachShader)(vx::u32 program, vx::u32 shader);
extern void(*glCompileShader)(vx::u32 shader);
extern vx::u32(*glCreateProgram)(void);
extern vx::u32(*glCreateShader)(vx::u32 type);
extern void(*glDeleteProgram)(vx::u32 program);
extern void(*glDeleteShader)(vx::u32 shader);
extern void(*glEnableVertexAttribArray)(vx::u32 index);
extern void(*glGetProgramiv)(vx::u32 program, vx::u32 pname, vx::i32* params);
extern void(*glGetProgramInfoLog)(vx::u32 program, vx::i32 bufSize, vx::i32* length, char* infoLog);
extern void(*glGetShaderiv)(vx::u32 shader, vx::u32 pname, vx::i32* params);
extern void(*glGetShaderInfoLog)(vx::u32 shader, vx::i32 bufSize, vx::i32* length, char* infoLog);
extern void(*glLinkProgram)(vx::u32 program);
extern void(*glShaderSource)(vx::u32 shader, vx::i32 count, char** string, vx::i32* length);
extern void(*glUseProgram)(vx::u32 program);
extern void(*glUniform1i)(vx::i32 location, vx::i32 v0);
extern void(*glVertexAttribPointer)(vx::u32 index, vx::i32 size, vx::u32 type, vx::u8 normalized, vx::i32 stride, void* pointer);
extern void(*glBindBufferBase)(vx::u32 target, vx::u32 index, vx::u32 buffer);
extern void(*glBindVertexArray)(vx::u32 array);
extern void(*glGenVertexArrays)(vx::i32 n, vx::u32* arrays);
extern void(*glGenSamplers)(vx::i32 count, vx::u32* samplers);
extern void(*glDeleteSamplers)(vx::i32 count, vx::u32* samplers);
extern void(*glBindSampler)(vx::u32 unit, vx::u32 sampler);
extern void(*glSamplerParameteri)(vx::u32 sampler, vx::u32 pname, vx::i32 param);
extern void(*glDrawArraysInstancedBaseInstance)(vx::u32 mode, vx::i32 first, vx::i32 count, vx::i32 instancecount, vx::u32 baseinstance);
extern void(*glDrawElementsInstancedBaseVertexBaseInstance)(vx::u32 mode, vx::i32 count, vx::u32 type, void* indices, vx::i32 instancecount, vx::i32 basevertex, vx::u32 baseinstance);

extern void vx_gl_init(void *(*addr)(const char *));

#ifdef VX_GL_IMPLEMENTATION

void(*glFrontFace)(vx::u32 mode);
void(*glScissor)(vx::i32 x, vx::i32 y, vx::i32 width, vx::i32 height);
void(*glTexImage2D)(vx::u32 target, vx::i32 level, vx::i32 internalformat, vx::i32 width, vx::i32 height, vx::i32 border, vx::u32 format, vx::u32 type, void* pixels);
void(*glClear)(vx::u32 mask);
void(*glClearColor)(float red, float green, float blue, float alpha);
void(*glClearStencil)(vx::i32 s);
void(*glClearDepth)(double depth);
void(*glDepthMask)(vx::u8 flag);
void(*glDisable)(vx::u32 cap);
void(*glEnable)(vx::u32 cap);
void(*glBlendFunc)(vx::u32 sfactor, vx::u32 dfactor);
void(*glDepthFunc)(vx::u32 func);
void(*glPixelStorei)(vx::u32 pname, vx::i32 param);
void(*glViewport)(vx::i32 x, vx::i32 y, vx::i32 width, vx::i32 height);
void(*glBindTexture)(vx::u32 target, vx::u32 texture);
void(*glDeleteTextures)(vx::i32 n, vx::u32* textures);
void(*glGenTextures)(vx::i32 n, vx::u32* textures);
void(*glActiveTexture)(vx::u32 texture);
void(*glBlendEquation)(vx::u32 mode);
void(*glBindBuffer)(vx::u32 target, vx::u32 buffer);
void(*glDeleteBuffers)(vx::i32 n, vx::u32* buffers);
void(*glGenBuffers)(vx::i32 n, vx::u32* buffers);
void(*glBufferData)(vx::u32 target, vx::iptr size, void* data, vx::u32 usage);
void(*glBufferSubData)(vx::u32 target, vx::iptr offset, vx::iptr size, void* data);
void(*glAttachShader)(vx::u32 program, vx::u32 shader);
void(*glCompileShader)(vx::u32 shader);
vx::u32(*glCreateProgram)(void);
vx::u32(*glCreateShader)(vx::u32 type);
void(*glDeleteProgram)(vx::u32 program);
void(*glDeleteShader)(vx::u32 shader);
void(*glEnableVertexAttribArray)(vx::u32 index);
void(*glGetProgramiv)(vx::u32 program, vx::u32 pname, vx::i32* params);
void(*glGetProgramInfoLog)(vx::u32 program, vx::i32 bufSize, vx::i32* length, char* infoLog);
void(*glGetShaderiv)(vx::u32 shader, vx::u32 pname, vx::i32* params);
void(*glGetShaderInfoLog)(vx::u32 shader, vx::i32 bufSize, vx::i32* length, char* infoLog);
void(*glLinkProgram)(vx::u32 program);
void(*glShaderSource)(vx::u32 shader, vx::i32 count, char** string, vx::i32* length);
void(*glUseProgram)(vx::u32 program);
void(*glUniform1i)(vx::i32 location, vx::i32 v0);
void(*glVertexAttribPointer)(vx::u32 index, vx::i32 size, vx::u32 type, vx::u8 normalized, vx::i32 stride, void* pointer);
void(*glBindBufferBase)(vx::u32 target, vx::u32 index, vx::u32 buffer);
void(*glBindVertexArray)(vx::u32 array);
void(*glGenVertexArrays)(vx::i32 n, vx::u32* arrays);
void(*glGenSamplers)(vx::i32 count, vx::u32* samplers);
void(*glDeleteSamplers)(vx::i32 count, vx::u32* samplers);
void(*glBindSampler)(vx::u32 unit, vx::u32 sampler);
void(*glSamplerParameteri)(vx::u32 sampler, vx::u32 pname, vx::i32 param);
void(*glDrawArraysInstancedBaseInstance)(vx::u32 mode, vx::i32 first, vx::i32 count, vx::i32 instancecount, vx::u32 baseinstance);
void(*glDrawElementsInstancedBaseVertexBaseInstance)(vx::u32 mode, vx::i32 count, vx::u32 type, void* indices, vx::i32 instancecount, vx::i32 basevertex, vx::u32 baseinstance);

void vx_gl_init(void *(*addr)(const char *))
{
    glFrontFace = (void(*)(vx::u32))addr("glFrontFace");
    glScissor = (void(*)(vx::i32, vx::i32, vx::i32, vx::i32))addr("glScissor");
    glTexImage2D = (void(*)(vx::u32, vx::i32, vx::i32, vx::i32, vx::i32, vx::i32, vx::u32, vx::u32, void*))addr("glTexImage2D");
    glClear = (void(*)(vx::u32))addr("glClear");
    glClearColor = (void(*)(float, float, float, float))addr("glClearColor");
    glClearStencil = (void(*)(vx::i32))addr("glClearStencil");
    glClearDepth = (void(*)(double))addr("glClearDepth");
    glDepthMask = (void(*)(vx::u8))addr("glDepthMask");
    glDisable = (void(*)(vx::u32))addr("glDisable");
    glEnable = (void(*)(vx::u32))addr("glEnable");
    glBlendFunc = (void(*)(vx::u32, vx::u32))addr("glBlendFunc");
    glDepthFunc = (void(*)(vx::u32))addr("glDepthFunc");
    glPixelStorei = (void(*)(vx::u32, vx::i32))addr("glPixelStorei");
    glViewport = (void(*)(vx::i32, vx::i32, vx::i32, vx::i32))addr("glViewport");
    glBindTexture = (void(*)(vx::u32, vx::u32))addr("glBindTexture");
    glDeleteTextures = (void(*)(vx::i32, vx::u32*))addr("glDeleteTextures");
    glGenTextures = (void(*)(vx::i32, vx::u32*))addr("glGenTextures");
    glActiveTexture = (void(*)(vx::u32))addr("glActiveTexture");
    glBlendEquation = (void(*)(vx::u32))addr("glBlendEquation");
    glBindBuffer = (void(*)(vx::u32, vx::u32))addr("glBindBuffer");
    glDeleteBuffers = (void(*)(vx::i32, vx::u32*))addr("glDeleteBuffers");
    glGenBuffers = (void(*)(vx::i32, vx::u32*))addr("glGenBuffers");
    glBufferData = (void(*)(vx::u32, vx::iptr, void*, vx::u32))addr("glBufferData");
    glBufferSubData = (void(*)(vx::u32, vx::iptr, vx::iptr, void*))addr("glBufferSubData");
    glAttachShader = (void(*)(vx::u32, vx::u32))addr("glAttachShader");
    glCompileShader = (void(*)(vx::u32))addr("glCompileShader");
    glCreateProgram = (vx::u32(*)(void))addr("glCreateProgram");
    glCreateShader = (vx::u32(*)(vx::u32))addr("glCreateShader");
    glDeleteProgram = (void(*)(vx::u32))addr("glDeleteProgram");
    glDeleteShader = (void(*)(vx::u32))addr("glDeleteShader");
    glEnableVertexAttribArray = (void(*)(vx::u32))addr("glEnableVertexAttribArray");
    glGetProgramiv = (void(*)(vx::u32, vx::u32, vx::i32*))addr("glGetProgramiv");
    glGetProgramInfoLog = (void(*)(vx::u32, vx::i32, vx::i32*, char*))addr("glGetProgramInfoLog");
    glGetShaderiv = (void(*)(vx::u32, vx::u32, vx::i32*))addr("glGetShaderiv");
    glGetShaderInfoLog = (void(*)(vx::u32, vx::i32, vx::i32*, char*))addr("glGetShaderInfoLog");
    glLinkProgram = (void(*)(vx::u32))addr("glLinkProgram");
    glShaderSource = (void(*)(vx::u32, vx::i32, char**, vx::i32*))addr("glShaderSource");
    glUseProgram = (void(*)(vx::u32))addr("glUseProgram");
    glUniform1i = (void(*)(vx::i32, vx::i32))addr("glUniform1i");
    glVertexAttribPointer = (void(*)(vx::u32, vx::i32, vx::u32, vx::u8, vx::i32, void*))addr("glVertexAttribPointer");
    glBindBufferBase = (void(*)(vx::u32, vx::u32, vx::u32))addr("glBindBufferBase");
    glBindVertexArray = (void(*)(vx::u32))addr("glBindVertexArray");
    glGenVertexArrays = (void(*)(vx::i32, vx::u32*))addr("glGenVertexArrays");
    glGenSamplers = (void(*)(vx::i32, vx::u32*))addr("glGenSamplers");
    glDeleteSamplers = (void(*)(vx::i32, vx::u32*))addr("glDeleteSamplers");
    glBindSampler = (void(*)(vx::u32, vx::u32))addr("glBindSampler");
    glSamplerParameteri = (void(*)(vx::u32, vx::u32, vx::i32))addr("glSamplerParameteri");
    glDrawArraysInstancedBaseInstance = (void(*)(vx::u32, vx::i32, vx::i32, vx::i32, vx::u32))addr("glDrawArraysInstancedBaseInstance");
    glDrawElementsInstancedBaseVertexBaseInstance = (void(*)(vx::u32, vx::i32, vx::u32, void*, vx::i32, vx::i32, vx::u32))addr("glDrawElementsInstancedBaseVertexBaseInstance");
}

#endif // VX_GL_IMPLEMENTATION

// clang-format on

#pragma once

#include "common/base.h"

// clang-format off

#define GL_DEPTH_BUFFER_BIT 0x00000100u
#define GL_STENCIL_BUFFER_BIT 0x00000400u
#define GL_COLOR_BUFFER_BIT 0x00004000u
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
#define GL_DONT_CARE 0x00001100u
#define GL_UNSIGNED_BYTE 0x00001401u
#define GL_UNSIGNED_SHORT 0x00001403u
#define GL_UNSIGNED_INT 0x00001405u
#define GL_RGBA 0x00001908u
#define GL_NEAREST 0x00002600u
#define GL_LINEAR 0x00002601u
#define GL_TEXTURE_MAG_FILTER 0x00002800u
#define GL_TEXTURE_MIN_FILTER 0x00002801u
#define GL_TEXTURE_WRAP_S 0x00002802u
#define GL_TEXTURE_WRAP_T 0x00002803u
#define GL_RGBA8 0x00008058u
#define GL_CLAMP_TO_EDGE 0x0000812fu
#define GL_FUNC_ADD 0x00008006u
#define GL_ELEMENT_ARRAY_BUFFER 0x00008893u
#define GL_FRAGMENT_SHADER 0x00008b30u
#define GL_VERTEX_SHADER 0x00008b31u
#define GL_LINK_STATUS 0x00008b82u
#define GL_INFO_LOG_LENGTH 0x00008b84u
#define GL_SRGB8_ALPHA8 0x00008c43u
#define GL_FRAMEBUFFER_SRGB 0x00008db9u
#define GL_VERTEX_SHADER_BIT 0x00000001u
#define GL_FRAGMENT_SHADER_BIT 0x00000002u
#define GL_DEBUG_OUTPUT_SYNCHRONOUS 0x00008242u
#define GL_DEBUG_SOURCE_API 0x00008246u
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM 0x00008247u
#define GL_DEBUG_SOURCE_SHADER_COMPILER 0x00008248u
#define GL_DEBUG_SOURCE_THIRD_PARTY 0x00008249u
#define GL_DEBUG_SOURCE_APPLICATION 0x0000824au
#define GL_DEBUG_SOURCE_OTHER 0x0000824bu
#define GL_DEBUG_TYPE_ERROR 0x0000824cu
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x0000824du
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR 0x0000824eu
#define GL_DEBUG_TYPE_PORTABILITY 0x0000824fu
#define GL_DEBUG_TYPE_PERFORMANCE 0x00008250u
#define GL_DEBUG_TYPE_OTHER 0x00008251u
#define GL_DEBUG_SEVERITY_HIGH 0x00009146u
#define GL_DEBUG_SEVERITY_MEDIUM 0x00009147u
#define GL_DEBUG_SEVERITY_LOW 0x00009148u
#define GL_DEBUG_TYPE_MARKER 0x00008268u
#define GL_DEBUG_SEVERITY_NOTIFICATION 0x0000826bu
#define GL_DEBUG_OUTPUT 0x000092e0u
#define GL_SHADER_STORAGE_BUFFER 0x000090d2u
#define GL_DYNAMIC_STORAGE_BIT 0x00000100u

typedef void(vx_gl_debug_proc)(vx::i32, vx::i32, vx::u32, vx::i32, vx::iptr, const char*, void*);

extern void(*glFrontFace)(vx::u32 mode);
extern void(*glScissor)(vx::i32 x, vx::i32 y, vx::i32 width, vx::i32 height);
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
extern void(*glDeleteTextures)(vx::i32 n, vx::u32* textures);
extern void(*glBlendEquation)(vx::u32 mode);
extern void(*glBindBuffer)(vx::u32 target, vx::u32 buffer);
extern void(*glDeleteBuffers)(vx::i32 n, vx::u32* buffers);
extern void(*glDeleteProgram)(vx::u32 program);
extern void(*glGetProgramiv)(vx::u32 program, vx::u32 pname, vx::i32* params);
extern void(*glGetProgramInfoLog)(vx::u32 program, vx::i32 bufSize, vx::i32* length, char* infoLog);
extern void(*glBindBufferBase)(vx::u32 target, vx::u32 index, vx::u32 buffer);
extern void(*glBindVertexArray)(vx::u32 array);
extern void(*glDeleteSamplers)(vx::i32 count, vx::u32* samplers);
extern void(*glSamplerParameteri)(vx::u32 sampler, vx::u32 pname, vx::i32 param);
extern void(*glUseProgramStages)(vx::u32 pipeline, vx::u32 stages, vx::u32 program);
extern vx::u32(*glCreateShaderProgramv)(vx::u32 type, vx::i32 count, char** strings);
extern void(*glBindProgramPipeline)(vx::u32 pipeline);
extern void(*glDeleteProgramPipelines)(vx::i32 n, vx::u32* pipelines);
extern void(*glProgramUniform1i)(vx::u32 program, vx::i32 location, vx::i32 v0);
extern void(*glDrawArraysInstancedBaseInstance)(vx::u32 mode, vx::i32 first, vx::i32 count, vx::i32 instancecount, vx::u32 baseinstance);
extern void(*glDrawElementsInstancedBaseVertexBaseInstance)(vx::u32 mode, vx::i32 count, vx::u32 type, void* indices, vx::i32 instancecount, vx::i32 basevertex, vx::u32 baseinstance);
extern void(*glDebugMessageControl)(vx::u32 source, vx::u32 type, vx::u32 severity, vx::i32 count, vx::u32* ids, vx::u8 enabled);
extern void(*glDebugMessageCallback)(vx_gl_debug_proc callback, void* userParam);
extern void(*glBindSamplers)(vx::u32 first, vx::i32 count, vx::u32* samplers);
extern void(*glCreateBuffers)(vx::i32 n, vx::u32* buffers);
extern void(*glNamedBufferStorage)(vx::u32 buffer, vx::iptr size, void* data, vx::u32 flags);
extern void(*glNamedBufferSubData)(vx::u32 buffer, vx::iptr offset, vx::iptr size, void* data);
extern void(*glCreateTextures)(vx::u32 target, vx::i32 n, vx::u32* textures);
extern void(*glTextureStorage2D)(vx::u32 texture, vx::i32 levels, vx::u32 internalformat, vx::i32 width, vx::i32 height);
extern void(*glTextureSubImage2D)(vx::u32 texture, vx::i32 level, vx::i32 xoffset, vx::i32 yoffset, vx::i32 width, vx::i32 height, vx::u32 format, vx::u32 type, void* pixels);
extern void(*glBindTextureUnit)(vx::u32 unit, vx::u32 texture);
extern void(*glCreateVertexArrays)(vx::i32 n, vx::u32* arrays);
extern void(*glCreateSamplers)(vx::i32 n, vx::u32* samplers);
extern void(*glCreateProgramPipelines)(vx::i32 n, vx::u32* pipelines);

extern void vx_gl_init(void *(*addr)(const char *));

#ifdef VX_GL_IMPLEMENTATION

void(*glFrontFace)(vx::u32 mode);
void(*glScissor)(vx::i32 x, vx::i32 y, vx::i32 width, vx::i32 height);
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
void(*glDeleteTextures)(vx::i32 n, vx::u32* textures);
void(*glBlendEquation)(vx::u32 mode);
void(*glBindBuffer)(vx::u32 target, vx::u32 buffer);
void(*glDeleteBuffers)(vx::i32 n, vx::u32* buffers);
void(*glDeleteProgram)(vx::u32 program);
void(*glGetProgramiv)(vx::u32 program, vx::u32 pname, vx::i32* params);
void(*glGetProgramInfoLog)(vx::u32 program, vx::i32 bufSize, vx::i32* length, char* infoLog);
void(*glBindBufferBase)(vx::u32 target, vx::u32 index, vx::u32 buffer);
void(*glBindVertexArray)(vx::u32 array);
void(*glDeleteSamplers)(vx::i32 count, vx::u32* samplers);
void(*glSamplerParameteri)(vx::u32 sampler, vx::u32 pname, vx::i32 param);
void(*glUseProgramStages)(vx::u32 pipeline, vx::u32 stages, vx::u32 program);
vx::u32(*glCreateShaderProgramv)(vx::u32 type, vx::i32 count, char** strings);
void(*glBindProgramPipeline)(vx::u32 pipeline);
void(*glDeleteProgramPipelines)(vx::i32 n, vx::u32* pipelines);
void(*glProgramUniform1i)(vx::u32 program, vx::i32 location, vx::i32 v0);
void(*glDrawArraysInstancedBaseInstance)(vx::u32 mode, vx::i32 first, vx::i32 count, vx::i32 instancecount, vx::u32 baseinstance);
void(*glDrawElementsInstancedBaseVertexBaseInstance)(vx::u32 mode, vx::i32 count, vx::u32 type, void* indices, vx::i32 instancecount, vx::i32 basevertex, vx::u32 baseinstance);
void(*glDebugMessageControl)(vx::u32 source, vx::u32 type, vx::u32 severity, vx::i32 count, vx::u32* ids, vx::u8 enabled);
void(*glDebugMessageCallback)(vx_gl_debug_proc callback, void* userParam);
void(*glBindSamplers)(vx::u32 first, vx::i32 count, vx::u32* samplers);
void(*glCreateBuffers)(vx::i32 n, vx::u32* buffers);
void(*glNamedBufferStorage)(vx::u32 buffer, vx::iptr size, void* data, vx::u32 flags);
void(*glNamedBufferSubData)(vx::u32 buffer, vx::iptr offset, vx::iptr size, void* data);
void(*glCreateTextures)(vx::u32 target, vx::i32 n, vx::u32* textures);
void(*glTextureStorage2D)(vx::u32 texture, vx::i32 levels, vx::u32 internalformat, vx::i32 width, vx::i32 height);
void(*glTextureSubImage2D)(vx::u32 texture, vx::i32 level, vx::i32 xoffset, vx::i32 yoffset, vx::i32 width, vx::i32 height, vx::u32 format, vx::u32 type, void* pixels);
void(*glBindTextureUnit)(vx::u32 unit, vx::u32 texture);
void(*glCreateVertexArrays)(vx::i32 n, vx::u32* arrays);
void(*glCreateSamplers)(vx::i32 n, vx::u32* samplers);
void(*glCreateProgramPipelines)(vx::i32 n, vx::u32* pipelines);

void vx_gl_init(void *(*addr)(const char *))
{
    glFrontFace = (void(*)(vx::u32))addr("glFrontFace");
    glScissor = (void(*)(vx::i32, vx::i32, vx::i32, vx::i32))addr("glScissor");
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
    glDeleteTextures = (void(*)(vx::i32, vx::u32*))addr("glDeleteTextures");
    glBlendEquation = (void(*)(vx::u32))addr("glBlendEquation");
    glBindBuffer = (void(*)(vx::u32, vx::u32))addr("glBindBuffer");
    glDeleteBuffers = (void(*)(vx::i32, vx::u32*))addr("glDeleteBuffers");
    glDeleteProgram = (void(*)(vx::u32))addr("glDeleteProgram");
    glGetProgramiv = (void(*)(vx::u32, vx::u32, vx::i32*))addr("glGetProgramiv");
    glGetProgramInfoLog = (void(*)(vx::u32, vx::i32, vx::i32*, char*))addr("glGetProgramInfoLog");
    glBindBufferBase = (void(*)(vx::u32, vx::u32, vx::u32))addr("glBindBufferBase");
    glBindVertexArray = (void(*)(vx::u32))addr("glBindVertexArray");
    glDeleteSamplers = (void(*)(vx::i32, vx::u32*))addr("glDeleteSamplers");
    glSamplerParameteri = (void(*)(vx::u32, vx::u32, vx::i32))addr("glSamplerParameteri");
    glUseProgramStages = (void(*)(vx::u32, vx::u32, vx::u32))addr("glUseProgramStages");
    glCreateShaderProgramv = (vx::u32(*)(vx::u32, vx::i32, char**))addr("glCreateShaderProgramv");
    glBindProgramPipeline = (void(*)(vx::u32))addr("glBindProgramPipeline");
    glDeleteProgramPipelines = (void(*)(vx::i32, vx::u32*))addr("glDeleteProgramPipelines");
    glProgramUniform1i = (void(*)(vx::u32, vx::i32, vx::i32))addr("glProgramUniform1i");
    glDrawArraysInstancedBaseInstance = (void(*)(vx::u32, vx::i32, vx::i32, vx::i32, vx::u32))addr("glDrawArraysInstancedBaseInstance");
    glDrawElementsInstancedBaseVertexBaseInstance = (void(*)(vx::u32, vx::i32, vx::u32, void*, vx::i32, vx::i32, vx::u32))addr("glDrawElementsInstancedBaseVertexBaseInstance");
    glDebugMessageControl = (void(*)(vx::u32, vx::u32, vx::u32, vx::i32, vx::u32*, vx::u8))addr("glDebugMessageControl");
    glDebugMessageCallback = (void(*)(vx_gl_debug_proc, void*))addr("glDebugMessageCallback");
    glBindSamplers = (void(*)(vx::u32, vx::i32, vx::u32*))addr("glBindSamplers");
    glCreateBuffers = (void(*)(vx::i32, vx::u32*))addr("glCreateBuffers");
    glNamedBufferStorage = (void(*)(vx::u32, vx::iptr, void*, vx::u32))addr("glNamedBufferStorage");
    glNamedBufferSubData = (void(*)(vx::u32, vx::iptr, vx::iptr, void*))addr("glNamedBufferSubData");
    glCreateTextures = (void(*)(vx::u32, vx::i32, vx::u32*))addr("glCreateTextures");
    glTextureStorage2D = (void(*)(vx::u32, vx::i32, vx::u32, vx::i32, vx::i32))addr("glTextureStorage2D");
    glTextureSubImage2D = (void(*)(vx::u32, vx::i32, vx::i32, vx::i32, vx::i32, vx::i32, vx::u32, vx::u32, void*))addr("glTextureSubImage2D");
    glBindTextureUnit = (void(*)(vx::u32, vx::u32))addr("glBindTextureUnit");
    glCreateVertexArrays = (void(*)(vx::i32, vx::u32*))addr("glCreateVertexArrays");
    glCreateSamplers = (void(*)(vx::i32, vx::u32*))addr("glCreateSamplers");
    glCreateProgramPipelines = (void(*)(vx::i32, vx::u32*))addr("glCreateProgramPipelines");
}

#endif // VX_GL_IMPLEMENTATION

// clang-format on

enums = [
    # Types
    "GL_UNSIGNED_BYTE",
    "GL_UNSIGNED_SHORT",
    "GL_UNSIGNED_INT",
    # Comparison Functions
    "GL_LEQUAL",
    # Fixed-Function State
    "GL_BLEND",
    "GL_CULL_FACE",
    "GL_DEPTH_TEST",
    "GL_SCISSOR_TEST",
    # Winding Order
    "GL_CCW",
    # Blending
    "GL_FUNC_ADD",
    "GL_SRC_ALPHA",
    "GL_ONE_MINUS_SRC_ALPHA",
    # Buffer Objects
    "GL_ELEMENT_ARRAY_BUFFER",
    "GL_SHADER_STORAGE_BUFFER",
    "GL_DYNAMIC_STORAGE_BIT",
    # Textures
    "GL_TEXTURE0",
    "GL_TEXTURE_2D",
    "GL_TEXTURE_WRAP_S",
    "GL_TEXTURE_WRAP_T",
    "GL_TEXTURE_MIN_FILTER",
    "GL_TEXTURE_MAG_FILTER",
    "GL_UNPACK_ROW_LENGTH",
    # Texture Internal Formats
    "GL_RGBA8",
    "GL_SRGB8_ALPHA8",
    # Texture Formats
    "GL_RGBA",
    # Address Modes
    "GL_CLAMP_TO_EDGE",
    # Filter Modes
    "GL_NEAREST",
    "GL_LINEAR",
    # Framebuffers
    "GL_COLOR_BUFFER_BIT",
    "GL_DEPTH_BUFFER_BIT",
    "GL_STENCIL_BUFFER_BIT",
    "GL_FRAMEBUFFER_SRGB",
    # Primitives
    "GL_POINTS",
    "GL_LINES",
    "GL_LINE_STRIP",
    "GL_TRIANGLES",
    "GL_TRIANGLE_STRIP",
    # Shaders
    "GL_VERTEX_SHADER",
    "GL_FRAGMENT_SHADER",
    "GL_VERTEX_SHADER_BIT",
    "GL_FRAGMENT_SHADER_BIT",
    "GL_LINK_STATUS",
    # Introspection
    "GL_INFO_LOG_LENGTH",
    # Debug
    "GL_DONT_CARE",
    "GL_DEBUG_OUTPUT",
    "GL_DEBUG_OUTPUT_SYNCHRONOUS",
    "GL_DEBUG_SOURCE_API",
    "GL_DEBUG_SOURCE_WINDOW_SYSTEM",
    "GL_DEBUG_SOURCE_SHADER_COMPILER",
    "GL_DEBUG_SOURCE_THIRD_PARTY",
    "GL_DEBUG_SOURCE_APPLICATION",
    "GL_DEBUG_SOURCE_OTHER",
    "GL_DEBUG_TYPE_ERROR",
    "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR",
    "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR",
    "GL_DEBUG_TYPE_PORTABILITY",
    "GL_DEBUG_TYPE_PERFORMANCE",
    "GL_DEBUG_TYPE_MARKER",
    "GL_DEBUG_TYPE_OTHER",
    "GL_DEBUG_SEVERITY_HIGH",
    "GL_DEBUG_SEVERITY_MEDIUM",
    "GL_DEBUG_SEVERITY_LOW",
    "GL_DEBUG_SEVERITY_NOTIFICATION",
]

functions = [
    # Fixed-Function State
    "glEnable",
    "glDisable",
    "glFrontFace",
    "glDepthFunc",
    "glDepthMask",
    "glScissor",
    "glViewport",
    "glBlendEquation",
    "glBlendFunc",
    # Clear
    "glClear",
    "glClearColor",
    "glClearDepth",
    "glClearStencil",
    # Buffer Objects
    "glCreateBuffers",
    "glBindBuffer",
    "glBindBufferBase",
    "glNamedBufferStorage",
    "glNamedBufferSubData",
    "glDeleteBuffers",
    # Vertex Array Objects
    "glCreateVertexArrays",
    "glBindVertexArray",
    # Textures
    "glCreateTextures",
    "glTextureStorage2D",
    "glTextureSubImage2D",
    "glBindTextureUnit",
    "glPixelStorei",
    "glDeleteTextures",
    # Samplers
    "glCreateSamplers",
    "glBindSamplers",
    "glSamplerParameteri",
    "glDeleteSamplers",
    # Shaders
    "glCreateShaderProgramv",
    "glCreateProgramPipelines",
    "glUseProgramStages",
    "glBindProgramPipeline",
    "glDeleteProgram",
    "glDeleteProgramPipelines",
    # Uniforms
    "glProgramUniform1i",
    # Draw Calls
    "glDrawArraysInstancedBaseInstance",
    "glDrawElementsInstancedBaseVertexBaseInstance",
    # Introspection
    "glGetProgramiv",
    "glGetProgramInfoLog",
    # Debug
    "glDebugMessageCallback",
    "glDebugMessageControl",
]

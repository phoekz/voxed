enums = [
    # Constants
    "GL_TRUE",
    "GL_FALSE",
    # Types
    "GL_UNSIGNED_BYTE",
    "GL_UNSIGNED_SHORT",
    "GL_UNSIGNED_INT",
    "GL_FLOAT",
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
    "GL_ARRAY_BUFFER",
    "GL_ELEMENT_ARRAY_BUFFER",
    "GL_SHADER_STORAGE_BUFFER",
    "GL_STATIC_DRAW",
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
    "GL_COMPILE_STATUS",
    "GL_LINK_STATUS",
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
    "glGenBuffers",
    "glBindBuffer",
    "glBindBufferBase",
    "glBufferData",
    "glBufferSubData",
    "glDeleteBuffers",
    # Vertex Array Objects
    "glGenVertexArrays",
    "glBindVertexArray",
    # Vertex Attributes
    "glEnableVertexAttribArray",
    "glVertexAttribPointer",
    # Textures
    "glGenTextures",
    "glBindTexture",
    "glActiveTexture",
    "glTexImage2D",
    "glPixelStorei",
    "glDeleteTextures",
    # Samplers
    "glGenSamplers",
    "glBindSampler",
    "glSamplerParameteri",
    "glDeleteSamplers",
    # Shaders
    "glCreateShader",
    "glShaderSource",
    "glCompileShader",
    "glCreateProgram",
    "glAttachShader",
    "glLinkProgram",
    "glUseProgram",
    "glDeleteShader",
    "glDeleteProgram",
    # Uniforms
    "glUniform1i",
    # Draw Calls
    "glDrawArraysInstancedBaseInstance",
    "glDrawElementsInstancedBaseVertexBaseInstance",
    # Introspection
    "glGetShaderiv",
    "glGetShaderInfoLog",
    "glGetProgramiv",
    "glGetProgramInfoLog",
]

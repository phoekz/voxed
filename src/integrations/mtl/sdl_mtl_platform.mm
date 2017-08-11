#include "platform/gpu.h"

#include <SDL_syswm.h>

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace
{
struct mtl_device
{
    id<MTLDevice> device;
    CAMetalLayer* layer;
    NSView* view;
    id<MTLCommandQueue> queue;

    MTLRenderPassDescriptor* main_pass;
    id<MTLTexture> depth_texture, stencil_texture;
    MTLPixelFormat color_format, depth_format, stencil_format;

    NSAutoreleasePool* release_pool;
    id<MTLCommandBuffer> cmdbuf;
    id<CAMetalDrawable> drawable;
};

struct mtl_pipeline
{
    id<MTLRenderPipelineState> pipeline;
    id<MTLDepthStencilState> depth_stencil;
    MTLCullMode cull_mode;
};
}

namespace vx
{
void platform_init(platform* platform, const char* title, int2 initial_size)
{
    //
    // Window
    //

    SDL_Window* sdl_window;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");

    sdl_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        initial_size.x,
        initial_size.y,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!sdl_window)
        fatal("SDL_CreateWindow failed with error: %s", SDL_GetError());

    platform->window = sdl_window;

    SDL_SysWMinfo syswm;
    SDL_VERSION(&syswm.version);
    SDL_GetWindowWMInfo(sdl_window, &syswm);

    //
    // Metal
    //

    mtl_device* mtl = (mtl_device*)calloc(1, sizeof(mtl_device));
    platform->gpu = (gpu_device*)mtl;

    mtl->device = MTLCreateSystemDefaultDevice();

    mtl->color_format = MTLPixelFormatRGBA8Unorm;
    mtl->depth_format = MTLPixelFormatDepth32Float;
    mtl->stencil_format = MTLPixelFormatStencil8;

    mtl->layer = [CAMetalLayer layer];
    mtl->layer.device = mtl->device;
    mtl->layer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    [mtl->layer retain];

    mtl->view = [syswm.info.cocoa.window contentView];
    [mtl->view setWantsLayer:YES];
    [mtl->view setLayer:mtl->layer];

    mtl->queue = [mtl->device newCommandQueue];

    //
    // Main render pass
    //

    // Depth & stencil textures

    MTLTextureDescriptor* depth_desc;
    MTLTextureDescriptor* stencil_desc;

    depth_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:mtl->depth_format
                                                                    width:initial_size.x
                                                                   height:initial_size.y
                                                                mipmapped:NO];

    stencil_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:mtl->stencil_format
                                                                      width:initial_size.x
                                                                     height:initial_size.y
                                                                  mipmapped:NO];

    depth_desc.textureType = MTLTextureType2D;
    depth_desc.sampleCount = 1;
    depth_desc.usage = MTLTextureUsageUnknown;
    depth_desc.storageMode = MTLStorageModePrivate;

    stencil_desc.textureType = MTLTextureType2D;
    stencil_desc.sampleCount = 1;
    stencil_desc.usage = MTLTextureUsageUnknown;
    stencil_desc.storageMode = MTLStorageModePrivate;

    mtl->depth_texture = [mtl->device newTextureWithDescriptor:depth_desc];
    mtl->stencil_texture = [mtl->device newTextureWithDescriptor:stencil_desc];

    // Attachments

    MTLRenderPassColorAttachmentDescriptor* color_attachment;
    MTLRenderPassDepthAttachmentDescriptor* depth_attachment;
    MTLRenderPassStencilAttachmentDescriptor* stencil_attachment;

    mtl->main_pass = [MTLRenderPassDescriptor renderPassDescriptor];

    color_attachment = mtl->main_pass.colorAttachments[0];
    depth_attachment = mtl->main_pass.depthAttachment;
    stencil_attachment = mtl->main_pass.stencilAttachment;

    color_attachment.loadAction = MTLLoadActionClear;
    color_attachment.storeAction = MTLStoreActionStore;
    color_attachment.clearColor = MTLClearColorMake(0.95, 0.95, 0.95, 1.0);

    depth_attachment.texture = mtl->depth_texture;
    depth_attachment.loadAction = MTLLoadActionClear;
    depth_attachment.storeAction = MTLStoreActionDontCare;
    depth_attachment.clearDepth = 1.0;

    stencil_attachment.texture = mtl->stencil_texture;
    stencil_attachment.loadAction = MTLLoadActionClear;
    stencil_attachment.storeAction = MTLStoreActionDontCare;
    stencil_attachment.clearStencil = 0;
}

void platform_quit(platform* platform)
{
    SDL_Window* sdl_window;
    mtl_device* device;

    sdl_window = (SDL_Window*)platform->window;
    device = (mtl_device*)platform->gpu;
    free(device);

    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
}

void platform_frame_begin(platform* platform)
{
    mtl_device* mtl = (mtl_device*)platform->gpu;

    mtl->release_pool = [[NSAutoreleasePool alloc] init];
    mtl->drawable = [mtl->layer nextDrawable];
    mtl->main_pass.colorAttachments[0].texture = mtl->drawable.texture;

    mtl->cmdbuf = [mtl->queue commandBuffer];
}

void platform_frame_end(platform* platform)
{
    mtl_device* mtl = (mtl_device*)platform->gpu;

    [mtl->cmdbuf presentDrawable:mtl->drawable];
    [mtl->cmdbuf commit];
    [mtl->release_pool release];
}

gpu_buffer* gpu_buffer_create(gpu_device* gpu, usize size, gpu_buffer_type /*type*/)
{
    mtl_device* mtl = (mtl_device*)gpu;
    return (gpu_buffer*)[mtl->device newBufferWithLength:size
                                                 options:MTLResourceCPUCacheModeDefaultCache];
}

void gpu_buffer_update(gpu_device*, gpu_buffer* buffer, void* data, usize size, usize offset)
{
    char* dst = (char*)[(id<MTLBuffer>)buffer contents];
    memcpy(dst + offset, data, size);
    NSRange range = NSMakeRange(offset, size);
    [(id<MTLBuffer>)buffer didModifyRange:range];
}

void gpu_buffer_destroy(gpu_device* /*gpu*/, gpu_buffer* buffer)
{
    [(id<MTLBuffer>)buffer release];
}

gpu_texture* gpu_texture_create(
    gpu_device* gpu,
    u32 width,
    u32 height,
    gpu_pixel_format format,
    void* data)
{
    mtl_device* mtl;
    MTLPixelFormat pixel_format;
    MTLTextureDescriptor* desc;
    id<MTLTexture> texture;

    mtl = (mtl_device*)gpu;

    switch (format)
    {
        case gpu_pixel_format::rgba8_unorm:
            pixel_format = MTLPixelFormatRGBA8Unorm;
            break;
        case gpu_pixel_format::rgba8_unorm_srgb:
            pixel_format = MTLPixelFormatRGBA8Unorm_sRGB;
            break;
        default:
            fatal("Invalid gpu texture format: %d", format);
            break;
    }

    desc = [[MTLTextureDescriptor alloc] init];
    desc.textureType = MTLTextureType2D;
    desc.pixelFormat = pixel_format;
    desc.width = width;
    desc.height = height;
    desc.depth = 1;
    desc.mipmapLevelCount = 1;
    desc.sampleCount = 1;
    desc.arrayLength = 1;

    texture = [mtl->device newTextureWithDescriptor:desc];

    [texture replaceRegion:MTLRegionMake2D(0, 0, width, height)
               mipmapLevel:0
                 withBytes:data
               bytesPerRow:4 * width];

    [desc release];

    return (gpu_texture*)texture;
}

void gpu_texture_destroy(gpu_device* /*gpu*/, gpu_texture* texture)
{
    [(id<MTLTexture>)texture release];
}

gpu_sampler* gpu_sampler_create(
    gpu_device* gpu,
    gpu_filter_mode min,
    gpu_filter_mode mag,
    gpu_filter_mode mip)
{
    mtl_device* mtl;
    id<MTLSamplerState> sampler;
    MTLSamplerDescriptor* desc;
    MTLSamplerMinMagFilter mtl_min;
    MTLSamplerMinMagFilter mtl_mag;
    MTLSamplerMipFilter mtl_mip;
    MTLSamplerAddressMode s_address;
    MTLSamplerAddressMode t_address;
    MTLSamplerAddressMode r_address;

    mtl = (mtl_device*)gpu;

    mtl_min = (min == gpu_filter_mode::nearest) ? MTLSamplerMinMagFilterNearest
                                                : MTLSamplerMinMagFilterLinear;

    mtl_mag = (mag == gpu_filter_mode::nearest) ? MTLSamplerMinMagFilterNearest
                                                : MTLSamplerMinMagFilterLinear;

    mtl_mip =
        (mip == gpu_filter_mode::nearest) ? MTLSamplerMipFilterNearest : MTLSamplerMipFilterLinear;

    s_address = MTLSamplerAddressModeClampToEdge;
    t_address = MTLSamplerAddressModeClampToEdge;
    r_address = MTLSamplerAddressModeClampToEdge;

    desc = [[MTLSamplerDescriptor alloc] init];
    desc.minFilter = mtl_min;
    desc.magFilter = mtl_mag;
    desc.mipFilter = mtl_mip;
    desc.sAddressMode = s_address;
    desc.tAddressMode = t_address;
    desc.rAddressMode = r_address;
    desc.maxAnisotropy = 1;
    desc.normalizedCoordinates = YES;
    desc.lodMinClamp = 0.0f;
    desc.lodMaxClamp = FLT_MAX;

    sampler = [mtl->device newSamplerStateWithDescriptor:desc];

    [desc release];

    return (gpu_sampler*)sampler;
}

void gpu_sampler_destroy(gpu_device* /*gpu*/, gpu_sampler* sampler)
{
    [(id<MTLSamplerState>)sampler release];
}

gpu_vertex_desc* gpu_vertex_desc_create(
    gpu_device*,
    gpu_vertex_desc_attribute* attributes,
    u32 attribute_count,
    u32 vertex_stride)
{
    MTLVertexDescriptor* vertex_desc = [[MTLVertexDescriptor alloc] init];
    [vertex_desc retain];

    for (u32 i = 0; i < attribute_count; ++i)
    {
        switch (attributes[i].format)
        {
            case gpu_vertex_format::float1:
                vertex_desc.attributes[i].format = MTLVertexFormatFloat;
                break;
            case gpu_vertex_format::float2:
                vertex_desc.attributes[i].format = MTLVertexFormatFloat2;
                break;
            case gpu_vertex_format::float3:
                vertex_desc.attributes[i].format = MTLVertexFormatFloat3;
                break;
            case gpu_vertex_format::float4:
                vertex_desc.attributes[i].format = MTLVertexFormatFloat4;
                break;
            case gpu_vertex_format::rgba8_unorm:
                vertex_desc.attributes[i].format = MTLVertexFormatUChar4Normalized;
                break;
            default:
                fatal("Invalid vertex format: %d", attributes[i].format);
        }

        vertex_desc.attributes[i].bufferIndex = attributes[i].buffer_index;
        vertex_desc.attributes[i].offset = attributes[i].offset;
    }

    vertex_desc.layouts[0].stride = vertex_stride;
    vertex_desc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

    return (gpu_vertex_desc*)vertex_desc;
}

void gpu_vertex_desc_destroy(gpu_device* gpu, gpu_vertex_desc* vertex_desc)
{
    (void)gpu;
    (void)vertex_desc;
}

gpu_shader* gpu_shader_create(
    gpu_device* gpu,
    gpu_shader_type,
    void* data,
    usize size,
    const char* main_function)
{
    mtl_device* mtl;
    dispatch_data_t ddata;
    id<MTLLibrary> lib;
    id<MTLFunction> func;
    NSError* error;

    mtl = (mtl_device*)gpu;
    ddata = dispatch_data_create(data, size, NULL, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    lib = [mtl->device newLibraryWithData:ddata error:&error];

    if (!lib)
        fatal("Metal library error: %s", error.description.UTF8String);

    func = [lib newFunctionWithName:@(main_function)];

    return (gpu_shader*)func;
}

void gpu_shader_destroy(gpu_device* gpu, gpu_shader* shader)
{
    (void)gpu;
    (void)shader;
}

gpu_pipeline* gpu_pipeline_create(
    gpu_device* gpu,
    gpu_shader* vertex_shader,
    gpu_shader* fragment_shader,
    const gpu_pipeline_options& options)
{
    mtl_device* mtl;
    id<MTLRenderPipelineState> pipeline;
    id<MTLDepthStencilState> depth_stencil;
    MTLRenderPipelineDescriptor* pdesc;
    MTLDepthStencilDescriptor* dsdesc;
    NSError* error;

    mtl = (mtl_device*)gpu;
    pdesc = [[MTLRenderPipelineDescriptor alloc] init];
    pdesc.sampleCount = 1;
    pdesc.vertexFunction = (id<MTLFunction>)vertex_shader;
    pdesc.fragmentFunction = (id<MTLFunction>)fragment_shader;

    MTLRenderPipelineColorAttachmentDescriptor* color_attachment = pdesc.colorAttachments[0];
    color_attachment.pixelFormat = MTLPixelFormatRGBA8Unorm;
    color_attachment.blendingEnabled = YES;

    // TODO(vinht): User-defined blending.
    color_attachment.rgbBlendOperation = MTLBlendOperationAdd;
    color_attachment.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    color_attachment.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

    color_attachment.alphaBlendOperation = MTLBlendOperationAdd;
    color_attachment.sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
    color_attachment.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

    pipeline = [mtl->device newRenderPipelineStateWithDescriptor:pdesc error:&error];

    if (!pipeline)
        fatal("Pipeline create error: %s", error.description.UTF8String);

    [pdesc release];

    dsdesc = [[MTLDepthStencilDescriptor alloc] init];
    dsdesc.depthCompareFunction =
        options.depth_test_enabled ? MTLCompareFunctionLess : MTLCompareFunctionAlways;
    dsdesc.depthWriteEnabled = YES;
    depth_stencil = [mtl->device newDepthStencilStateWithDescriptor:dsdesc];
    [dsdesc release];

    mtl_pipeline* mtlp = (mtl_pipeline*)std::malloc(sizeof(mtl_pipeline));
    mtlp->pipeline = pipeline;
    mtlp->depth_stencil = depth_stencil;
    mtlp->cull_mode = options.culling_enabled ? MTLCullModeBack : MTLCullModeNone;
    return (gpu_pipeline*)mtlp;
}

void gpu_pipeline_destroy(gpu_device* /*gpu*/, gpu_pipeline* pipeline)
{
    mtl_pipeline* mtlp = (mtl_pipeline*)pipeline;
    [mtlp->pipeline release];
    [mtlp->depth_stencil release];
    std::free(mtlp);
}

gpu_channel* gpu_channel_open(gpu_device* gpu)
{
    mtl_device* mtl = (mtl_device*)gpu;
    return (gpu_channel*)[mtl->cmdbuf renderCommandEncoderWithDescriptor:mtl->main_pass];
}

void gpu_channel_close(gpu_device*, gpu_channel* channel)
{
    [(id<MTLRenderCommandEncoder>)channel endEncoding];
}

void gpu_channel_clear_cmd(gpu_channel* /*channel*/, gpu_clear_cmd_args* /*args*/) {}

void gpu_channel_set_buffer_cmd(gpu_channel* channel, gpu_buffer* buffer, u32 index)
{
    id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>)channel;
    [encoder setVertexBuffer:(id<MTLBuffer>)buffer offset:0 atIndex:index];
    [encoder setFragmentBuffer:(id<MTLBuffer>)buffer offset:0 atIndex:index];
}

void gpu_channel_set_vertex_desc_cmd(gpu_channel* /*channel*/, gpu_vertex_desc* /*vertex_desc*/) {}

void gpu_channel_set_texture_cmd(gpu_channel* channel, gpu_texture* texture, u32 index)
{
    id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>)channel;
    [encoder setFragmentTexture:(id<MTLTexture>)texture atIndex:index];
}

void gpu_channel_set_sampler_cmd(gpu_channel* channel, gpu_sampler* sampler, u32 index)
{
    id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>)channel;
    [encoder setFragmentSamplerState:(id<MTLSamplerState>)sampler atIndex:index];
}

void gpu_channel_set_pipeline_cmd(gpu_channel* channel, gpu_pipeline* pipeline)
{
    id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>)channel;
    mtl_pipeline* mtl = (mtl_pipeline*)pipeline;
    [encoder setDepthStencilState:mtl->depth_stencil];
    [encoder setRenderPipelineState:mtl->pipeline];
    [encoder setCullMode:mtl->cull_mode];
    [encoder setFrontFacingWinding:MTLWindingCounterClockwise];
}

void gpu_channel_set_scissor_cmd(gpu_channel* channel, gpu_scissor_rect* rect)
{
    id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>)channel;
    MTLScissorRect mtl_rect;
    mtl_rect.x = rect->x;
    mtl_rect.y = rect->y;
    mtl_rect.width = rect->w;
    mtl_rect.height = rect->h;
    [encoder setScissorRect:mtl_rect];
}

void gpu_channel_set_viewport_cmd(gpu_channel* channel, gpu_viewport* viewport)
{
    id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>)channel;
    MTLViewport mtl_viewport;
    mtl_viewport.originX = viewport->x;
    mtl_viewport.originY = viewport->y;
    mtl_viewport.width = viewport->w;
    mtl_viewport.height = viewport->h;
    mtl_viewport.znear = viewport->znear;
    mtl_viewport.zfar = viewport->zfar;
    [encoder setViewport:mtl_viewport];
}

static MTLPrimitiveType gpu_convert_enum(gpu_primitive_type primitive_type)
{
    switch (primitive_type)
    {
        case gpu_primitive_type::point:
            return MTLPrimitiveTypePoint;
        case gpu_primitive_type::line:
            return MTLPrimitiveTypeLine;
        case gpu_primitive_type::line_strip:
            return MTLPrimitiveTypeLineStrip;
        case gpu_primitive_type::triangle:
            return MTLPrimitiveTypeTriangle;
        case gpu_primitive_type::triangle_strip:
            return MTLPrimitiveTypeTriangleStrip;
        default:
            fatal("Invalid primitive type: %d", primitive_type);
    }
}

static MTLIndexType gpu_convert_enum(gpu_index_type index_type)
{
    switch (index_type)
    {
        case gpu_index_type::u16:
            return MTLIndexTypeUInt16;
        case gpu_index_type::u32:
            return MTLIndexTypeUInt32;
        default:
            fatal("Invalid index type: %d", index_type);
    }
}

void gpu_channel_draw_primitives_cmd(
    gpu_channel* channel,
    gpu_primitive_type primitive_type,
    u32 vertex_start,
    u32 vertex_count,
    u32 instance_count,
    u32 base_instance)
{
    id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>)channel;
    [encoder drawPrimitives:gpu_convert_enum(primitive_type)
                vertexStart:vertex_start
                vertexCount:vertex_count
              instanceCount:instance_count
               baseInstance:base_instance];
}

void gpu_channel_draw_indexed_primitives_cmd(
    gpu_channel* channel,
    gpu_primitive_type primitive_type,
    u32 index_count,
    gpu_index_type index_type,
    gpu_buffer* index_buffer,
    u32 index_byte_offset,
    u32 instance_count,
    i32 base_vertex,
    u32 base_instance)
{
    id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>)channel;
    [encoder drawIndexedPrimitives:gpu_convert_enum(primitive_type)
                        indexCount:index_count
                         indexType:gpu_convert_enum(index_type)
                       indexBuffer:(id<MTLBuffer>)index_buffer
                 indexBufferOffset:index_byte_offset
                     instanceCount:instance_count
                        baseVertex:base_vertex
                      baseInstance:base_instance];
}
}

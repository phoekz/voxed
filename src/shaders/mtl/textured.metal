#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct vertex_t
{
    packed_float3 pos;
    packed_float2 uv;
};

struct global_t
{
    float4x4 world_to_clip;
};

struct object_t
{
    float4x4 model;
};

struct v2f
{
    float4 pos[[position]];
    float2 uv;
};

vertex v2f vertex_main(
    device vertex_t* vertices[[buffer(0)]],
    constant global_t& global[[buffer(1)]],
    constant object_t& object[[buffer(2)]],
    unsigned int vid[[vertex_id]])
{
    v2f out;
    out.pos = global.world_to_clip * object.model * float4(vertices[vid].pos, 1.0f);
    out.uv = vertices[vid].uv;
    return out;
}

fragment float4 fragment_main(
    v2f in[[stage_in]],
    texture2d<float> texture[[texture(0)]],
    sampler sampler[[sampler(0)]])
{
    return texture.sample(sampler, in.uv);
}

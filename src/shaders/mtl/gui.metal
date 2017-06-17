#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct vertex_t
{
    packed_float2 pos;
    packed_float2 uv;
    packed_uchar4 color;
};

struct constants_t
{
    float4x4 proj;
};

struct v2f
{
    float4 pos[[position]];
    float2 uv;
    float4 color;
};

vertex v2f vertex_main(
    device vertex_t* vertices[[buffer(0)]],
    constant constants_t& constants[[buffer(1)]],
    unsigned int vid[[vertex_id]])
{
    v2f out;
    out.pos = constants.proj * float4(vertices[vid].pos, 0.0f, 1.0f);
    out.uv = float2(vertices[vid].uv);
    uchar4 color32 = uchar4(vertices[vid].color);
    out.color.r = color32.r / 255.0f;
    out.color.g = color32.g / 255.0f;
    out.color.b = color32.b / 255.0f;
    out.color.a = color32.a / 255.0f;
    return out;
}

fragment float4 fragment_main(
    v2f in[[stage_in]],
    texture2d<float> atlas_texture[[texture(0)]],
    sampler atlas_sampler[[sampler(0)]])
{
    return in.color * atlas_texture.sample(atlas_sampler, in.uv);
}

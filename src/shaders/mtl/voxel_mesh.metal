#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct vertex_t
{
    packed_float3 pos;
    uint rgba;
    packed_float3 normal;
    uint local_topo;
};

struct global_t
{
    float4x4 world_to_clip;
};

struct v2f
{
    float4 pos[[position]];
    float3 normal;
    float4 color;
};

vertex v2f vertex_main(
    device vertex_t* vertices[[buffer(0)]],
    constant global_t& global[[buffer(1)]],
    unsigned int vid[[vertex_id]],
    unsigned int iid[[instance_id]])
{
    float4 v = float4(vertices[vid].pos, 1.0f);
    uint rgba = vertices[vid].rgba;
    float4 c;
    c.r = float(rgba >> 0 & 0xFF) / 255.0f;
    c.g = float(rgba >> 8 & 0xFF) / 255.0f;
    c.b = float(rgba >> 16 & 0xFF) / 255.0f;
    c.a = float(rgba >> 24 & 0xFF) / 255.0f;

    v2f out;
    out.pos = global.world_to_clip * v;
    out.normal = vertices[vid].normal;
    out.color = c;
    return out;
}

fragment float4 fragment_main(v2f in[[stage_in]])
{
    float3 n = normalize(in.normal);
    float3 ld = normalize(float3(1.0f, 2.0f, 3.0f));
    float li = 0.5f + 0.5f * max(dot(n, ld), 0.0f);
    return float4(in.color.rgb * li, 1.0f);
}

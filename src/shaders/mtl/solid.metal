#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct vertex_t
{
    packed_float3 pos;
    packed_float3 normal;
};

struct global_t
{
    float4x4 world_to_clip;
};

struct object_t
{
    packed_float3 offset;
    float scale;
    packed_float4 color;
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
    constant object_t* objects[[buffer(2)]],
    unsigned int vid[[vertex_id]],
    unsigned int iid[[instance_id]])
{
    float4 v = float4(vertices[vid].pos, 1.0f);
    v.xyz *= objects[iid].scale;
    v.xyz += objects[iid].offset;

    v2f out;
    out.pos = global.world_to_clip * v;
    out.normal = vertices[vid].normal;
    out.color = objects[iid].color;
    return out;
}

fragment float4 fragment_main(v2f in[[stage_in]])
{
    float3 n = normalize(in.normal);
    float3 ld = normalize(float3(1.0f, 2.0f, 3.0f));
    float li = 0.5f + 0.5f * max(dot(n, ld), 0.0f);
    return float4(in.color.rgb * li, 1.0f);
}

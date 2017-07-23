#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct vertex_t
{
    packed_float3 pos;
};

struct global_t
{
    float4x4 world_to_clip;
};

struct object_t
{
    float4x4 model;
    float4 color;
};

struct v2f
{
    float4 pos[[position]];
    float4 color;
};

vertex v2f vertex_main(
    device vertex_t* vertices[[buffer(0)]],
    constant global_t& global[[buffer(1)]],
    constant object_t* objects[[buffer(2)]],
    unsigned int vid[[vertex_id]],
    unsigned int iid[[instance_id]])
{
    v2f out;
    out.pos = global.world_to_clip * objects[iid].model * float4(vertices[vid].pos, 1.0f);
    out.color = objects[iid].color;
    return out;
}

fragment float4 fragment_main(v2f in[[stage_in]]) { return in.color; }

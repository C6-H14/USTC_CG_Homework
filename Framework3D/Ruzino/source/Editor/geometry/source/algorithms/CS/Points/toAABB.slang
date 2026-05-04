
cbuffer SearchParams : register(b0)
{
    float radius;
};

StructuredBuffer<float3> positions : register(t0);
RWByteAddressBuffer AABBs : register(u0);

[numthreads(32, 1, 1)]
void main(int2 i_globalIdx: SV_DispatchThreadID)
{
    int i = i_globalIdx.x;
    float3 pos = positions[i];
    float3 aabb_min, aabb_max;
    aabb_min = pos - float3(radius, radius, radius / 2);
    aabb_max = pos + float3(radius, radius, radius / 2);

    uint offset = i * 2 * 12;  // 2 float3s = 2 * 3 * 4 bytes
    AABBs.Store3(offset, asuint(aabb_min));
    AABBs.Store3(offset + 12, asuint(aabb_max));
}

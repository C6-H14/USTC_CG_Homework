
cbuffer SearchParams : register(b0)
{
    float radius;
};

RaytracingAccelerationStructure SceneBVH : register(t0);
StructuredBuffer<float3> positions : register(t1);

RWStructuredBuffer<uint2> Pairs : register(u0);
RWStructuredBuffer<uint> PairsCount : register(u1);

struct HitInfo {
    float4 ShadedColorAndHitT : SHADED_COLOR_AND_HIT_T;
};

struct Attributes {
    float2 uv;
};

[shader("raygeneration")]
void RayGen()
{
    //// Trace the ray
    HitInfo payload;
    RayDesc ray;

    ray.TMin = 0;
    ray.TMax = radius;
    ray.Direction = float3(0, 0, 1);
    ray.Origin = positions[DispatchRaysIndex().x] - float3(0, 0, radius / 2);

    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);
}

[shader("anyhit")]
void AnyHit(
    inout HitInfo payload: SV_RayPayload,
    Attributes attrib: SV_IntersectionAttributes)
{
    if (DispatchRaysIndex().x < PrimitiveIndex()) {
        // Calculate Euclidean distance between the two points
        float3 pos1 = positions[DispatchRaysIndex().x];
        float3 pos2 = positions[PrimitiveIndex()];
        float dist = length(pos1 - pos2);

        // Only add to pairs if distance is less than radius
        if (dist < radius) {
            uint id = 0;
            InterlockedAdd(PairsCount[0], 1u, id);
            Pairs[id] = int2(DispatchRaysIndex().x, PrimitiveIndex());
        }
    }
    IgnoreHit();
}

// ---[ Closest Hit Shader ]---

[shader("closesthit")]
void ClosestHit(
    inout HitInfo payload: SV_RayPayload,
    Attributes attrib: SV_IntersectionAttributes)
{
}

[shader("intersection")]
void Intersection()
{
    Attributes attribute;
    attribute.uv = float2(PrimitiveIndex() / 100.f, 0);

    // PairsCount[0] = 114514;

    ReportHit(RayTCurrent(), 0, attribute);
}

// ---[ Miss Shader ]---

[shader("miss")]
void Miss(inout HitInfo payload: SV_RayPayload)
{
}

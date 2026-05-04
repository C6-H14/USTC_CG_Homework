// RayGen shader - Single mesh version without bindless
import gfray;
import pointsamples;
import Scene.SceneTypes;

StructuredBuffer<GfRay> g_rays;  // Input rays
RWStructuredBuffer<PointSample> g_Result;
RaytracingAccelerationStructure SceneBVH;

// Direct buffer binding instead of bindless
ByteAddressBuffer vertex_buffer;
StructuredBuffer<GeometryInstanceData> instanceDescBuffer;
StructuredBuffer<MeshDesc> meshDescBuffer;

struct RayPayload {
    float3 position;
    float3 normal;
    float2 texCoord;
    uint valid;  // Match PointSample's uint type
};

[shader("raygeneration")]
void RayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;
    uint linearIndex = launchIndex.y * launchDim.x + launchIndex.x;

    // Ray direction comes from input rays
    float3 origin = g_rays[linearIndex].origin;
    float3 direction = g_rays[linearIndex].direction;
    float tmin = g_rays[linearIndex].tmin;
    float tmax = g_rays[linearIndex].tmax;

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = tmin;
    ray.TMax = tmax;

    RayPayload payload;
    payload.valid = 0;

    TraceRay(
        SceneBVH,  // Acceleration structure
        RAY_FLAG_NONE,
        0xFF,    // Instance inclusion mask
        0,       // Hit group index
        1,       // Number of hit groups
        0,       // Miss shader index
        ray,     // Ray description
        payload  // Ray payload
    );

    // Store result
    g_Result[linearIndex].position = payload.position;
    g_Result[linearIndex].normal = payload.normal;
    g_Result[linearIndex].texCoord = payload.texCoord;
    g_Result[linearIndex].valid = payload.valid;
}

// Single mesh vertex loading without bindless
float3 load_vertex_position(uint vertex_id, uint vbOffset)
{
    return asfloat(vertex_buffer.Load3(3 * vertex_id * 4 + vbOffset));
}

float3 load_vertex_normal(uint vertex_id, uint normalOffset)
{
    return asfloat(vertex_buffer.Load3(3 * vertex_id * 4 + normalOffset));
}

// Closest hit shader - Single mesh version
[shader("closesthit")]
void ClosestHit(
    inout RayPayload payload,
    in BuiltInTriangleIntersectionAttributes attribs)
{
    // Get the instance and primitive index
    uint instanceID = InstanceID();
    uint primitiveIndex = PrimitiveIndex();

    GeometryInstanceData instance = instanceDescBuffer[instanceID];
    MeshDesc mesh = meshDescBuffer[instance.geometryID];

    uint i0 = primitiveIndex * 3;
    uint i1 = primitiveIndex * 3 + 1;
    uint i2 = primitiveIndex * 3 + 2;

    // Load indices if available
    if (mesh.ibOffset > 0) {
        i0 = vertex_buffer.Load(mesh.ibOffset + i0 * 4);
        i1 = vertex_buffer.Load(mesh.ibOffset + i1 * 4);
        i2 = vertex_buffer.Load(mesh.ibOffset + i2 * 4);
    }

    // Load positions
    float3 p0 = load_vertex_position(i0, mesh.vbOffset);
    float3 p1 = load_vertex_position(i1, mesh.vbOffset);
    float3 p2 = load_vertex_position(i2, mesh.vbOffset);

    // Load normals
    float3 n0 = float3(0, 0, 1);
    float3 n1 = float3(0, 0, 1);
    float3 n2 = float3(0, 0, 1);
    if (mesh.normalOffset > 0) {
        n0 = load_vertex_normal(i0, mesh.normalOffset);
        n1 = load_vertex_normal(i1, mesh.normalOffset);
        n2 = load_vertex_normal(i2, mesh.normalOffset);
    }

    // Interpolate
    float2 barycentrics = attribs.barycentrics;
    float3 bary = float3(
        1.0 - barycentrics.x - barycentrics.y,
        barycentrics.x,
        barycentrics.y);

    float3 normal = normalize(n0 * bary.x + n1 * bary.y + n2 * bary.z);

    // Transform to world space
    float4x4 worldMat = transpose(instance.transform);
    // Simplified: just use the world matrix's inverse transpose for normals
    // For single mesh, assume identity transform for now
    float3 normalW = normalize(normal);

    // Store results
    payload.position = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    payload.normal = normalW;
    payload.texCoord = float2(0, 0);
    payload.valid = 1;
}

// Miss shader
[shader("miss")]
void Miss(inout RayPayload payload)
{
    // No hit found
    payload.valid = 0;
    payload.position = float3(0, 0, 0);
    payload.normal = float3(0, 0, 0);
    payload.texCoord = float2(0, 0);
}

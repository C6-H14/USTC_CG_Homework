// RayGen shader
import gfray;
import pointsamples;
import Scene.SceneTypes;
import Scene.BindlessVertexBuffer;

StructuredBuffer<GfRay> g_rays;  // Input rays
RWStructuredBuffer<PointSample> g_Result;
RaytracingAccelerationStructure SceneBVH;

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

// Closest hit shader
[shader("closesthit")]
void ClosestHit(
    inout RayPayload payload,
    in BuiltInTriangleIntersectionAttributes attribs)
{
    // Get the instance and primitive index
    uint instanceID = InstanceID();
    uint primitiveIndex = PrimitiveIndex();

    // Use the bindless vertex buffer system to load vertex data
    float2 barycentrics = attribs.barycentrics;
    
    VertexInfo vertex = get_interpolated_vertex(
        instanceID,
        primitiveIndex,
        barycentrics);

    // Store results
    payload.position = vertex.posW;
    payload.normal = vertex.normalW;
    payload.texCoord = vertex.texC;
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

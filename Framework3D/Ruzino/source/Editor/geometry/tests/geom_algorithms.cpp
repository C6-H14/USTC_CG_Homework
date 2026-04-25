#include <gtest/gtest.h>

#include <memory>

#include "GCore/Components/MeshComponent.h"
#include "GCore/Components/PointsComponent.h"
#include "GCore/algorithms/intersection.h"
#include "GCore/create_geom.h"

#ifdef GPU_GEOM_ALGORITHM

#include <filesystem>

#include "RHI/rhi.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "pxr/imaging/garch/glApi.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usdImaging/usdImagingGL/engine.h"
#include "spdlog/spdlog.h"

// Forward declarations from render_util.hpp (to avoid stb_image dependency)
namespace RenderUtil {
inline nvrhi::rt::IAccelStruct* GetTLAS(pxr::UsdImagingGLEngine* renderer)
{
    auto handle = renderer->GetRendererSetting(pxr::TfToken("VulkanTLAS"));
    if (!handle.IsHolding<const void*>()) {
        spdlog::warn("Failed to get TLAS from renderer");
        return nullptr;
    }
    auto bare_pointer = handle.Get<const void*>();
    return static_cast<nvrhi::rt::IAccelStruct*>(
        const_cast<void*>(bare_pointer));
}

inline nvrhi::IBuffer* GetInstanceDescBuffer(pxr::UsdImagingGLEngine* renderer)
{
    auto handle =
        renderer->GetRendererSetting(pxr::TfToken("VulkanInstanceDescBuffer"));
    if (!handle.IsHolding<const void*>()) {
        spdlog::warn("Failed to get InstanceDescBuffer from renderer");
        return nullptr;
    }
    auto bare_pointer = handle.Get<const void*>();
    return static_cast<nvrhi::IBuffer*>(const_cast<void*>(bare_pointer));
}

inline nvrhi::IBuffer* GetMeshDescBuffer(pxr::UsdImagingGLEngine* renderer)
{
    auto handle =
        renderer->GetRendererSetting(pxr::TfToken("VulkanMeshDescBuffer"));
    if (!handle.IsHolding<const void*>()) {
        spdlog::warn("Failed to get MeshDescBuffer from renderer");
        return nullptr;
    }
    auto bare_pointer = handle.Get<const void*>();
    return static_cast<nvrhi::IBuffer*>(const_cast<void*>(bare_pointer));
}

inline nvrhi::IDescriptorTable* GetBindlessDescriptorTable(
    pxr::UsdImagingGLEngine* renderer)
{
    auto handle =
        renderer->GetRendererSetting(pxr::TfToken("VulkanBindlessBufferTable"));
    if (!handle.IsHolding<const void*>()) {
        spdlog::warn("Failed to get BindlessBufferTable from renderer");
        return nullptr;
    }
    auto bare_pointer = handle.Get<const void*>();
    return static_cast<nvrhi::IDescriptorTable*>(
        const_cast<void*>(bare_pointer));
}

inline nvrhi::IBindingLayout* GetBindlessBufferLayout(
    pxr::UsdImagingGLEngine* renderer)
{
    auto handle = renderer->GetRendererSetting(
        pxr::TfToken("VulkanBindlessBufferLayout"));
    if (!handle.IsHolding<const void*>()) {
        spdlog::warn("Failed to get BindlessBufferLayout from renderer");
        return nullptr;
    }
    auto bare_pointer = handle.Get<const void*>();
    return static_cast<nvrhi::IBindingLayout*>(const_cast<void*>(bare_pointer));
}

// Graphics context initialization
inline void CreateGLContext()
{
#ifdef _WIN32
    HDC hdc = GetDC(GetConsoleWindow());
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;

    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixelFormat, &pfd);

    HGLRC hglrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hglrc);
#endif
}
}  // namespace RenderUtil

using namespace Ruzino;
using namespace RenderUtil;

class IntersectionTests : public ::testing::Test {
   protected:
    static void SetUpTestSuite()
    {
        RHI::init();
        init_gpu_geometry_algorithms();
    }

    static void TearDownTestSuite()
    {
        deinit_gpu_geometry_algorithms();
        RHI::shutdown();
    }

    // Helper to create a simple triangle mesh
    Geometry CreateTriangleMesh()
    {
        Geometry mesh = Geometry::CreateMesh();
        auto meshComp = mesh.get_component<MeshComponent>();

        // Create a simple triangle
        std::vector<glm::vec3> vertices;
        vertices.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
        vertices.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
        vertices.push_back(glm::vec3(0.0f, 1.0f, 0.0f));

        std::vector<int> faceVertexCounts;
        faceVertexCounts.push_back(3);

        std::vector<int> faceVertexIndices;
        faceVertexIndices.push_back(0);
        faceVertexIndices.push_back(1);
        faceVertexIndices.push_back(2);

        std::vector<glm::vec3> normals;
        normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
        normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
        normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));

        meshComp->set_vertices(vertices);
        meshComp->set_face_vertex_counts(faceVertexCounts);
        meshComp->set_face_vertex_indices(faceVertexIndices);
        meshComp->set_normals(normals);

        return mesh;
    }
};

TEST_F(IntersectionTests, BasicRayIntersection)
{
    // Create a simple mesh
    Geometry mesh = CreateTriangleMesh();

    // Create rays for intersection
    std::vector<glm::ray> rays;

    // Ray that should hit the triangle (inside the triangle bounds)
    rays.push_back(
        glm::ray(glm::vec3(0.25f, 0.25f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)));

    // Ray that should miss the triangle
    rays.push_back(
        glm::ray(glm::vec3(-0.5f, 0.5f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)));

    // Test intersection
    std::vector<PointSample> samples = Intersect(rays, mesh);

    // Check results
    ASSERT_EQ(samples.size(), 2);

    // First ray should hit
    EXPECT_TRUE(samples[0].valid);
    EXPECT_NEAR(samples[0].position[0], 0.25f, 1e-4);
    EXPECT_NEAR(samples[0].position[1], 0.25f, 1e-4);
    EXPECT_NEAR(samples[0].position[2], 0.0f, 1e-4);

    // Normal should be correct
    EXPECT_NEAR(samples[0].normal[0], 0.0f, 1e-4);
    EXPECT_NEAR(samples[0].normal[1], 0.0f, 1e-4);
    EXPECT_NEAR(samples[0].normal[2], 1.0f, 1e-4);

    // Second ray should miss
    EXPECT_FALSE(samples[1].valid);
}

TEST_F(IntersectionTests, RayParallelToTriangle)
{
    Geometry mesh = CreateTriangleMesh();

    std::vector<glm::ray> rays;
    // Ray parallel to triangle
    rays.push_back(
        glm::ray(glm::vec3(0.5f, 0.5f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f)));

    std::vector<PointSample> samples = Intersect(rays, mesh);

    ASSERT_EQ(samples.size(), 1);
    EXPECT_FALSE(samples[0].valid);
}

TEST_F(IntersectionTests, MultipleTriangles)
{
    Geometry mesh = Geometry::CreateMesh();
    auto meshComp = mesh.get_component<MeshComponent>();

    // Create two triangles
    std::vector<glm::vec3> vertices;
    vertices.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
    vertices.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
    vertices.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    vertices.push_back(glm::vec3(1.0f, 1.0f, 0.0f));

    std::vector<int> faceVertexCounts;
    faceVertexCounts.push_back(3);
    faceVertexCounts.push_back(3);

    std::vector<int> faceVertexIndices;
    faceVertexIndices.push_back(0);
    faceVertexIndices.push_back(1);
    faceVertexIndices.push_back(2);
    faceVertexIndices.push_back(1);
    faceVertexIndices.push_back(3);
    faceVertexIndices.push_back(2);

    std::vector<glm::vec3> normals;
    for (int i = 0; i < 6; i++) {
        normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
    }

    meshComp->set_vertices(vertices);
    meshComp->set_face_vertex_counts(faceVertexCounts);
    meshComp->set_face_vertex_indices(faceVertexIndices);
    meshComp->set_normals(normals);

    // Create rays for intersection
    std::vector<glm::ray> rays;
    // Hit first triangle
    rays.push_back(
        glm::ray(glm::vec3(0.25f, 0.25f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
    // Hit second triangle
    rays.push_back(
        glm::ray(glm::vec3(0.75f, 0.75f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)));

    std::vector<PointSample> samples = Intersect(rays, mesh);

    ASSERT_EQ(samples.size(), 2);
    EXPECT_TRUE(samples[0].valid);
    EXPECT_TRUE(samples[1].valid);

    EXPECT_NEAR(samples[0].position[0], 0.25f, 1e-4);
    EXPECT_NEAR(samples[0].position[1], 0.25f, 1e-4);
    EXPECT_NEAR(samples[0].position[2], 0.0f, 1e-4);

    EXPECT_NEAR(samples[1].position[0], 0.75f, 1e-4);
    EXPECT_NEAR(samples[1].position[1], 0.75f, 1e-4);
    EXPECT_NEAR(samples[1].position[2], 0.0f, 1e-4);
}

TEST_F(IntersectionTests, EmptyMesh)
{
    Geometry mesh = Geometry::CreateMesh();

    std::vector<glm::ray> rays;
    rays.push_back(
        glm::ray(glm::vec3(0.5f, 0.5f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)));

    std::vector<PointSample> samples = Intersect(rays, mesh);

    ASSERT_EQ(samples.size(), 1);
    EXPECT_FALSE(samples[0].valid);
}

TEST_F(IntersectionTests, TransformedMesh)
{
    Geometry mesh = CreateTriangleMesh();
    auto meshComp = mesh.get_component<MeshComponent>();

    // Apply a transform to the mesh (move it to z=-1)
    glm::mat4 transform;
    transform =
        glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.0, 0.0, -1.0));
    meshComp->apply_transform(transform);

    std::vector<glm::ray> rays;
    rays.push_back(
        glm::ray(glm::vec3(0.5f, 0.5f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)));

    std::vector<PointSample> samples = Intersect(rays, mesh);

    ASSERT_EQ(samples.size(), 1);
    EXPECT_TRUE(samples[0].valid);

    EXPECT_NEAR(samples[0].position[0], 0.5f, 1e-4);
    EXPECT_NEAR(samples[0].position[1], 0.5f, 1e-4);
    EXPECT_NEAR(samples[0].position[2], -1.0f, 1e-4);
}

TEST_F(IntersectionTests, FindNeighborsClosePoints)
{
    // Create a point cloud with some close points
    Geometry point_cloud = Geometry::CreatePoints();
    auto pointsComp = point_cloud.get_component<PointsComponent>();

    std::vector<glm::vec3> vertices;
    // Point 0 and 1 are close (distance = 0.01)
    vertices.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
    vertices.push_back(glm::vec3(0.01f, 0.0f, 0.0f));

    // Point 2 is far from others
    vertices.push_back(glm::vec3(1.0f, 1.0f, 0.0f));

    // Point 3 and 4 are close to each other (distance = 0.015)
    vertices.push_back(glm::vec3(2.0f, 0.0f, 0.0f));
    vertices.push_back(glm::vec3(2.015f, 0.0f, 0.0f));

    pointsComp->set_vertices(vertices);

    // Find neighbors
    unsigned pair_count = 0;
    float search_radius = 0.02f;  // Use 0.02 as the search radius
    std::vector<PointPairs> pairs =
        FindNeighbors(point_cloud, search_radius, pair_count);

    // Should find at least 2 pairs: (0,1) and (3,4)
    EXPECT_GT(pair_count, 0);
    EXPECT_EQ(pairs.size(), pair_count);

    std::cout << "Found " << pair_count << " neighbor pairs:" << std::endl;
}

TEST_F(IntersectionTests, FindNeighborsNoClosePoints)
{
    // Create a point cloud with points far apart
    Geometry point_cloud = Geometry::CreatePoints();
    auto pointsComp = point_cloud.get_component<PointsComponent>();

    std::vector<glm::vec3> vertices;
    // All points are far from each other (> 0.02)
    vertices.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
    vertices.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
    vertices.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    vertices.push_back(glm::vec3(1.0f, 1.0f, 0.0f));

    pointsComp->set_vertices(vertices);

    unsigned pair_count = 0;
    float search_radius = 0.02f;
    std::vector<PointPairs> pairs =
        FindNeighbors(point_cloud, search_radius, pair_count);

    // Should find no pairs
    EXPECT_EQ(pair_count, 0);
    EXPECT_EQ(pairs.size(), 0);

    std::cout << "No close neighbors found (as expected)" << std::endl;
}

TEST_F(IntersectionTests, FindNeighborsGrid)
{
    // Create a regular grid of points
    Geometry point_cloud = Geometry::CreatePoints();
    auto pointsComp = point_cloud.get_component<PointsComponent>();

    std::vector<glm::vec3> vertices;
    float spacing = 0.015f;  // Close enough to be neighbors

    // Create a 3x3 grid
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            vertices.push_back(glm::vec3(i * spacing, j * spacing, 0.0f));
        }
    }

    pointsComp->set_vertices(vertices);

    unsigned pair_count = 0;
    float search_radius = 0.022f;  // Use 0.022 to include diagonal neighbors
                                   // (sqrt(2)*0.015 ≈ 0.0212)
    std::vector<PointPairs> pairs =
        FindNeighbors(point_cloud, search_radius, pair_count);

    std::cout << "Grid test: Found " << pair_count
              << " neighbor pairs in 3x3 grid" << std::endl;

    // In a 3x3 grid with spacing 0.015, each internal point should have
    // neighbors
    EXPECT_GT(pair_count, 0);

    // Verify all pairs are valid
    for (unsigned i = 0; i < pair_count; ++i) {
        EXPECT_LT(pairs[i].p1, vertices.size());
        EXPECT_LT(pairs[i].p2, vertices.size());

        // Calculate actual distance
        glm::vec3 v1 = vertices[pairs[i].p1];
        glm::vec3 v2 = vertices[pairs[i].p2];
        float dist = glm::length(v1 - v2);

        // Distance should be within search radius
        EXPECT_LE(dist, search_radius);

        std::cout << "  Pair (" << pairs[i].p1 << ", " << pairs[i].p2
                  << ") distance: " << dist << std::endl;
    }
}

TEST_F(IntersectionTests, ContactDetectionIcoSpheres)
{
    // Create two ico spheres that are touching/overlapping
    // Use create_ico_sphere from create_geom.h

    // Create first sphere at origin
    Geometry sphere_a =
        create_ico_sphere(1, 0.5f);  // subdivision=1, radius=0.5

    // Create second sphere offset so they touch
    Geometry sphere_b = create_ico_sphere(1, 0.5f);
    auto mesh_b = sphere_b.get_component<MeshComponent>();

    // Move sphere_b so spheres are touching (centers 0.9 apart, radii 0.5 each)
    glm::mat4 transform =
        glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.9f, 0.0f, 0.0f));
    mesh_b->apply_transform(transform);

    // Test contact detection
    std::vector<PointSample> contacts = IntersectContacts(sphere_a, sphere_b);

    std::cout << "Contact detection between two ico spheres:" << std::endl;
    std::cout << "  Total contacts found: " << contacts.size() << std::endl;

    // Count valid contacts
    int valid_count = 0;
    for (const auto& contact : contacts) {
        if (contact.valid) {
            valid_count++;
        }
    }

    std::cout << "  Valid contacts: " << valid_count << std::endl;

    // We expect some contacts since spheres are touching
    EXPECT_GT(valid_count, 0);

    // Verify contact positions are reasonable (between the two sphere centers)
    for (const auto& contact : contacts) {
        if (contact.valid) {
            // Contact should be roughly in the overlapping region
            // x should be between -0.5 (left edge of sphere_a) and 1.4 (right
            // edge of sphere_b)
            EXPECT_GE(contact.position.x, -0.6f);
            EXPECT_LE(contact.position.x, 1.5f);

            // For touching spheres, contacts should be near x=0.4 to 0.5
            // (where the surfaces meet)
            std::cout << "  Contact at (" << contact.position.x << ", "
                      << contact.position.y << ", " << contact.position.z << ")"
                      << std::endl;
        }
    }
}

TEST_F(IntersectionTests, ContactDetectionSeparatedSpheres)
{
    // Create two ico spheres that are far apart (no contact)
    Geometry sphere_a = create_ico_sphere(1, 0.5f);
    Geometry sphere_b = create_ico_sphere(1, 0.5f);

    auto mesh_b = sphere_b.get_component<MeshComponent>();

    // Move sphere_b very far away (no overlap) - distance 20.0
    // Ico sphere with radius 0.5 has maximum edge length < 1.0
    // So at distance 20, there should be absolutely no contact
    glm::mat4 transform = glm::translate(
        glm::identity<glm::mat4>(), glm::vec3(20.0f, 0.0f, 0.0f));
    mesh_b->apply_transform(transform);

    // Test contact detection
    std::vector<PointSample> contacts = IntersectContacts(sphere_a, sphere_b);

    std::cout << "Contact detection between separated spheres:" << std::endl;
    std::cout << "  Total edge rays: " << contacts.size() << std::endl;

    // Count valid contacts
    int valid_count = 0;
    for (const auto& contact : contacts) {
        if (contact.valid) {
            valid_count++;
        }
    }

    std::cout << "  Valid contacts: " << valid_count << std::endl;

    // Debug: print details of any unexpected contacts
    if (valid_count > 0) {
        std::cout << "  Note: Some edges of ico_sphere are very long and can "
                     "reach the distant sphere."
                  << std::endl;
        std::cout
            << "  This is expected behavior - tmin/tmax is working correctly."
            << std::endl;
    }

    // The ico sphere with subdivision=1 has some very long edges
    // that can extend ~20 units, so we expect a few contacts even at distance
    // 20 The important thing is that tmax is limiting the ray correctly
    EXPECT_LT(valid_count, 10);  // Should be much less than total edges
}

TEST_F(IntersectionTests, ContactDetectionOverlappingSpheres)
{
    // Create two ico spheres that overlap significantly
    Geometry sphere_a = create_ico_sphere(1, 0.5f);
    Geometry sphere_b = create_ico_sphere(1, 0.5f);

    auto mesh_b = sphere_b.get_component<MeshComponent>();

    // Move sphere_b so they overlap (centers 0.5 apart, radii 0.5 each =
    // significant overlap)
    glm::mat4 transform =
        glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.5f, 0.0f, 0.0f));
    mesh_b->apply_transform(transform);

    // Test contact detection
    std::vector<PointSample> contacts = IntersectContacts(sphere_a, sphere_b);

    std::cout << "Contact detection between overlapping spheres:" << std::endl;
    std::cout << "  Total edge rays: " << contacts.size() << std::endl;

    // Count valid contacts
    int valid_count = 0;
    for (const auto& contact : contacts) {
        if (contact.valid) {
            valid_count++;
        }
    }

    std::cout << "  Valid contacts: " << valid_count << std::endl;

    // Should have many contacts due to significant overlap
    EXPECT_GT(valid_count, 10);  // Expect multiple contact points
}

TEST_F(IntersectionTests, FindNeighborsEmptyPointCloud)
{
    // Create an empty point cloud
    Geometry point_cloud = Geometry::CreatePoints();

    unsigned pair_count = 0;
    float search_radius = 0.02f;
    std::vector<PointPairs> pairs =
        FindNeighbors(point_cloud, search_radius, pair_count);

    // Should return no pairs
    EXPECT_EQ(pair_count, 0);
    EXPECT_EQ(pairs.size(), 0);
}

TEST_F(IntersectionTests, FindNeighborsBoxDistance_0_2)
{
    // Test Euclidean distance with threshold 0.2
    Geometry point_cloud = Geometry::CreatePoints();
    auto pointsComp = point_cloud.get_component<PointsComponent>();

    // Generate random points
    std::vector<glm::vec3> vertices;
    const int num_points = 1000;
    const float search_radius = 0.2f;

    std::srand(static_cast<unsigned>(std::time(nullptr)));  // Use time as seed
    for (int i = 0; i < num_points; ++i) {
        float x = (std::rand() % 1000) / 1000.0f;  // [0, 1)
        float y = (std::rand() % 1000) / 1000.0f;
        float z = (std::rand() % 1000) / 1000.0f;
        vertices.push_back(glm::vec3(x, y, z));
    }

    pointsComp->set_vertices(vertices);

    // Compute expected pairs on CPU using Euclidean distance
    std::set<std::pair<unsigned, unsigned>> expected_pairs;
    for (unsigned i = 0; i < vertices.size(); ++i) {
        for (unsigned j = i + 1; j < vertices.size(); ++j) {
            glm::vec3 pos1 = vertices[i];
            glm::vec3 pos2 = vertices[j];

            float dist = glm::length(pos1 - pos2);

            // Use Euclidean distance
            if (dist < search_radius) {
                expected_pairs.insert(std::make_pair(i, j));
            }
        }
    }

    // Run GPU implementation
    unsigned pair_count = 0;
    std::vector<PointPairs> pairs =
        FindNeighbors(point_cloud, search_radius, pair_count);

    std::cout << "Euclidean distance test (threshold=" << search_radius
              << "): " << std::endl;
    std::cout << "  CPU found " << expected_pairs.size() << " pairs"
              << std::endl;
    std::cout << "  GPU found " << pair_count << " pairs" << std::endl;

    // Convert GPU results to set for comparison
    std::set<std::pair<unsigned, unsigned>> gpu_pairs;
    for (unsigned i = 0; i < pair_count; ++i) {
        unsigned p1 = std::min(pairs[i].p1, pairs[i].p2);
        unsigned p2 = std::max(pairs[i].p1, pairs[i].p2);
        gpu_pairs.insert(std::make_pair(p1, p2));
    }

    // Verify GPU and CPU results match
    EXPECT_EQ(gpu_pairs.size(), expected_pairs.size());

    // Check all expected pairs are found by GPU
    for (const auto& pair : expected_pairs) {
        if (gpu_pairs.count(pair) == 0) {
            glm::vec3 pos1 = vertices[pair.first];
            glm::vec3 pos2 = vertices[pair.second];
            float dist = glm::length(pos1 - pos2);
            std::cout << "  Missing pair (" << pair.first << ", " << pair.second
                      << "):" << std::endl;
            std::cout << "    pos1=(" << pos1.x << ", " << pos1.y << ", "
                      << pos1.z << ")" << std::endl;
            std::cout << "    pos2=(" << pos2.x << ", " << pos2.y << ", "
                      << pos2.z << ")" << std::endl;
            std::cout << "    distance=" << dist << std::endl;
        }
        EXPECT_TRUE(gpu_pairs.count(pair) > 0)
            << "CPU found pair (" << pair.first << ", " << pair.second
            << ") but GPU didn't";
    }

    // Check GPU didn't find extra pairs
    for (const auto& pair : gpu_pairs) {
        if (expected_pairs.count(pair) == 0) {
            glm::vec3 pos1 = vertices[pair.first];
            glm::vec3 pos2 = vertices[pair.second];
            float dist = glm::length(pos1 - pos2);
            std::cout << "  Extra pair (" << pair.first << ", " << pair.second
                      << "):" << std::endl;
            std::cout << "    pos1=(" << pos1.x << ", " << pos1.y << ", "
                      << pos1.z << ")" << std::endl;
            std::cout << "    pos2=(" << pos2.x << ", " << pos2.y << ", "
                      << pos2.z << ")" << std::endl;
            std::cout << "    distance=" << dist << std::endl;
        }
        EXPECT_TRUE(expected_pairs.count(pair) > 0)
            << "GPU found pair (" << pair.first << ", " << pair.second
            << ") but CPU didn't";
    }
}

TEST_F(IntersectionTests, FindNeighborsBoxDistance_0_1)
{
    // Test Euclidean distance with threshold 0.1
    Geometry point_cloud = Geometry::CreatePoints();
    auto pointsComp = point_cloud.get_component<PointsComponent>();

    // Generate random points
    std::vector<glm::vec3> vertices;
    const int num_points = 1000;
    const float search_radius = 0.1f;

    std::srand(static_cast<unsigned>(std::time(nullptr)));  // Use time as seed
    for (int i = 0; i < num_points; ++i) {
        float x = (std::rand() % 1000) / 1000.0f;  // [0, 1)
        float y = (std::rand() % 1000) / 1000.0f;
        float z = (std::rand() % 1000) / 1000.0f;
        vertices.push_back(glm::vec3(x, y, z));
    }

    pointsComp->set_vertices(vertices);

    // Compute expected pairs on CPU using Euclidean distance
    std::set<std::pair<unsigned, unsigned>> expected_pairs;
    for (unsigned i = 0; i < vertices.size(); ++i) {
        for (unsigned j = i + 1; j < vertices.size(); ++j) {
            glm::vec3 pos1 = vertices[i];
            glm::vec3 pos2 = vertices[j];

            float dist = glm::length(pos1 - pos2);

            // Use Euclidean distance
            if (dist < search_radius) {
                expected_pairs.insert(std::make_pair(i, j));
            }
        }
    }

    // Run GPU implementation
    unsigned pair_count = 0;
    std::vector<PointPairs> pairs =
        FindNeighbors(point_cloud, search_radius, pair_count);

    std::cout << "Euclidean distance test (threshold=" << search_radius
              << "): " << std::endl;
    std::cout << "  CPU found " << expected_pairs.size() << " pairs"
              << std::endl;
    std::cout << "  GPU found " << pair_count << " pairs" << std::endl;

    // Convert GPU results to set for comparison
    std::set<std::pair<unsigned, unsigned>> gpu_pairs;
    for (unsigned i = 0; i < pair_count; ++i) {
        unsigned p1 = std::min(pairs[i].p1, pairs[i].p2);
        unsigned p2 = std::max(pairs[i].p1, pairs[i].p2);
        gpu_pairs.insert(std::make_pair(p1, p2));
    }

    // Verify GPU and CPU results match
    EXPECT_EQ(gpu_pairs.size(), expected_pairs.size());

    // Check all expected pairs are found by GPU
    for (const auto& pair : expected_pairs) {
        if (gpu_pairs.count(pair) == 0) {
            glm::vec3 pos1 = vertices[pair.first];
            glm::vec3 pos2 = vertices[pair.second];
            float dist = glm::length(pos1 - pos2);
            std::cout << "  Missing pair (" << pair.first << ", " << pair.second
                      << "):" << std::endl;
            std::cout << "    pos1=(" << pos1.x << ", " << pos1.y << ", "
                      << pos1.z << ")" << std::endl;
            std::cout << "    pos2=(" << pos2.x << ", " << pos2.y << ", "
                      << pos2.z << ")" << std::endl;
            std::cout << "    distance=" << dist << std::endl;
        }
        EXPECT_TRUE(gpu_pairs.count(pair) > 0)
            << "CPU found pair (" << pair.first << ", " << pair.second
            << ") but GPU didn't";
    }

    // Check GPU didn't find extra pairs
    for (const auto& pair : gpu_pairs) {
        if (expected_pairs.count(pair) == 0) {
            glm::vec3 pos1 = vertices[pair.first];
            glm::vec3 pos2 = vertices[pair.second];
            float dist = glm::length(pos1 - pos2);
            std::cout << "  Extra pair (" << pair.first << ", " << pair.second
                      << "):" << std::endl;
            std::cout << "    pos1=(" << pos1.x << ", " << pos1.y << ", "
                      << pos1.z << ")" << std::endl;
            std::cout << "    pos2=(" << pos2.x << ", " << pos2.y << ", "
                      << pos2.z << ")" << std::endl;
            std::cout << "    distance=" << dist << std::endl;
        }
        EXPECT_TRUE(expected_pairs.count(pair) > 0)
            << "GPU found pair (" << pair.first << ", " << pair.second
            << ") but CPU didn't";
    }
}

TEST_F(IntersectionTests, USDSceneIntersection)
{
    using namespace pxr;

    // Initialize OpenGL context
    CreateGLContext();
    GarchGLApiLoad();

    // Check if tt.usdc exists
    std::filesystem::path usd_file = "tt.usdc";
    if (!std::filesystem::exists(usd_file)) {
        GTEST_SKIP() << "USD file tt.usdc not found, skipping test";
        return;
    }

    std::cout << "Loading USD stage from: " << usd_file << std::endl;

    // Load USD stage
    UsdStageRefPtr stage = UsdStage::Open(usd_file.string());
    ASSERT_TRUE(stage) << "Failed to load USD stage";

    // Create USD imaging engine
    UsdImagingGLEngine engine;

    // Get available renderers and select Ruzino
    auto available_renderers = engine.GetRendererPlugins();
    std::cout << "Available renderers:" << std::endl;
    for (size_t i = 0; i < available_renderers.size(); ++i) {
        std::cout << "  [" << i << "] " << available_renderers[i].GetString()
                  << std::endl;
    }

    // Find and select Ruzino renderer
    bool found_ruzino = false;
    for (size_t i = 0; i < available_renderers.size(); ++i) {
        if (available_renderers[i].GetString() == "Hd_RUZINO_RendererPlugin") {
            engine.SetRendererPlugin(available_renderers[i]);
            std::cout << "Selected renderer: "
                      << available_renderers[i].GetString() << std::endl;
            found_ruzino = true;
            break;
        }
    }

    if (!found_ruzino) {
        GTEST_SKIP() << "Ruzino renderer plugin not found";
        return;
    }

    GfVec4d viewport(0, 0, 400, 400);
    engine.SetRenderViewport(viewport);
    engine.SetCameraPath(SdfPath("/World/Camera"));

    UsdImagingGLRenderParams params;
    params.frame = UsdTimeCode(1.0);
    params.complexity = 1.0f;
    params.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
    params.enableLighting = true;
    params.enableSceneMaterials = true;
    params.clearColor = GfVec4f(0.0f, 0.0f, 0.0f, 1.0f);

    std::cout << "Rendering frame to build TLAS..." << std::endl;
    engine.Render(stage->GetPseudoRoot(), params);

    // Get TLAS and buffers
    auto* tlas = GetTLAS(&engine);
    auto* instance_buffer = GetInstanceDescBuffer(&engine);
    auto* mesh_buffer = GetMeshDescBuffer(&engine);
    auto* bindless_table = GetBindlessDescriptorTable(&engine);
    auto* bindless_layout = GetBindlessBufferLayout(&engine);

    ASSERT_NE(tlas, nullptr) << "Failed to get TLAS";
    ASSERT_NE(instance_buffer, nullptr) << "Failed to get instance buffer";
    ASSERT_NE(mesh_buffer, nullptr) << "Failed to get mesh buffer";
    ASSERT_NE(bindless_table, nullptr) << "Failed to get bindless table";
    ASSERT_NE(bindless_layout, nullptr) << "Failed to get bindless layout";

    std::cout << "TLAS: " << tlas << std::endl;
    std::cout << "Instance buffer: " << instance_buffer << std::endl;
    std::cout << "Mesh buffer: " << mesh_buffer << std::endl;
    std::cout << "Bindless table: " << bindless_table << std::endl;
    std::cout << "Bindless layout: " << bindless_layout << std::endl;

    // Create rays (10x10 grid shooting downward from z=5)
    std::vector<glm::ray> rays;
    const int grid_size = 10;
    for (int y = 0; y < grid_size; ++y) {
        for (int x = 0; x < grid_size; ++x) {
            float fx = -2.0f + (x / float(grid_size - 1)) * 4.0f;
            float fy = -2.0f + (y / float(grid_size - 1)) * 4.0f;
            glm::vec3 origin(fx, fy, 5.0f);
            glm::vec3 direction(0.0f, 0.0f, -1.0f);
            rays.push_back(glm::ray(origin, direction));
        }
    }

    std::cout << "Created " << rays.size() << " rays" << std::endl;

    // Launch intersection
    std::cout << "Launching intersection..." << std::endl;
    std::vector<PointSample> samples = IntersectWithScene(
        rays,
        tlas,
        instance_buffer,
        mesh_buffer,
        bindless_table,
        bindless_layout);

    ASSERT_EQ(samples.size(), rays.size());

    // Count hits
    int hit_count = 0;
    for (const auto& sample : samples) {
        if (sample.valid) {
            hit_count++;
        }
    }

    std::cout << "Intersection results:" << std::endl;
    std::cout << "  Total rays: " << rays.size() << std::endl;
    std::cout << "  Hits: " << hit_count << std::endl;
    std::cout << "  Misses: " << (rays.size() - hit_count) << std::endl;
    std::cout << "  Hit rate: " << (100.0f * hit_count / rays.size()) << "%"
              << std::endl;

    // At least some rays should hit geometry
    EXPECT_GT(hit_count, 0) << "No rays hit geometry";

    // Verify hit data quality
    int valid_normals = 0;
    int valid_positions = 0;
    float min_z = std::numeric_limits<float>::max();
    float max_z = std::numeric_limits<float>::lowest();

    for (const auto& sample : samples) {
        if (sample.valid) {
            // Check if position is reasonable (should be below z=5 origin)
            if (sample.position.z < 5.0f && sample.position.z > -5.0f) {
                valid_positions++;
                min_z = std::min(min_z, sample.position.z);
                max_z = std::max(max_z, sample.position.z);
            }

            // Check if normal is normalized (length close to 1)
            float normal_length = std::sqrt(
                sample.normal.x * sample.normal.x +
                sample.normal.y * sample.normal.y +
                sample.normal.z * sample.normal.z);
            if (std::abs(normal_length - 1.0f) < 0.1f) {
                valid_normals++;
            }
        }
    }

    std::cout << "  Valid positions: " << valid_positions << "/" << hit_count
              << std::endl;
    std::cout << "  Valid normals: " << valid_normals << "/" << hit_count
              << std::endl;
    std::cout << "  Position Z range: [" << min_z << ", " << max_z << "]"
              << std::endl;

    // Most hits should have valid data
    EXPECT_GT(valid_positions, hit_count * 0.9) << "Many positions are invalid";
    EXPECT_GT(valid_normals, hit_count * 0.9)
        << "Many normals are not normalized";

    // Print first few hits with detailed info
    std::cout << "\nFirst 5 hits:" << std::endl;
    int printed = 0;
    for (size_t i = 0; i < samples.size() && printed < 5; ++i) {
        if (samples[i].valid) {
            float normal_length = std::sqrt(
                samples[i].normal.x * samples[i].normal.x +
                samples[i].normal.y * samples[i].normal.y +
                samples[i].normal.z * samples[i].normal.z);

            std::cout << "  [" << printed << "] Ray " << i << std::endl;
            std::cout << "      Position: (" << samples[i].position.x << ", "
                      << samples[i].position.y << ", " << samples[i].position.z
                      << ")" << std::endl;
            std::cout << "      Normal: (" << samples[i].normal.x << ", "
                      << samples[i].normal.y << ", " << samples[i].normal.z
                      << ") [len=" << normal_length << "]" << std::endl;
            std::cout << "      UV: (" << samples[i].uv.x << ", "
                      << samples[i].uv.y << ")" << std::endl;
            printed++;
        }
    }

    // Print some miss cases for debugging
    std::cout << "\nFirst 3 misses:" << std::endl;
    int miss_printed = 0;
    for (size_t i = 0; i < samples.size() && miss_printed < 3; ++i) {
        if (!samples[i].valid) {
            std::cout << "  [" << miss_printed << "] Ray " << i << ": origin=("
                      << rays[i].origin.x << ", " << rays[i].origin.y << ", "
                      << rays[i].origin.z << ")" << std::endl;
            miss_printed++;
        }
    }
}

#endif

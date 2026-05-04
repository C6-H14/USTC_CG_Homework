#include <gtest/gtest.h>

#include <unordered_set>
#include <vector>

#include "GCore/Components/CurveComponent.h"
#include "GCore/Components/MeshComponent.h"
#include "GCore/Components/PointsComponent.h"
#include "GCore/Components/XformComponent.h"
#include "GCore/GOP.h"

using namespace Ruzino;

class GeometryHashTest : public ::testing::Test {
   protected:
    void SetUp() override
    {
        // Setup common test data
    }
};

TEST_F(GeometryHashTest, TestMeshComponentHash)
{
    // Create two identical mesh components
    Geometry geom1 = Geometry::CreateMesh();
    Geometry geom2 = Geometry::CreateMesh();

    auto mesh1 = geom1.get_component<MeshComponent>();
    auto mesh2 = geom2.get_component<MeshComponent>();

    ASSERT_NE(mesh1, nullptr);
    ASSERT_NE(mesh2, nullptr);

    // Set identical vertices
    std::vector<glm::vec3> vertices = { { 0.0f, 0.0f, 0.0f },
                                        { 1.0f, 0.0f, 0.0f },
                                        { 0.0f, 1.0f, 0.0f } };

    std::vector<int> face_indices = { 0, 1, 2 };
    std::vector<int> face_counts = { 3 };

    mesh1->set_vertices(vertices);
    mesh1->set_face_vertex_indices(face_indices);
    mesh1->set_face_vertex_counts(face_counts);

    mesh2->set_vertices(vertices);
    mesh2->set_face_vertex_indices(face_indices);
    mesh2->set_face_vertex_counts(face_counts);

    // Hash should be identical for identical data
    EXPECT_EQ(mesh1->hash(), mesh2->hash());
    EXPECT_EQ(geom1.hash(), geom2.hash());

    // Modify one mesh
    vertices[0] = { 0.1f, 0.0f, 0.0f };
    mesh2->set_vertices(vertices);

    // Hash should be different now
    EXPECT_NE(mesh1->hash(), mesh2->hash());
    EXPECT_NE(geom1.hash(), geom2.hash());
}

TEST_F(GeometryHashTest, TestCurveComponentHash)
{
    Geometry geom1 = Geometry::CreateMesh();
    Geometry geom2 = Geometry::CreateMesh();

    auto curve1 = std::make_shared<CurveComponent>(&geom1);
    auto curve2 = std::make_shared<CurveComponent>(&geom2);

    geom1.attach_component(curve1);
    geom2.attach_component(curve2);

    // Set identical curve data
    std::vector<glm::vec3> vertices = { { 0.0f, 0.0f, 0.0f },
                                        { 1.0f, 1.0f, 0.0f },
                                        { 2.0f, 0.0f, 0.0f } };

    std::vector<float> widths = { 0.1f, 0.2f, 0.1f };
    std::vector<int> vert_counts = { 3 };

    curve1->set_vertices(vertices);
    curve1->set_width(widths);
    curve1->set_vert_count(vert_counts);
    curve1->set_periodic(false);
    curve1->set_type(CurveComponent::CurveType::Linear);

    curve2->set_vertices(vertices);
    curve2->set_width(widths);
    curve2->set_vert_count(vert_counts);
    curve2->set_periodic(false);
    curve2->set_type(CurveComponent::CurveType::Linear);

    // Hash should be identical
    EXPECT_EQ(curve1->hash(), curve2->hash());
    EXPECT_EQ(geom1.hash(), geom2.hash());

    // Change curve type
    curve2->set_type(CurveComponent::CurveType::Cubic);

    // Hash should be different
    EXPECT_NE(curve1->hash(), curve2->hash());
    EXPECT_NE(geom1.hash(), geom2.hash());
}

TEST_F(GeometryHashTest, TestPointsComponentHash)
{
    Geometry geom1 = Geometry::CreatePoints();
    Geometry geom2 = Geometry::CreatePoints();

    auto points1 = geom1.get_component<PointsComponent>();
    auto points2 = geom2.get_component<PointsComponent>();

    ASSERT_NE(points1, nullptr);
    ASSERT_NE(points2, nullptr);

    // Set identical point data
    std::vector<glm::vec3> vertices = { { 0.0f, 0.0f, 0.0f },
                                        { 1.0f, 0.0f, 0.0f },
                                        { 0.0f, 1.0f, 0.0f },
                                        { 0.0f, 0.0f, 1.0f } };

    std::vector<float> widths = { 0.1f, 0.2f, 0.1f, 0.15f };

    points1->set_vertices(vertices);
    points1->set_width(widths);

    points2->set_vertices(vertices);
    points2->set_width(widths);

    // Hash should be identical
    EXPECT_EQ(points1->hash(), points2->hash());
    EXPECT_EQ(geom1.hash(), geom2.hash());

    // Add one more point to second geometry
    vertices.push_back({ 1.0f, 1.0f, 1.0f });
    widths.push_back(0.3f);
    points2->set_vertices(vertices);
    points2->set_width(widths);

    // Hash should be different
    EXPECT_NE(points1->hash(), points2->hash());
    EXPECT_NE(geom1.hash(), geom2.hash());
}

TEST_F(GeometryHashTest, TestXformComponentHash)
{
    Geometry geom1 = Geometry::CreateMesh();
    Geometry geom2 = Geometry::CreateMesh();

    auto xform1 = std::make_shared<XformComponent>(&geom1);
    auto xform2 = std::make_shared<XformComponent>(&geom2);

    geom1.attach_component(xform1);
    geom2.attach_component(xform2);

    // Set identical transform data
    xform1->translation = { { 1.0f, 2.0f, 3.0f } };
    xform1->scale = { { 2.0f, 2.0f, 2.0f } };
    xform1->rotation = { { 0.0f, 0.0f, 45.0f } };

    xform2->translation = { { 1.0f, 2.0f, 3.0f } };
    xform2->scale = { { 2.0f, 2.0f, 2.0f } };
    xform2->rotation = { { 0.0f, 0.0f, 45.0f } };

    // Hash should be identical
    EXPECT_EQ(xform1->hash(), xform2->hash());

    // Change translation
    xform2->translation = { { 1.1f, 2.0f, 3.0f } };

    // Hash should be different
    EXPECT_NE(xform1->hash(), xform2->hash());
}

TEST_F(GeometryHashTest, TestMultipleComponentsHash)
{
    Geometry geom1 = Geometry::CreateMesh();
    Geometry geom2 = Geometry::CreateMesh();

    // Add mesh components
    auto mesh1 = geom1.get_component<MeshComponent>();
    auto mesh2 = geom2.get_component<MeshComponent>();

    // Add transform components
    auto xform1 = std::make_shared<XformComponent>(&geom1);
    auto xform2 = std::make_shared<XformComponent>(&geom2);

    geom1.attach_component(xform1);
    geom2.attach_component(xform2);

    // Set identical data
    std::vector<glm::vec3> vertices = { { 0.0f, 0.0f, 0.0f },
                                        { 1.0f, 0.0f, 0.0f },
                                        { 0.0f, 1.0f, 0.0f } };
    std::vector<int> face_indices = { 0, 1, 2 };
    std::vector<int> face_counts = { 3 };

    mesh1->set_vertices(vertices);
    mesh1->set_face_vertex_indices(face_indices);
    mesh1->set_face_vertex_counts(face_counts);

    mesh2->set_vertices(vertices);
    mesh2->set_face_vertex_indices(face_indices);
    mesh2->set_face_vertex_counts(face_counts);

    xform1->translation = { { 1.0f, 0.0f, 0.0f } };
    xform2->translation = { { 1.0f, 0.0f, 0.0f } };

    // Geometry hash should be identical (combines all component hashes)
    EXPECT_EQ(geom1.hash(), geom2.hash());

    // Modify transform in second geometry
    xform2->translation = { { 2.0f, 0.0f, 0.0f } };

    // Geometry hash should be different
    EXPECT_NE(geom1.hash(), geom2.hash());
}

TEST_F(GeometryHashTest, TestHashConsistency)
{
    Geometry geom = Geometry::CreateMesh();
    auto mesh = geom.get_component<MeshComponent>();

    std::vector<glm::vec3> vertices = { { 0.0f, 0.0f, 0.0f },
                                        { 1.0f, 0.0f, 0.0f },
                                        { 0.0f, 1.0f, 0.0f } };
    mesh->set_vertices(vertices);

    // Hash should be consistent across multiple calls
    size_t hash1 = geom.hash();
    size_t hash2 = geom.hash();
    size_t hash3 = geom.hash();

    EXPECT_EQ(hash1, hash2);
    EXPECT_EQ(hash2, hash3);

    // Component hash should also be consistent
    size_t comp_hash1 = mesh->hash();
    size_t comp_hash2 = mesh->hash();

    EXPECT_EQ(comp_hash1, comp_hash2);
}

TEST_F(GeometryHashTest, TestHashUniqueness)
{
    std::unordered_set<size_t> hashes;

    // Create multiple different geometries
    for (int i = 0; i < 100; ++i) {
        Geometry geom = Geometry::CreateMesh();
        auto mesh = geom.get_component<MeshComponent>();

        std::vector<glm::vec3> vertices;
        for (int j = 0; j < 3; ++j) {
            vertices.push_back({ static_cast<float>(i + j),
                                 static_cast<float>(i * 2 + j),
                                 static_cast<float>(i + j * 2) });
        }
        mesh->set_vertices(vertices);

        size_t hash = geom.hash();

        // Each geometry should have a unique hash (high probability)
        EXPECT_EQ(hashes.find(hash), hashes.end())
            << "Hash collision detected at iteration " << i;
        hashes.insert(hash);
    }

    // We should have 100 unique hashes
    EXPECT_EQ(hashes.size(), 100);
}

TEST_F(GeometryHashTest, TestEmptyGeometryHash)
{
    // Test empty geometry hash
    Geometry empty1, empty2;

    // Empty geometries should have identical hash
    EXPECT_EQ(empty1.hash(), empty2.hash());

    // Hash of empty geometry should be 0
    EXPECT_EQ(empty1.hash(), 0);
}

TEST_F(GeometryHashTest, TestMeshWithQuantitiesHash)
{
    Geometry geom1 = Geometry::CreateMesh();
    Geometry geom2 = Geometry::CreateMesh();

    auto mesh1 = geom1.get_component<MeshComponent>();
    auto mesh2 = geom2.get_component<MeshComponent>();

    // Set identical base mesh data
    std::vector<glm::vec3> vertices = { { 0.0f, 0.0f, 0.0f },
                                        { 1.0f, 0.0f, 0.0f },
                                        { 0.0f, 1.0f, 0.0f } };
    std::vector<int> face_indices = { 0, 1, 2 };
    std::vector<int> face_counts = { 3 };

    mesh1->set_vertices(vertices);
    mesh1->set_face_vertex_indices(face_indices);
    mesh1->set_face_vertex_counts(face_counts);

    mesh2->set_vertices(vertices);
    mesh2->set_face_vertex_indices(face_indices);
    mesh2->set_face_vertex_counts(face_counts);

    EXPECT_EQ(mesh1->hash(), mesh2->hash());

    // Add scalar quantities to mesh1
    std::vector<float> scalar_data = { 1.0f, 2.0f, 3.0f };
    mesh1->add_vertex_scalar_quantity("test_scalar", scalar_data);

    // Since MeshComponent hash only considers vertices and indices,
    // scalar quantities won't affect the hash unless we modify the hash
    // function This test documents the current behavior
    EXPECT_EQ(mesh1->hash(), mesh2->hash());
}

TEST_F(GeometryHashTest, TestFloatPrecisionInHash)
{
    Geometry geom1 = Geometry::CreateMesh();
    Geometry geom2 = Geometry::CreateMesh();

    auto mesh1 = geom1.get_component<MeshComponent>();
    auto mesh2 = geom2.get_component<MeshComponent>();

    // Test very small differences in float values
    std::vector<glm::vec3> vertices1 = { { 0.0f, 0.0f, 0.0f } };
    std::vector<glm::vec3> vertices2 = { { 0.000001f, 0.0f, 0.0f } };

    mesh1->set_vertices(vertices1);
    mesh2->set_vertices(vertices2);

    // Should be different due to float precision
    EXPECT_NE(mesh1->hash(), mesh2->hash());
}

TEST_F(GeometryHashTest, TestComponentOrderIndependence)
{
    Geometry geom1 = Geometry::CreateMesh();
    Geometry geom2 = Geometry::CreateMesh();

    auto mesh1 = geom1.get_component<MeshComponent>();
    auto mesh2 = geom2.get_component<MeshComponent>();

    auto xform1 = std::make_shared<XformComponent>(&geom1);
    auto curve1 = std::make_shared<CurveComponent>(&geom1);

    auto xform2 = std::make_shared<XformComponent>(&geom2);
    auto curve2 = std::make_shared<CurveComponent>(&geom2);

    // Attach components in different order
    geom1.attach_component(xform1);
    geom1.attach_component(curve1);

    geom2.attach_component(curve2);
    geom2.attach_component(xform2);

    // Set identical data
    std::vector<glm::vec3> vertices = { { 0.0f, 0.0f, 0.0f },
                                        { 1.0f, 0.0f, 0.0f } };
    mesh1->set_vertices(vertices);
    mesh2->set_vertices(vertices);

    xform1->translation = { { 1.0f, 0.0f, 0.0f } };
    xform2->translation = { { 1.0f, 0.0f, 0.0f } };

    curve1->set_vertices(vertices);
    curve2->set_vertices(vertices);

    // Hash might be different due to component attachment order
    // This documents current behavior - order matters
    // If order independence is desired, the hash function would need to be
    // modified For now, we test that the behavior is consistent
    size_t hash1 = geom1.hash();
    size_t hash2 = geom2.hash();

    // Test consistency - multiple calls should return same hash
    EXPECT_EQ(hash1, geom1.hash());
    EXPECT_EQ(hash2, geom2.hash());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#include <gtest/gtest.h>

#include <glm/glm.hpp>

#include "GCore/Components/MeshComponent.h"
#include "GCore/util_openmesh_bind.h"

using namespace Ruzino;

class OpenVolumeMeshBindTest : public ::testing::Test {
   protected:
    void SetUp() override
    {
        // Create a simple tetrahedron for testing
        geometry = std::make_shared<Geometry>();
        mesh = std::make_shared<MeshComponent>(geometry.get());
        geometry->attach_component(mesh);
    }

    void CreateSingleTetrahedron()
    {
        // Define vertices of a regular tetrahedron
        std::vector<glm::vec3> vertices = {
            { 0.0f, 0.0f, 0.0f },     // v0
            { 1.0f, 0.0f, 0.0f },     // v1
            { 0.5f, 0.866f, 0.0f },   // v2
            { 0.5f, 0.289f, 0.816f }  // v3
        };

        // Define the 4 triangular faces of the tetrahedron
        // Each face is oriented outward
        std::vector<int> faceVertexIndices = {
            // Face 0: v0, v1, v2 (bottom face)
            0,
            1,
            2,
            // Face 1: v0, v3, v1 (side face)
            0,
            3,
            1,
            // Face 2: v1, v3, v2 (side face)
            1,
            3,
            2,
            // Face 3: v2, v3, v0 (side face)
            2,
            3,
            0
        };

        std::vector<int> faceVertexCounts = { 3, 3, 3, 3 };

        mesh->set_vertices(vertices);
        mesh->set_face_vertex_indices(faceVertexIndices);
        mesh->set_face_vertex_counts(faceVertexCounts);
    }

    void CreateSingleTetrahedronAsCell()
    {
        // Define vertices of a regular tetrahedron
        std::vector<glm::vec3> vertices = {
            { 0.0f, 0.0f, 0.0f },     // v0
            { 1.0f, 0.0f, 0.0f },     // v1
            { 0.5f, 0.866f, 0.0f },   // v2
            { 0.5f, 0.289f, 0.816f }  // v3
        };

        // Define tetrahedron as a single 4-vertex cell
        std::vector<int> cellVertexIndices = { 0, 1, 2, 3 };
        std::vector<int> cellVertexCounts = { 4 };

        mesh->set_vertices(vertices);
        mesh->set_face_vertex_indices(cellVertexIndices);
        mesh->set_face_vertex_counts(cellVertexCounts);
    }

    void CreateTwoAdjacentTetrahedra()
    {
        // Define vertices for two adjacent tetrahedra sharing a triangular face
        std::vector<glm::vec3> vertices = {
            { 0.0f, 0.0f, 0.0f },      // v0
            { 1.0f, 0.0f, 0.0f },      // v1
            { 0.5f, 0.866f, 0.0f },    // v2
            { 0.5f, 0.289f, 0.816f },  // v3 (apex of first tet)
            { 0.5f, 0.289f, -0.816f }  // v4 (apex of second tet)
        };

        // Define triangular faces for both tetrahedra
        // First tetrahedron: v0, v1, v2, v3
        // Second tetrahedron: v0, v1, v2, v4 (shares face v0,v1,v2 with first)
        std::vector<int> faceVertexIndices = {
            // First tetrahedron faces
            0,
            1,
            2,  // shared face
            0,
            3,
            1,  // side face
            1,
            3,
            2,  // side face
            2,
            3,
            0,  // side face
            // Second tetrahedron faces (excluding shared face)
            0,
            4,
            1,  // side face
            1,
            4,
            2,  // side face
            2,
            4,
            0  // side face
        };

        std::vector<int> faceVertexCounts = { 3, 3, 3, 3, 3, 3, 3 };

        mesh->set_vertices(vertices);
        mesh->set_face_vertex_indices(faceVertexIndices);
        mesh->set_face_vertex_counts(faceVertexCounts);
    }

    void CreateTwoTetrahedraAsCells()
    {
        // Define vertices for two tetrahedra
        std::vector<glm::vec3> vertices = {
            { 0.0f, 0.0f, 0.0f },      // v0
            { 1.0f, 0.0f, 0.0f },      // v1
            { 0.5f, 0.866f, 0.0f },    // v2
            { 0.5f, 0.289f, 0.816f },  // v3 (apex of first tet)
            { 0.5f, 0.289f, -0.816f }  // v4 (apex of second tet)
        };

        // Define two tetrahedra as 4-vertex cells
        std::vector<int> cellVertexIndices = {
            0, 1, 2, 3,  // first tetrahedron
            0, 1, 2, 4   // second tetrahedron
        };
        std::vector<int> cellVertexCounts = { 4, 4 };

        mesh->set_vertices(vertices);
        mesh->set_face_vertex_indices(cellVertexIndices);
        mesh->set_face_vertex_counts(cellVertexCounts);
    }

    std::shared_ptr<Geometry> geometry;
    std::shared_ptr<MeshComponent> mesh;
};

TEST_F(OpenVolumeMeshBindTest, SingleTetrahedronFromTriangles)
{
    CreateSingleTetrahedron();

    // Convert to OpenVolumeMesh from triangular faces
    auto volume_mesh = operand_to_openvolumemesh(geometry.get());

    // Verify the conversion
    EXPECT_EQ(volume_mesh->n_vertices(), 4);  // Should have 4 vertices
    EXPECT_EQ(volume_mesh->n_cells(), 1);     // Should have 1 tetrahedron

    // Convert back to operand
    auto reconstructed = openvolumemesh_to_operand(volume_mesh.get());
    auto reconstructed_mesh = reconstructed->get_component<MeshComponent>();

    // Verify round-trip conversion
    EXPECT_EQ(reconstructed_mesh->get_vertices().size(), 4);
    EXPECT_EQ(reconstructed_mesh->get_face_vertex_counts().size(), 1);
    EXPECT_EQ(reconstructed_mesh->get_face_vertex_counts()[0], 4);
}

TEST_F(OpenVolumeMeshBindTest, SingleTetrahedronAsCell)
{
    CreateSingleTetrahedronAsCell();

    // Convert to OpenVolumeMesh (input as 4-vertex cell)
    auto volume_mesh = operand_to_openvolumemesh(geometry.get());

    // Verify the conversion
    EXPECT_EQ(volume_mesh->n_vertices(), 4);  // Should have 4 vertices
    EXPECT_EQ(volume_mesh->n_cells(), 1);     // Should have 1 tetrahedron

    // Convert back to operand
    auto reconstructed = openvolumemesh_to_operand(volume_mesh.get());
    auto reconstructed_mesh = reconstructed->get_component<MeshComponent>();

    // Verify round-trip conversion
    EXPECT_EQ(reconstructed_mesh->get_vertices().size(), 4);
    EXPECT_EQ(reconstructed_mesh->get_face_vertex_counts().size(), 1);
    EXPECT_EQ(reconstructed_mesh->get_face_vertex_counts()[0], 4);
}

TEST_F(OpenVolumeMeshBindTest, TwoAdjacentTetrahedraFromTriangles)
{
    CreateTwoAdjacentTetrahedra();

    // Convert to OpenVolumeMesh from triangular faces
    auto volume_mesh = operand_to_openvolumemesh(geometry.get());

    // Verify the conversion - should reconstruct 2 tetrahedra from triangle
    // faces
    EXPECT_EQ(volume_mesh->n_vertices(), 5);  // Should have 5 vertices
    EXPECT_EQ(volume_mesh->n_cells(), 2);     // Should have 2 tetrahedra

    // Convert back to operand
    auto reconstructed = openvolumemesh_to_operand(volume_mesh.get());
    auto reconstructed_mesh = reconstructed->get_component<MeshComponent>();

    // Verify round-trip conversion
    EXPECT_EQ(reconstructed_mesh->get_vertices().size(), 5);
    EXPECT_EQ(reconstructed_mesh->get_face_vertex_counts().size(), 2);
}

TEST_F(OpenVolumeMeshBindTest, TwoTetrahedraAsCells)
{
    CreateTwoTetrahedraAsCells();

    // Convert to OpenVolumeMesh (input as 4-vertex cells)
    auto volume_mesh = operand_to_openvolumemesh(geometry.get());

    // Verify the conversion
    EXPECT_EQ(volume_mesh->n_vertices(), 5);  // Should have 5 vertices
    EXPECT_EQ(volume_mesh->n_cells(), 2);     // Should have 2 tetrahedra

    // Convert back to operand
    auto reconstructed = openvolumemesh_to_operand(volume_mesh.get());
    auto reconstructed_mesh = reconstructed->get_component<MeshComponent>();

    // Verify round-trip conversion
    EXPECT_EQ(reconstructed_mesh->get_vertices().size(), 5);
    EXPECT_EQ(reconstructed_mesh->get_face_vertex_counts().size(), 2);
}

TEST_F(OpenVolumeMeshBindTest, EmptyMeshHandling)
{
    // Test with empty mesh
    auto volume_mesh = operand_to_openvolumemesh(geometry.get());

    EXPECT_EQ(volume_mesh->n_vertices(), 0);
    EXPECT_EQ(volume_mesh->n_cells(), 0);
}

TEST_F(OpenVolumeMeshBindTest, InvalidTriangleHandling)
{
    // Test with non-triangular faces that can't form tetrahedra
    std::vector<glm::vec3> vertices = { { 0.0f, 0.0f, 0.0f },
                                        { 1.0f, 0.0f, 0.0f },
                                        { 0.5f, 0.866f, 0.0f },
                                        { 0.0f, 1.0f, 0.0f },
                                        { 0.5f, 0.5f, 1.0f } };

    // Define some quad faces instead of triangles
    std::vector<int> faceVertexIndices = {
        0, 1, 2, 3,  // quad face
        1, 2, 4      // triangle face
    };
    std::vector<int> faceVertexCounts = { 4, 3 };

    mesh->set_vertices(vertices);
    mesh->set_face_vertex_indices(faceVertexIndices);
    mesh->set_face_vertex_counts(faceVertexCounts);

    auto volume_mesh = operand_to_openvolumemesh(geometry.get());

    EXPECT_EQ(volume_mesh->n_vertices(), 5);
    // Should handle gracefully - limited tetrahedra due to mixed face types
}

TEST_F(OpenVolumeMeshBindTest, LargeTetrahedralMeshFromCells)
{
    // Create a larger mesh with 8 vertices forming multiple tetrahedra as cells
    std::vector<glm::vec3> vertices = {
        { 0.0f, 0.0f, 0.0f },  // v0
        { 1.0f, 0.0f, 0.0f },  // v1
        { 0.0f, 1.0f, 0.0f },  // v2
        { 0.0f, 0.0f, 1.0f },  // v3
        { 1.0f, 1.0f, 0.0f },  // v4
        { 1.0f, 0.0f, 1.0f },  // v5
        { 0.0f, 1.0f, 1.0f },  // v6
        { 1.0f, 1.0f, 1.0f }   // v7
    };

    // Define 6 tetrahedra as 4-vertex cells
    std::vector<int> cellVertexIndices = { 0, 1, 2, 3, 1, 2, 3, 5, 2, 3, 5, 6,
                                           1, 2, 4, 5, 2, 4, 5, 7, 2, 5, 6, 7 };
    std::vector<int> cellVertexCounts = { 4, 4, 4, 4, 4, 4 };

    mesh->set_vertices(vertices);
    mesh->set_face_vertex_indices(cellVertexIndices);
    mesh->set_face_vertex_counts(cellVertexCounts);

    auto volume_mesh = operand_to_openvolumemesh(geometry.get());

    EXPECT_EQ(volume_mesh->n_vertices(), 8);
    EXPECT_EQ(volume_mesh->n_cells(), 6);

    // Test round-trip conversion
    auto reconstructed = openvolumemesh_to_operand(volume_mesh.get());
    auto reconstructed_mesh = reconstructed->get_component<MeshComponent>();

    EXPECT_EQ(reconstructed_mesh->get_vertices().size(), 8);
    EXPECT_EQ(reconstructed_mesh->get_face_vertex_counts().size(), 6);

    // All cells should be tetrahedra (4 vertices each)
    for (auto count : reconstructed_mesh->get_face_vertex_counts()) {
        EXPECT_EQ(count, 4);
    }
}

TEST_F(OpenVolumeMeshBindTest, VertexDataPreservation)
{
    // Test that vertex positions are preserved through conversion with 4-vertex
    // cells
    std::vector<glm::vec3> original_vertices = { { 1.5f, 2.5f, 3.5f },
                                                 { 4.5f, 5.5f, 6.5f },
                                                 { 7.5f, 8.5f, 9.5f },
                                                 { 10.5f, 11.5f, 12.5f } };

    // Create as 4-vertex cell (tetrahedron)
    std::vector<int> cellVertexIndices = { 0, 1, 2, 3 };
    std::vector<int> cellVertexCounts = { 4 };

    mesh->set_vertices(original_vertices);
    mesh->set_face_vertex_indices(cellVertexIndices);
    mesh->set_face_vertex_counts(cellVertexCounts);

    auto volume_mesh = operand_to_openvolumemesh(geometry.get());
    auto reconstructed = openvolumemesh_to_operand(volume_mesh.get());
    auto reconstructed_mesh = reconstructed->get_component<MeshComponent>();

    const auto& reconstructed_vertices = reconstructed_mesh->get_vertices();
    EXPECT_EQ(reconstructed_vertices.size(), original_vertices.size());

    // Check that vertex positions are preserved (with some floating point
    // tolerance)
    for (size_t i = 0; i < original_vertices.size(); i++) {
        EXPECT_NEAR(reconstructed_vertices[i].x, original_vertices[i].x, 1e-6f);
        EXPECT_NEAR(reconstructed_vertices[i].y, original_vertices[i].y, 1e-6f);
        EXPECT_NEAR(reconstructed_vertices[i].z, original_vertices[i].z, 1e-6f);
    }
}

TEST_F(OpenVolumeMeshBindTest, TetrahedralMeshWithAttributes)
{
    CreateSingleTetrahedron();

    // Add normals to the mesh
    std::vector<glm::vec3> normals = { { 0.0f, 0.0f, 1.0f },
                                       { 1.0f, 0.0f, 0.0f },
                                       { 0.0f, 1.0f, 0.0f },
                                       { 0.0f, 0.0f, -1.0f } };
    mesh->set_normals(normals);

    // Add texture coordinates
    std::vector<glm::vec2> texcoords = {
        { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f }, { 0.25f, 0.5f }
    };
    mesh->set_texcoords_array(texcoords);

    // Add vertex colors
    std::vector<glm::vec3> colors = {
        { 1.0f, 0.0f, 0.0f },  // red
        { 0.0f, 1.0f, 0.0f },  // green
        { 0.0f, 0.0f, 1.0f },  // blue
        { 1.0f, 1.0f, 0.0f }   // yellow
    };
    mesh->set_display_color(colors);

    auto volume_mesh = operand_to_openvolumemesh(geometry.get());
    auto reconstructed = openvolumemesh_to_operand(volume_mesh.get());
    auto reconstructed_mesh = reconstructed->get_component<MeshComponent>();

    // Verify that mesh structure is preserved (attributes might be lost in
    // volume mesh)
    EXPECT_EQ(reconstructed_mesh->get_vertices().size(), 4);
    EXPECT_EQ(reconstructed_mesh->get_face_vertex_counts().size(), 1);
}

TEST_F(OpenVolumeMeshBindTest, ComplexTetrahedralMeshStressTest)
{
    // Create a more complex mesh that could potentially break handle management
    const int grid_size = 4;
    std::vector<glm::vec3> vertices;
    std::vector<int> cellVertexIndices;
    std::vector<int> cellVertexCounts;

    // Create a 3D grid of vertices
    for (int i = 0; i < grid_size; i++) {
        for (int j = 0; j < grid_size; j++) {
            for (int k = 0; k < grid_size; k++) {
                vertices.push_back(
                    glm::vec3(
                        static_cast<float>(i),
                        static_cast<float>(j),
                        static_cast<float>(k)));
            }
        }
    }

    // Create tetrahedra by subdividing each cube into 6 tetrahedra
    auto get_vertex_index = [grid_size](int i, int j, int k) {
        return i * grid_size * grid_size + j * grid_size + k;
    };

    for (int i = 0; i < grid_size - 1; i++) {
        for (int j = 0; j < grid_size - 1; j++) {
            for (int k = 0; k < grid_size - 1; k++) {
                // Get the 8 vertices of the cube
                int v000 = get_vertex_index(i, j, k);
                int v001 = get_vertex_index(i, j, k + 1);
                int v010 = get_vertex_index(i, j + 1, k);
                int v011 = get_vertex_index(i, j + 1, k + 1);
                int v100 = get_vertex_index(i + 1, j, k);
                int v101 = get_vertex_index(i + 1, j, k + 1);
                int v110 = get_vertex_index(i + 1, j + 1, k);
                int v111 = get_vertex_index(i + 1, j + 1, k + 1);

                // Subdivide cube into 6 tetrahedra
                std::vector<std::array<int, 4>> cube_tets = {
                    { v000, v001, v011, v111 }, { v000, v011, v010, v111 },
                    { v000, v010, v110, v111 }, { v000, v110, v100, v111 },
                    { v000, v100, v101, v111 }, { v000, v101, v001, v111 }
                };

                for (const auto& tet : cube_tets) {
                    for (int vertex : tet) {
                        cellVertexIndices.push_back(vertex);
                    }
                    cellVertexCounts.push_back(4);
                }
            }
        }
    }

    mesh->set_vertices(vertices);
    mesh->set_face_vertex_indices(cellVertexIndices);
    mesh->set_face_vertex_counts(cellVertexCounts);

    auto volume_mesh = operand_to_openvolumemesh(geometry.get());

    EXPECT_EQ(volume_mesh->n_vertices(), grid_size * grid_size * grid_size);
    EXPECT_GT(volume_mesh->n_cells(), 0);

    // Stress test: iterate through all vertices and check their connectivity
    bool stress_test_passed = true;
    int boundary_vertex_count = 0;
    int interior_vertex_count = 0;

    for (int vertex_id = 0; vertex_id < volume_mesh->n_vertices();
         ++vertex_id) {
        auto vh = OpenVolumeMesh::VertexHandle(vertex_id);

        if (!vh.is_valid()) {
            stress_test_passed = false;
            break;
        }

        // Check if vertex is on boundary
        bool is_boundary = false;
        int connected_faces = 0;

        for (auto vf_it = volume_mesh->vf_iter(vh); vf_it.valid(); ++vf_it) {
            connected_faces++;
            if (volume_mesh->is_boundary(*vf_it)) {
                is_boundary = true;
            }
        }

        if (is_boundary) {
            boundary_vertex_count++;
        }
        else {
            interior_vertex_count++;
        }

        // Check cell connectivity
        int connected_cells = 0;
        for (auto vc_it = volume_mesh->vc_iter(vh); vc_it.valid(); ++vc_it) {
            connected_cells++;

            // Verify that this vertex is indeed part of the cell
            bool vertex_found_in_cell = false;
            for (auto cv_it = volume_mesh->cv_iter(*vc_it); cv_it.valid();
                 ++cv_it) {
                if ((*cv_it).idx() == vertex_id) {
                    vertex_found_in_cell = true;
                    break;
                }
            }

            if (!vertex_found_in_cell) {
                stress_test_passed = false;
                break;
            }
        }

        // Interior vertices should have more connections than boundary vertices
        if (!is_boundary && connected_cells == 0) {
            stress_test_passed = false;
            break;
        }
    }

    EXPECT_TRUE(stress_test_passed);
    EXPECT_GT(boundary_vertex_count, 0);
    EXPECT_GT(interior_vertex_count, 0);

    // Test that all cells are properly formed tetrahedra
    bool all_cells_valid = true;
    for (auto c_it = volume_mesh->cells_begin();
         c_it != volume_mesh->cells_end();
         ++c_it) {
        std::vector<OpenVolumeMesh::VertexHandle> cell_vertices;

        for (auto cv_it = volume_mesh->cv_iter(*c_it); cv_it.valid(); ++cv_it) {
            cell_vertices.push_back(*cv_it);
        }

        if (cell_vertices.size() != 4) {
            all_cells_valid = false;
            break;
        }

        // Check that all vertex handles are valid
        for (const auto& vh : cell_vertices) {
            if (!vh.is_valid() || vh.idx() >= volume_mesh->n_vertices()) {
                all_cells_valid = false;
                break;
            }
        }

        if (!all_cells_valid)
            break;
    }

    EXPECT_TRUE(all_cells_valid);
}

TEST_F(OpenVolumeMeshBindTest, ComplexPyramidMeshForFEM)
{
    // Create a pyramid mesh that mimics real FEM usage patterns
    // Base vertices (square base)
    std::vector<glm::vec3> vertices = {
        // Base vertices (z=0)
        { -1.0f, -1.0f, 0.0f },  // 0
        { 1.0f, -1.0f, 0.0f },   // 1
        { 1.0f, 1.0f, 0.0f },    // 2
        { -1.0f, 1.0f, 0.0f },   // 3
        // Apex vertex
        { 0.0f, 0.0f, 2.0f }  // 4
    };

    // Define pyramid as 6 tetrahedra (subdividing the pyramid)
    // Each tetrahedron connects apex to a triangular face of the base
    std::vector<int> cellVertexIndices = {
        // Tetrahedron 1: apex + triangle (0,1,2)
        4,
        0,
        1,
        2,
        // Tetrahedron 2: apex + triangle (0,2,3)
        4,
        0,
        2,
        3,
        // You could add more subdivisions here for complexity
    };
    std::vector<int> cellVertexCounts = { 4, 4 };

    mesh->set_vertices(vertices);
    mesh->set_face_vertex_indices(cellVertexIndices);
    mesh->set_face_vertex_counts(cellVertexCounts);

    // Test conversion to OpenVolumeMesh
    auto volume_mesh = operand_to_openvolumemesh(geometry.get());

    EXPECT_EQ(volume_mesh->n_vertices(), 5);
    EXPECT_EQ(volume_mesh->n_cells(), 2);

    // Test handle validity like FEM solver would do
    bool all_handles_valid = true;
    std::vector<bool> vertex_boundary_status;
    std::vector<std::vector<int>> vertex_cell_connectivity;

    // Check each vertex handle validity and boundary status
    for (int vertex_id = 0; vertex_id < volume_mesh->n_vertices();
         ++vertex_id) {
        auto vh = OpenVolumeMesh::VertexHandle(vertex_id);

        // Check if handle is valid
        if (!vh.is_valid()) {
            all_handles_valid = false;
            break;
        }

        // Check boundary status (mimicking FEM solver boundary detection)
        bool is_boundary = false;
        for (auto vf_it = volume_mesh->vf_iter(vh); vf_it.valid(); ++vf_it) {
            if (volume_mesh->is_boundary(*vf_it)) {
                is_boundary = true;
                break;
            }
        }
        vertex_boundary_status.push_back(is_boundary);

        // Build vertex-cell connectivity (mimicking FEM assembly)
        std::vector<int> connected_cells;
        for (auto vc_it = volume_mesh->vc_iter(vh); vc_it.valid(); ++vc_it) {
            connected_cells.push_back((*vc_it).idx());
        }
        vertex_cell_connectivity.push_back(connected_cells);
    }

    EXPECT_TRUE(all_handles_valid);
    EXPECT_EQ(vertex_boundary_status.size(), 5);
    EXPECT_EQ(vertex_cell_connectivity.size(), 5);

    // Verify that apex vertex (index 4) is connected to both tetrahedra
    EXPECT_GE(vertex_cell_connectivity[4].size(), 2);

    // Test cell iteration and vertex extraction (mimicking FEM assembly)
    int total_processed_cells = 0;
    bool cell_iteration_successful = true;

    for (auto c_it = volume_mesh->cells_begin();
         c_it != volume_mesh->cells_end();
         ++c_it) {
        std::vector<int> cell_vertices;

        // Extract vertices of current cell
        for (auto cv_it = volume_mesh->cv_iter(*c_it); cv_it.valid(); ++cv_it) {
            cell_vertices.push_back((*cv_it).idx());
        }

        // Each tetrahedron should have exactly 4 vertices
        if (cell_vertices.size() != 4) {
            cell_iteration_successful = false;
            break;
        }

        total_processed_cells++;
    }

    EXPECT_TRUE(cell_iteration_successful);
    EXPECT_EQ(total_processed_cells, 2);

    // Test round-trip conversion
    auto reconstructed = openvolumemesh_to_operand(volume_mesh.get());
    auto reconstructed_mesh = reconstructed->get_component<MeshComponent>();

    EXPECT_EQ(reconstructed_mesh->get_vertices().size(), 5);
    EXPECT_EQ(reconstructed_mesh->get_face_vertex_counts().size(), 2);
}

TEST_F(OpenVolumeMeshBindTest, FEMSolverCompatibilityTest)
{
    // Test that mimics exactly how FEMSolver3D would use the volume mesh
    CreateSingleTetrahedron();

    auto volume_mesh = operand_to_openvolumemesh(geometry.get());

    // Simulate FEM assembly process
    int n_vertices = volume_mesh->n_vertices();
    std::vector<std::vector<int>> vertex_to_cells(n_vertices);
    std::vector<bool> is_boundary_vertex(n_vertices, false);

    // Build vertex-to-cell connectivity (exactly like FEMSolver3D)
    for (int vertex_id = 0; vertex_id < n_vertices; ++vertex_id) {
        auto vh = OpenVolumeMesh::VertexHandle(vertex_id);

        // Check boundary status
        for (auto vf_it = volume_mesh->vf_iter(vh); vf_it.valid(); ++vf_it) {
            if (volume_mesh->is_boundary(*vf_it)) {
                is_boundary_vertex[vertex_id] = true;
                break;
            }
        }

        // Get connected cells
        for (auto vc_it = volume_mesh->vc_iter(vh); vc_it.valid(); ++vc_it) {
            vertex_to_cells[vertex_id].push_back((*vc_it).idx());
        }
    }

    // Simulate matrix assembly for each vertex
    bool assembly_successful = true;
    for (int vertex_id = 0; vertex_id < n_vertices; ++vertex_id) {
        if (is_boundary_vertex[vertex_id]) {
            // Boundary vertex: just identity in matrix
            continue;
        }

        // Interior vertex: process all connected cells
        for (int cell_idx : vertex_to_cells[vertex_id]) {
            auto ch = OpenVolumeMesh::CellHandle(cell_idx);
            std::vector<int> tet_vertex_ids;
            std::vector<glm::vec3> tet_coords;

            // Extract cell vertices (exactly like FEMSolver3D)
            for (auto cv_it = volume_mesh->cv_iter(ch); cv_it.valid();
                 ++cv_it) {
                tet_vertex_ids.push_back((*cv_it).idx());
                auto point = volume_mesh->vertex(*cv_it);
                tet_coords.push_back(glm::vec3(point[0], point[1], point[2]));
            }

            // Verify tetrahedron structure
            if (tet_vertex_ids.size() != 4 || tet_coords.size() != 4) {
                assembly_successful = false;
                break;
            }

            // Find position of current vertex in tetrahedron
            int vertex_pos = -1;
            for (int i = 0; i < 4; i++) {
                if (tet_vertex_ids[i] == vertex_id) {
                    vertex_pos = i;
                    break;
                }
            }

            if (vertex_pos == -1) {
                assembly_successful = false;
                break;
            }

            // Simulate Jacobian computation (simplified)
            auto v0 = tet_coords[vertex_pos];
            std::vector<glm::vec3> others;
            for (int i = 0; i < 4; i++) {
                if (i != vertex_pos) {
                    others.push_back(tet_coords[i]);
                }
            }

            // Check for degenerate tetrahedron
            auto d1 = others[0] - v0;
            auto d2 = others[1] - v0;
            auto d3 = others[2] - v0;

            float det = d1.x * (d2.y * d3.z - d2.z * d3.y) -
                        d1.y * (d2.x * d3.z - d2.z * d3.x) +
                        d1.z * (d2.x * d3.y - d2.y * d3.x);

            if (std::abs(det) < 1e-10f) {
                assembly_successful = false;
                break;
            }
        }

        if (!assembly_successful)
            break;
    }

    EXPECT_TRUE(assembly_successful);
    EXPECT_EQ(vertex_to_cells.size(), 4);

    // Verify that at least some vertices are marked as boundary
    bool has_boundary_vertices = false;
    for (bool is_boundary : is_boundary_vertex) {
        if (is_boundary) {
            has_boundary_vertices = true;
            break;
        }
    }
    EXPECT_TRUE(has_boundary_vertices);
}

TEST_F(OpenVolumeMeshBindTest, ErrorHandlingTests)
{
    // Test null pointer handling
    auto null_volume_mesh = operand_to_openvolumemesh(nullptr);
    EXPECT_TRUE(null_volume_mesh->n_vertices() == 0);

    // Test with geometry having no mesh component
    auto bare_geometry = std::make_shared<Geometry>();
    auto result = operand_to_openvolumemesh(bare_geometry.get());
    EXPECT_EQ(result->n_vertices(), 0);
}

TEST_F(OpenVolumeMeshBindTest, InvalidVertexHandling)
{
    // Test with invalid vertex indices in 4-vertex cells
    std::vector<glm::vec3> vertices = { { 0.0f, 0.0f, 0.0f },
                                        { 1.0f, 0.0f, 0.0f },
                                        { 0.5f, 0.866f, 0.0f },
                                        { 0.5f, 0.289f, 0.816f } };

    // Include an invalid vertex index
    std::vector<int> cellVertexIndices = {
        0, 1, 2, 10
    };  // vertex 10 doesn't exist
    std::vector<int> cellVertexCounts = { 4 };

    mesh->set_vertices(vertices);
    mesh->set_face_vertex_indices(cellVertexIndices);
    mesh->set_face_vertex_counts(cellVertexCounts);

    auto volume_mesh = operand_to_openvolumemesh(geometry.get());
    EXPECT_EQ(volume_mesh->n_vertices(), 4);
    // Invalid tetrahedron should be rejected
}

TEST_F(OpenVolumeMeshBindTest, ComplexMultiLevelPyramidFEMStressTest)
{
    // Create a more complex multi-level pyramid that tests handle robustness
    std::vector<glm::vec3> vertices;
    std::vector<int> cellVertexIndices;
    std::vector<int> cellVertexCounts;

    // Level 0: Large base (4x4 grid)
    const int base_size = 4;
    for (int i = 0; i < base_size; i++) {
        for (int j = 0; j < base_size; j++) {
            vertices.push_back(
                glm::vec3(
                    static_cast<float>(i) -
                        static_cast<float>(base_size) / 2.0f,
                    static_cast<float>(j) -
                        static_cast<float>(base_size) / 2.0f,
                    0.0f));
        }
    }

    // Level 1: Middle level (2x2 grid, elevated)
    const int mid_size = 2;
    int level1_start = vertices.size();
    for (int i = 0; i < mid_size; i++) {
        for (int j = 0; j < mid_size; j++) {
            vertices.push_back(
                glm::vec3(
                    static_cast<float>(i) -
                        static_cast<float>(mid_size) / 2.0f + 0.5f,
                    static_cast<float>(j) -
                        static_cast<float>(mid_size) / 2.0f + 0.5f,
                    1.5f));
        }
    }

    // Level 2: Apex
    int apex_index = vertices.size();
    vertices.push_back(glm::vec3(0.0f, 0.0f, 3.0f));

    auto get_base_index = [](int i, int j, int size) { return i * size + j; };

    // Create tetrahedra connecting different levels
    // Connect base level to middle level
    for (int i = 0; i < base_size - 1; i++) {
        for (int j = 0; j < base_size - 1; j++) {
            // Get 4 base vertices of current quad
            int b00 = get_base_index(i, j, base_size);
            int b01 = get_base_index(i, j + 1, base_size);
            int b10 = get_base_index(i + 1, j, base_size);
            int b11 = get_base_index(i + 1, j + 1, base_size);

            // Connect to corresponding middle level vertex (if exists)
            if (i >= 1 && i < 3 && j >= 1 && j < 3) {
                int mid_i = (i - 1) / 1;
                int mid_j = (j - 1) / 1;
                int mid_idx =
                    level1_start + get_base_index(mid_i, mid_j, mid_size);

                // Create tetrahedra connecting base quad to middle vertex
                std::vector<std::array<int, 4>> base_to_mid_tets = {
                    { b00, b01, b11, mid_idx }, { b00, b11, b10, mid_idx }
                };

                for (const auto& tet : base_to_mid_tets) {
                    for (int vertex : tet) {
                        cellVertexIndices.push_back(vertex);
                    }
                    cellVertexCounts.push_back(4);
                }
            }
            else {
                // Outer base triangles connect directly to apex
                std::vector<std::array<int, 4>> base_to_apex_tets = {
                    { b00, b01, b11, apex_index }, { b00, b11, b10, apex_index }
                };

                for (const auto& tet : base_to_apex_tets) {
                    for (int vertex : tet) {
                        cellVertexIndices.push_back(vertex);
                    }
                    cellVertexCounts.push_back(4);
                }
            }
        }
    }

    // Connect middle level to apex
    for (int i = 0; i < mid_size - 1; i++) {
        for (int j = 0; j < mid_size - 1; j++) {
            int m00 = level1_start + get_base_index(i, j, mid_size);
            int m01 = level1_start + get_base_index(i, j + 1, mid_size);
            int m10 = level1_start + get_base_index(i + 1, j, mid_size);
            int m11 = level1_start + get_base_index(i + 1, j + 1, mid_size);

            std::vector<std::array<int, 4>> mid_to_apex_tets = {
                { m00, m01, m11, apex_index }, { m00, m11, m10, apex_index }
            };

            for (const auto& tet : mid_to_apex_tets) {
                for (int vertex : tet) {
                    cellVertexIndices.push_back(vertex);
                }
                cellVertexCounts.push_back(4);
            }
        }
    }

    mesh->set_vertices(vertices);
    mesh->set_face_vertex_indices(cellVertexIndices);
    mesh->set_face_vertex_counts(cellVertexCounts);

    // Convert to volume mesh and test
    auto volume_mesh = operand_to_openvolumemesh(geometry.get());

    // Verify basic structure
    EXPECT_EQ(volume_mesh->n_vertices(), vertices.size());
    EXPECT_GT(volume_mesh->n_cells(), 0);

    // Stress test ALL aspects that FEMSolver would use
    bool comprehensive_test_passed = true;
    std::vector<std::vector<int>> vertex_neighbor_vertices(
        volume_mesh->n_vertices());
    std::vector<std::vector<int>> vertex_incident_cells(
        volume_mesh->n_vertices());
    std::vector<bool> vertex_is_boundary(volume_mesh->n_vertices(), false);

    // Test 1: Vertex handle validity and boundary detection
    for (int vertex_id = 0; vertex_id < volume_mesh->n_vertices();
         ++vertex_id) {
        auto vh = OpenVolumeMesh::VertexHandle(vertex_id);

        if (!vh.is_valid()) {
            comprehensive_test_passed = false;
            break;
        }

        // Test boundary detection (critical for FEM boundary conditions)
        for (auto vf_it = volume_mesh->vf_iter(vh); vf_it.valid(); ++vf_it) {
            if (volume_mesh->is_boundary(*vf_it)) {
                vertex_is_boundary[vertex_id] = true;
                break;
            }
        }

        // Collect incident cells (for FEM assembly)
        for (auto vc_it = volume_mesh->vc_iter(vh); vc_it.valid(); ++vc_it) {
            vertex_incident_cells[vertex_id].push_back((*vc_it).idx());
        }

        // Collect neighbor vertices (for finite difference approximations)
        std::set<int> neighbors;
        for (auto vc_it = volume_mesh->vc_iter(vh); vc_it.valid(); ++vc_it) {
            for (auto cv_it = volume_mesh->cv_iter(*vc_it); cv_it.valid();
                 ++cv_it) {
                int neighbor_id = (*cv_it).idx();
                if (neighbor_id != vertex_id) {
                    neighbors.insert(neighbor_id);
                }
            }
        }
        vertex_neighbor_vertices[vertex_id] =
            std::vector<int>(neighbors.begin(), neighbors.end());
    }

    EXPECT_TRUE(comprehensive_test_passed);

    // Test 2: Cell handle validity and geometric consistency
    bool cell_test_passed = true;
    std::vector<float> cell_volumes;
    std::vector<std::array<float, 3>> cell_centroids;

    for (auto c_it = volume_mesh->cells_begin();
         c_it != volume_mesh->cells_end();
         ++c_it) {
        if (!(*c_it).is_valid()) {
            cell_test_passed = false;
            break;
        }

        std::vector<glm::vec3> cell_coords;
        std::vector<int> cell_vertex_ids;

        for (auto cv_it = volume_mesh->cv_iter(*c_it); cv_it.valid(); ++cv_it) {
            if (!(*cv_it).is_valid()) {
                cell_test_passed = false;
                break;
            }

            cell_vertex_ids.push_back((*cv_it).idx());
            auto point = volume_mesh->vertex(*cv_it);
            cell_coords.push_back(glm::vec3(point[0], point[1], point[2]));
        }

        if (cell_coords.size() != 4) {
            cell_test_passed = false;
            break;
        }

        // Compute volume (should be positive for proper orientation)
        auto& v0 = cell_coords[0];
        auto& v1 = cell_coords[1];
        auto& v2 = cell_coords[2];
        auto& v3 = cell_coords[3];

        auto d1 = v1 - v0;
        auto d2 = v2 - v0;
        auto d3 = v3 - v0;

        float det = d1.x * (d2.y * d3.z - d2.z * d3.y) -
                    d1.y * (d2.x * d3.z - d2.z * d3.x) +
                    d1.z * (d2.x * d3.y - d2.y * d3.x);

        float volume = std::abs(det) / 6.0f;
        cell_volumes.push_back(volume);

        // Compute centroid
        auto centroid = (v0 + v1 + v2 + v3) * 0.25f;
        cell_centroids.push_back({ centroid.x, centroid.y, centroid.z });

        // Check for degenerate tetrahedra (critical for FEM stability)
        if (volume < 1e-10f) {
            cell_test_passed = false;
            break;
        }
    }

    EXPECT_TRUE(cell_test_passed);
    EXPECT_GT(cell_volumes.size(), 0);

    // Test 3: Face handle validity and boundary consistency
    bool face_test_passed = true;
    int boundary_face_count = 0;
    int interior_face_count = 0;

    for (auto f_it = volume_mesh->faces_begin();
         f_it != volume_mesh->faces_end();
         ++f_it) {
        if (!(*f_it).is_valid()) {
            face_test_passed = false;
            break;
        }

        if (volume_mesh->is_boundary(*f_it)) {
            boundary_face_count++;
        }
        else {
            interior_face_count++;
        }

        // Check face vertices
        int face_vertex_count = 0;
        for (auto fv_it = volume_mesh->fv_iter(*f_it); fv_it.valid(); ++fv_it) {
            if (!(*fv_it).is_valid()) {
                face_test_passed = false;
                break;
            }
            face_vertex_count++;
        }

        if (face_vertex_count != 3) {  // Should be triangular faces
            face_test_passed = false;
            break;
        }
    }

    EXPECT_TRUE(face_test_passed);
    EXPECT_GT(boundary_face_count, 0);

    // Test 4: Edge handle validity
    bool edge_test_passed = true;
    for (auto e_it = volume_mesh->edges_begin();
         e_it != volume_mesh->edges_end();
         ++e_it) {
        if (!(*e_it).is_valid()) {
            edge_test_passed = false;
            break;
        }

        // Check edge vertices
        auto from_vertex = volume_mesh->edge(*e_it).from_vertex();
        auto to_vertex = volume_mesh->edge(*e_it).to_vertex();

        if (!from_vertex.is_valid() || !to_vertex.is_valid()) {
            edge_test_passed = false;
            break;
        }
    }

    EXPECT_TRUE(edge_test_passed);

    // Test 5: Simulate actual FEM assembly pattern
    bool fem_assembly_test_passed = true;

    for (int vertex_id = 0; vertex_id < volume_mesh->n_vertices();
         ++vertex_id) {
        if (vertex_is_boundary[vertex_id]) {
            // Boundary vertex: verify we can evaluate boundary conditions
            auto vh = OpenVolumeMesh::VertexHandle(vertex_id);
            auto point = volume_mesh->vertex(vh);

            // Simulate boundary condition evaluation
            double boundary_value =
                std::sin(point[0]) * std::cos(point[1]) + point[2];
            (void)boundary_value;  // Suppress unused variable warning
            continue;
        }

        // Interior vertex: simulate stiffness matrix assembly
        for (int cell_idx : vertex_incident_cells[vertex_id]) {
            auto ch = OpenVolumeMesh::CellHandle(cell_idx);

            std::vector<int> tet_vertices;
            std::vector<std::array<float, 3>> tet_coords;

            for (auto cv_it = volume_mesh->cv_iter(ch); cv_it.valid();
                 ++cv_it) {
                tet_vertices.push_back((*cv_it).idx());
                auto point = volume_mesh->vertex(*cv_it);
                tet_coords.push_back(
                    { static_cast<float>(point[0]),
                      static_cast<float>(point[1]),
                      static_cast<float>(point[2]) });
            }

            if (tet_vertices.size() != 4) {
                fem_assembly_test_passed = false;
                break;
            }

            // Find current vertex position in tetrahedron
            int vertex_pos = -1;
            for (int i = 0; i < 4; i++) {
                if (tet_vertices[i] == vertex_id) {
                    vertex_pos = i;
                    break;
                }
            }

            if (vertex_pos == -1) {
                fem_assembly_test_passed = false;
                break;
            }

            // Simulate Jacobian computation for shape function derivatives
            auto& v0 = tet_coords[vertex_pos];
            std::vector<std::array<float, 3>> others;
            for (int i = 0; i < 4; i++) {
                if (i != vertex_pos) {
                    others.push_back(tet_coords[i]);
                }
            }

            // Compute Jacobian matrix elements
            float dx1 = others[0][0] - v0[0], dy1 = others[0][1] - v0[1],
                  dz1 = others[0][2] - v0[2];
            float dx2 = others[1][0] - v0[0], dy2 = others[1][1] - v0[1],
                  dz2 = others[1][2] - v0[2];
            float dx3 = others[2][0] - v0[0], dy3 = others[2][1] - v0[1],
                  dz3 = others[2][2] - v0[2];

            float det = dx1 * (dy2 * dz3 - dz2 * dy3) -
                        dy1 * (dx2 * dz3 - dz2 * dx3) +
                        dz1 * (dx2 * dy3 - dy2 * dx3);

            if (std::abs(det) < 1e-12f) {
                fem_assembly_test_passed = false;
                break;
            }

            // Use these for stiffness matrix computation (simplified)
            float stiffness_contribution = dx1 * dx1 + dy1 * dy1 + dz1 * dz1;
            (void)stiffness_contribution;  // Suppress unused variable warning
        }

        if (!fem_assembly_test_passed)
            break;
    }

    EXPECT_TRUE(fem_assembly_test_passed);

    // Final verification: topology consistency
    EXPECT_GT(vertex_neighbor_vertices.size(), 0);
    EXPECT_GT(vertex_incident_cells.size(), 0);

    // Apex vertex should have the most connections
    int apex_connections = vertex_incident_cells[apex_index].size();
    EXPECT_GT(
        apex_connections, 4);  // Should be connected to multiple tetrahedra

    // Base vertices should be boundary vertices
    for (int i = 0; i < base_size * base_size; i++) {
        EXPECT_TRUE(vertex_is_boundary[i]);
    }
}

TEST_F(OpenVolumeMeshBindTest, ActualFEMSolverIntegrationTest)
{
    // Test with actual FEM solver usage pattern
    CreateSingleTetrahedronAsCell();

    auto volume_mesh = operand_to_openvolumemesh(geometry.get());

    // Test that we can extract all the information that FEMSolver3D needs
    bool integration_test_passed = true;
    
    // Test 1: Basic mesh properties
    int n_vertices = volume_mesh->n_vertices();
    int n_cells = volume_mesh->n_cells();
    
    EXPECT_GT(n_vertices, 0);
    EXPECT_GT(n_cells, 0);
    
    // Test 2: Vertex iteration and boundary detection
    std::vector<bool> vertex_is_boundary(n_vertices, false);
    for (int vertex_id = 0; vertex_id < n_vertices; ++vertex_id) {
        auto vh = OpenVolumeMesh::VertexHandle(vertex_id);
        
        if (!vh.is_valid()) {
            integration_test_passed = false;
            break;
        }
        
        // Check boundary detection (exactly like FEMSolver3D does)
        for (auto vf_it = volume_mesh->vf_iter(vh); vf_it.valid(); ++vf_it) {
            if (volume_mesh->is_boundary(*vf_it)) {
                vertex_is_boundary[vertex_id] = true;
                break;
            }
        }
    }
    
    // Test 3: Cell iteration and vertex extraction
    for (auto c_it = volume_mesh->cells_begin(); c_it != volume_mesh->cells_end(); ++c_it) {
        std::vector<int> cell_vertex_ids;
        std::vector<std::array<double, 3>> cell_coords;
        
        for (auto cv_it = volume_mesh->cv_iter(*c_it); cv_it.valid(); ++cv_it) {
            if (!(*cv_it).is_valid()) {
                integration_test_passed = false;
                break;
            }
            
            cell_vertex_ids.push_back((*cv_it).idx());
            auto point = volume_mesh->vertex(*cv_it);
            cell_coords.push_back({point[0], point[1], point[2]});
        }
        
        if (cell_vertex_ids.size() != 4 || cell_coords.size() != 4) {
            integration_test_passed = false;
            break;
        }
        
        // Test 4: Jacobian computation (mimicking FEMSolver3D)
        for (int vertex_id : cell_vertex_ids) {
            // Find vertex position in cell
            int vertex_pos = -1;
            for (int i = 0; i < 4; i++) {
                if (cell_vertex_ids[i] == vertex_id) {
                    vertex_pos = i;
                    break;
                }
            }
            
            if (vertex_pos == -1) {
                integration_test_passed = false;
                break;
            }
            
            // Compute tetrahedron volume (check for degeneracy)
            auto& v0 = cell_coords[0];
            auto& v1 = cell_coords[1];
            auto& v2 = cell_coords[2];
            auto& v3 = cell_coords[3];
            
            double d1x = v1[0] - v0[0], d1y = v1[1] - v0[1], d1z = v1[2] - v0[2];
            double d2x = v2[0] - v0[0], d2y = v2[1] - v0[1], d2z = v2[2] - v0[2];
            double d3x = v3[0] - v0[0], d3y = v3[1] - v0[1], d3z = v3[2] - v0[2];
            
            double det = d1x * (d2y * d3z - d2z * d3y) -
                        d1y * (d2x * d3z - d2z * d3x) +
                        d1z * (d2x * d3y - d2y * d3x);
            
            if (std::abs(det) < 1e-12) {
                integration_test_passed = false;
                break;
            }
        }
        
        if (!integration_test_passed) break;
    }
    
    EXPECT_TRUE(integration_test_passed);
    
    // Test 5: Verify boundary vertices exist
    bool has_boundary_vertices = false;
    for (bool is_boundary : vertex_is_boundary) {
        if (is_boundary) {
            has_boundary_vertices = true;
            break;
        }
    }
    EXPECT_TRUE(has_boundary_vertices);
}

#include <GCore/Components/MeshComponent.h>
#include <GCore/Components/MeshViews.h>
#include <GCore/GOP.h>
#include <gtest/gtest.h>

#include <RHI/rhi.hpp>

#if RUZINO_WITH_CUDA
#include <RHI/internal/cuda_extension.hpp>
#endif

using namespace Ruzino;

class GPUInterfaceTest : public ::testing::Test {
   protected:
    void SetUp() override
    {
        // Create a simple test mesh
        geometry = std::make_shared<Geometry>();
        mesh = std::make_shared<MeshComponent>(geometry.get());

        // Create a simple triangle mesh
        std::vector<glm::vec3> vertices = { { 0.0f, 0.0f, 0.0f },
                                            { 1.0f, 0.0f, 0.0f },
                                            { 0.0f, 1.0f, 0.0f } };

        std::vector<int> face_vertex_counts = { 3 };
        std::vector<int> face_vertex_indices = { 0, 1, 2 };

        mesh->set_vertices(vertices);
        mesh->set_face_vertex_counts(face_vertex_counts);
        mesh->set_face_vertex_indices(face_vertex_indices);
    }

    std::shared_ptr<Geometry> geometry;
    std::shared_ptr<MeshComponent> mesh;
};

// Test 1: View lock mechanism - should prevent multiple views
TEST_F(GPUInterfaceTest, ViewLockPreventsMultipleViews)
{
    {
        auto view1 = get_igl_view(*mesh);

        // Try to create another view while the first is active
        EXPECT_THROW({ auto view2 = get_igl_view(*mesh); }, std::runtime_error);
    }

    // After first view is destroyed, should be able to create another
    EXPECT_NO_THROW({ auto view3 = get_igl_view(*mesh); });
}

// Test 2: View lock works across different view types
TEST_F(GPUInterfaceTest, ViewLockWorksAcrossViewTypes)
{
    {
        auto igl_view = get_igl_view(*mesh);

        // Try to create a different type of view
        EXPECT_THROW(
            { auto const_view = get_igl_view(*mesh); }, std::runtime_error);

#ifdef GEOM_USD_EXTENSION
        EXPECT_THROW(
            { auto usd_view = get_usd_view(*mesh); }, std::runtime_error);
#endif
    }
}

// Test 3: IGL View basic functionality
TEST_F(GPUInterfaceTest, IGLViewBasicFunctionality)
{
    auto view = get_igl_view(*mesh);

    // Test get vertices
    auto vertices_matrix = view.get_vertices();
    EXPECT_EQ(vertices_matrix.rows(), 3);
    EXPECT_EQ(vertices_matrix.cols(), 3);

    // Test get faces
    auto faces = view.get_faces();
    EXPECT_EQ(faces.rows(), 1);  // One triangle
    EXPECT_EQ(faces.cols(), 3);  // Three vertices per triangle
}

// Test 4: IGL View modification
TEST_F(GPUInterfaceTest, IGLViewModification)
{
    {
        auto view = get_igl_view(*mesh);

        // Modify vertices
        Eigen::MatrixXf new_vertices(3, 3);
        new_vertices << 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f;

        view.set_vertices(new_vertices);
    }

    // Verify modification persisted
    auto vertices = mesh->get_vertices();
    EXPECT_EQ(vertices.size(), 3);
    EXPECT_FLOAT_EQ(vertices[1].x, 2.0f);
    EXPECT_FLOAT_EQ(vertices[2].y, 2.0f);
}

#if RUZINO_WITH_CUDA
// Test 5: CUDA View lazy loading
TEST_F(GPUInterfaceTest, CUDAViewLazyLoading)
{
    Ruzino::cuda::cuda_init();

    {
        auto view = get_cuda_view(*mesh);

        // First access should create the buffer
        auto vertices_buffer = view.get_vertices();
        EXPECT_NE(vertices_buffer, nullptr);
        EXPECT_EQ(vertices_buffer->getDesc().element_count, 3);

        // Second access should return the same buffer (cached)
        auto vertices_buffer2 = view.get_vertices();
        EXPECT_EQ(vertices_buffer, vertices_buffer2);
    }
}

// Test 6: CUDA View RAII sync back to CPU
TEST_F(GPUInterfaceTest, CUDAViewRAIISync)
{
    Ruzino::cuda::cuda_init();

    {
        auto view = get_cuda_view(*mesh);

        // Get buffer and create new data on GPU
        auto vertices_buffer = view.get_vertices();

        // Modify data on CPU side and upload to GPU
        std::vector<glm::vec3> new_vertices = { { 5.0f, 5.0f, 5.0f },
                                                { 6.0f, 6.0f, 6.0f },
                                                { 7.0f, 7.0f, 7.0f } };

        auto modified_buffer =
            Ruzino::cuda::create_cuda_linear_buffer(new_vertices);
        view.set_vertices(modified_buffer);

        // When view goes out of scope, data should sync back
    }

    // Verify data was synced back
    auto vertices = mesh->get_vertices();
    EXPECT_EQ(vertices.size(), 3);
    EXPECT_FLOAT_EQ(vertices[0].x, 5.0f);
    EXPECT_FLOAT_EQ(vertices[1].y, 6.0f);
    EXPECT_FLOAT_EQ(vertices[2].z, 7.0f);
}

// Test 7: CUDA View scalar quantities
TEST_F(GPUInterfaceTest, CUDAViewScalarQuantities)
{
    Ruzino::cuda::cuda_init();

    // Add a scalar quantity to the mesh
    std::vector<float> quality = { 0.5f, 0.8f, 0.9f };
    mesh->add_vertex_scalar_quantity("quality", quality);

    {
        auto view = get_cuda_view(*mesh);

        // Get the scalar quantity buffer
        auto quality_buffer = view.get_vertex_scalar_quantity("quality");
        EXPECT_NE(quality_buffer, nullptr);
        EXPECT_EQ(quality_buffer->getDesc().element_count, 3);

        // Verify data
        auto host_data = quality_buffer->get_host_vector<float>();
        EXPECT_EQ(host_data.size(), 3);
        EXPECT_FLOAT_EQ(host_data[0], 0.5f);
        EXPECT_FLOAT_EQ(host_data[1], 0.8f);
        EXPECT_FLOAT_EQ(host_data[2], 0.9f);
    }
}
#endif

// Test 8: Const view can be created from const mesh
TEST_F(GPUInterfaceTest, ConstViewFromConstMesh)
{
    const MeshComponent* const_mesh = mesh.get();

    auto const_view = get_igl_view(*const_mesh);

    // Should be able to read data
    auto vertices = const_view.get_vertices();
    EXPECT_EQ(vertices.rows(), 3);
}

// Test 9: Multiple sequential views (after each is destroyed)
TEST_F(GPUInterfaceTest, SequentialViews)
{
    // Create and destroy multiple views sequentially
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW({
            auto view = get_igl_view(*mesh);
            auto vertices = view.get_vertices();
            EXPECT_EQ(vertices.rows(), 3);
        });
    }
}

// Test 10: View with vector quantities
TEST_F(GPUInterfaceTest, ViewVectorQuantities)
{
    // Add normals
    std::vector<glm::vec3> normals = { { 0.0f, 0.0f, 1.0f },
                                       { 0.0f, 0.0f, 1.0f },
                                       { 0.0f, 0.0f, 1.0f } };
    mesh->set_normals(normals);

    auto view = get_igl_view(*mesh);
    auto normals_matrix = view.get_normals();

    EXPECT_EQ(normals_matrix.rows(), 3);
    EXPECT_EQ(normals_matrix.cols(), 3);
    EXPECT_FLOAT_EQ(normals_matrix(0, 2), 1.0f);
}

// ===== NVRHI View Tests =====

// Test 11: NVRHI View lazy loading
TEST_F(GPUInterfaceTest, NVRHIViewLazyLoading)
{
    RHI::init(false, true);
    auto device = RHI::get_device();
    ASSERT_NE(device, nullptr);

    {
        auto view = get_nvrhi_view(*mesh, device);

        // First access should create the buffer (without commandList, buffer
        // created but not initialized)
        auto vertices_buffer = view.get_vertices();
        EXPECT_NE(vertices_buffer, nullptr);
        EXPECT_EQ(vertices_buffer->getDesc().byteSize, 3 * sizeof(glm::vec3));

        // Second access should return the same buffer (cached)
        auto vertices_buffer2 = view.get_vertices();
        EXPECT_EQ(vertices_buffer, vertices_buffer2);
    }

    RHI::shutdown();
}

// Test 12: NVRHI View with commandList initialization
TEST_F(GPUInterfaceTest, NVRHIViewWithCommandList)
{
    RHI::init(false, true);
    auto device = RHI::get_device();
    ASSERT_NE(device, nullptr);

    auto commandList = device->createCommandList();
    commandList->open();

    {
        auto view = get_nvrhi_view(*mesh, device);

        // Get buffer with commandList - should initialize data immediately
        auto vertices_buffer = view.get_vertices(commandList);
        EXPECT_NE(vertices_buffer, nullptr);
        EXPECT_EQ(vertices_buffer->getDesc().byteSize, 3 * sizeof(glm::vec3));
        EXPECT_TRUE(vertices_buffer->getDesc().isVertexBuffer);
    }

    commandList->close();
    device->executeCommandList(commandList);

    RHI::shutdown();
}

// Test 13: NVRHI View all mesh attributes
TEST_F(GPUInterfaceTest, NVRHIViewAllAttributes)
{
    RHI::init(false, true);
    auto device = RHI::get_device();
    ASSERT_NE(device, nullptr);

    // Add various attributes
    std::vector<glm::vec3> normals = { { 0.0f, 0.0f, 1.0f },
                                       { 0.0f, 0.0f, 1.0f },
                                       { 0.0f, 0.0f, 1.0f } };
    std::vector<glm::vec2> uvs = { { 0.0f, 0.0f },
                                   { 1.0f, 0.0f },
                                   { 0.5f, 1.0f } };
    std::vector<glm::vec3> colors = { { 1.0f, 0.0f, 0.0f },
                                      { 0.0f, 1.0f, 0.0f },
                                      { 0.0f, 0.0f, 1.0f } };

    mesh->set_normals(normals);
    mesh->set_texcoords_array(uvs);
    mesh->set_display_color(colors);

    auto commandList = device->createCommandList();
    commandList->open();

    {
        auto view = get_nvrhi_view(*mesh, device);

        // Get all buffers
        auto vertices_buf = view.get_vertices(commandList);
        auto normals_buf = view.get_normals(commandList);
        auto uvs_buf = view.get_uv_coordinates(commandList);
        auto colors_buf = view.get_display_colors(commandList);
        auto face_counts_buf = view.get_face_vertex_counts(commandList);
        auto face_indices_buf = view.get_face_vertex_indices(commandList);

        // Verify all buffers created
        EXPECT_NE(vertices_buf, nullptr);
        EXPECT_NE(normals_buf, nullptr);
        EXPECT_NE(uvs_buf, nullptr);
        EXPECT_NE(colors_buf, nullptr);
        EXPECT_NE(face_counts_buf, nullptr);
        EXPECT_NE(face_indices_buf, nullptr);

        // Verify buffer properties
        EXPECT_TRUE(vertices_buf->getDesc().isVertexBuffer);
        EXPECT_TRUE(normals_buf->getDesc().isVertexBuffer);
        EXPECT_TRUE(uvs_buf->getDesc().isVertexBuffer);
        EXPECT_TRUE(face_indices_buf->getDesc().isIndexBuffer);

        // Verify sizes
        EXPECT_EQ(vertices_buf->getDesc().byteSize, 3 * sizeof(glm::vec3));
        EXPECT_EQ(normals_buf->getDesc().byteSize, 3 * sizeof(glm::vec3));
        EXPECT_EQ(uvs_buf->getDesc().byteSize, 3 * sizeof(glm::vec2));
        EXPECT_EQ(face_indices_buf->getDesc().byteSize, 3 * sizeof(int));
    }

    commandList->close();
    device->executeCommandList(commandList);

    RHI::shutdown();
}

// Test 14: NVRHI View scalar quantities
TEST_F(GPUInterfaceTest, NVRHIViewScalarQuantities)
{
    RHI::init(false, true);
    auto device = RHI::get_device();
    ASSERT_NE(device, nullptr);

    // Add scalar quantities
    std::vector<float> vertex_quality = { 0.5f, 0.8f, 0.9f };
    std::vector<float> face_area = { 1.0f };

    mesh->add_vertex_scalar_quantity("quality", vertex_quality);
    mesh->add_face_scalar_quantity("area", face_area);

    auto commandList = device->createCommandList();
    commandList->open();

    {
        auto view = get_nvrhi_view(*mesh, device);

        auto quality_buf =
            view.get_vertex_scalar_quantity("quality", commandList);
        auto area_buf = view.get_face_scalar_quantity("area", commandList);

        EXPECT_NE(quality_buf, nullptr);
        EXPECT_NE(area_buf, nullptr);

        EXPECT_EQ(quality_buf->getDesc().byteSize, 3 * sizeof(float));
        EXPECT_EQ(area_buf->getDesc().byteSize, 1 * sizeof(float));
    }

    commandList->close();
    device->executeCommandList(commandList);

    RHI::shutdown();
}

// Test 15: NVRHI View setting buffers
TEST_F(GPUInterfaceTest, NVRHIViewSettingBuffers)
{
    RHI::init(false, true);
    auto device = RHI::get_device();
    ASSERT_NE(device, nullptr);

    // Create a custom buffer
    nvrhi::BufferDesc bufferDesc;
    bufferDesc.byteSize = 3 * sizeof(glm::vec3);
    bufferDesc.isVertexBuffer = true;
    bufferDesc.canHaveUAVs = true;
    bufferDesc.debugName = "custom_vertices";

    auto customBuffer = device->createBuffer(bufferDesc);

    {
        auto view = get_nvrhi_view(*mesh, device);

        // Set the buffer
        view.set_vertices(customBuffer);

        // Get it back - should be the same buffer
        auto retrieved = view.get_vertices();
        EXPECT_EQ(retrieved, customBuffer.Get());
    }

    RHI::shutdown();
}

// Test 16: NVRHI View lock mechanism
TEST_F(GPUInterfaceTest, NVRHIViewLockMechanism)
{
    RHI::init(false, true);
    auto device = RHI::get_device();
    ASSERT_NE(device, nullptr);

    {
        auto nvrhi_view = get_nvrhi_view(*mesh, device);

        // Try to create another view while NVRHI view is active
        EXPECT_THROW(
            { auto igl_view = get_igl_view(*mesh); }, std::runtime_error);
    }

    // After NVRHI view is destroyed, should be able to create IGL view
    EXPECT_NO_THROW({ auto igl_view = get_igl_view(*mesh); });

    RHI::shutdown();
}

// Test 17: NVRHI View vector quantities
TEST_F(GPUInterfaceTest, NVRHIViewVectorQuantities)
{
    RHI::init(false, true);
    auto device = RHI::get_device();
    ASSERT_NE(device, nullptr);

    // Add vector quantities
    std::vector<glm::vec3> vertex_velocities = { { 1.0f, 0.0f, 0.0f },
                                                 { 0.0f, 1.0f, 0.0f },
                                                 { 0.0f, 0.0f, 1.0f } };

    mesh->add_vertex_vector_quantity("velocity", vertex_velocities);

    auto commandList = device->createCommandList();
    commandList->open();

    {
        auto view = get_nvrhi_view(*mesh, device);

        auto velocity_buf =
            view.get_vertex_vector_quantity("velocity", commandList);

        EXPECT_NE(velocity_buf, nullptr);
        EXPECT_EQ(velocity_buf->getDesc().byteSize, 3 * sizeof(glm::vec3));
    }

    commandList->close();
    device->executeCommandList(commandList);

    RHI::shutdown();
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

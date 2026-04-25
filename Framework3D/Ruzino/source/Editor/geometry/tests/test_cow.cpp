#include <gtest/gtest.h>
#include "GCore/GOP.h"
#include "GCore/Components/MeshComponent.h"

using namespace Ruzino;

TEST(GeometryCOW, ShallowCopyOnAssignment)
{
    // Create original geometry
    Geometry original = Geometry::CreateMesh();
    auto mesh1 = original.get_component<MeshComponent>();
    ASSERT_NE(mesh1, nullptr);
    
    // Assignment should create shallow copy
    Geometry copy = original;
    
    // Verify they share the same components vector
    EXPECT_EQ(&original.get_components(), &copy.get_components());
}

TEST(GeometryCOW, DeepCopyOnWrite)
{
    // Create original geometry
    Geometry original = Geometry::CreateMesh();
    
    // Assignment should create shallow copy
    Geometry copy = original;
    
    // Verify they share the same components vector initially
    EXPECT_EQ(&original.get_components(), &copy.get_components());
    
    // Access for write on copy should trigger COW
    auto mesh2 = copy.get_component<MeshComponent>();
    
    // Now they should have different components vectors
    EXPECT_NE(&original.get_components(), &copy.get_components());
}

TEST(GeometryCOW, ConstAccessDoesNotTriggerCOW)
{
    // Create original geometry
    Geometry original = Geometry::CreateMesh();
    
    // Assignment should create shallow copy
    Geometry copy_mutable = original;
    const Geometry& copy = copy_mutable;
    
    // Const access should not trigger COW
    auto mesh2 = copy.get_component<MeshComponent>();
    
    // They should still share the same components vector
    EXPECT_EQ(&original.get_components(), &copy.get_components());
}

TEST(GeometryCOW, AttachComponentTriggersCOW)
{
    // Create original geometry
    Geometry original = Geometry::CreateMesh();
    
    // Assignment should create shallow copy
    Geometry copy = original;
    
    // Verify shared initially
    EXPECT_EQ(&original.get_components(), &copy.get_components());
    
    // Attach component should trigger COW
    auto new_mesh = std::make_shared<MeshComponent>(&copy);
    copy.attach_component(new_mesh);
    
    // They should have different components vectors now
    EXPECT_NE(&original.get_components(), &copy.get_components());
    
    // Original should have 1 component, copy should have 2
    EXPECT_EQ(original.get_components().size(), 1);
    EXPECT_EQ(copy.get_components().size(), 2);
}

TEST(GeometryCOW, MoveSemantics)
{
    // Create original geometry
    Geometry original = Geometry::CreateMesh();
    
    // Store pointer to components for verification
    const auto* original_components = &original.get_components();
    
    // Move should transfer ownership
    Geometry moved = std::move(original);
    
    // moved should have the component at the same address
    EXPECT_EQ(&moved.get_components(), original_components);
    
    // moved should have a mesh component
    auto mesh = moved.get_component<MeshComponent>();
    EXPECT_NE(mesh, nullptr);
}

TEST(GeometryCOW, MultipleShallowCopies)
{
    // Create original geometry
    Geometry original = Geometry::CreateMesh();
    
    // Create multiple shallow copies
    Geometry copy1 = original;
    Geometry copy2 = original;
    Geometry copy3 = copy1;
    
    // All should share the same components vector
    EXPECT_EQ(&original.get_components(), &copy1.get_components());
    EXPECT_EQ(&original.get_components(), &copy2.get_components());
    EXPECT_EQ(&original.get_components(), &copy3.get_components());
    
    // Modify one copy
    auto mesh = copy2.get_component<MeshComponent>();
    
    // Only copy2 should have different components
    EXPECT_EQ(&original.get_components(), &copy1.get_components());
    EXPECT_NE(&original.get_components(), &copy2.get_components());
    EXPECT_EQ(&original.get_components(), &copy3.get_components());
}

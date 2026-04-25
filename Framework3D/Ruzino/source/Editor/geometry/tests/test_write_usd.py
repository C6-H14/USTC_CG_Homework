"""
Test writing geometry to USD files using RuzinoGraph and write_usd node

This uses stage_py to create Stage objects and convert them to GeomPayload
for use with the node graph system.
"""
import sys
import os

# Paths are set up in conftest.py
# Just need the binary_dir reference
binary_dir = os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', 'Binaries', 'Release')
binary_dir = os.path.abspath(binary_dir)

from ruzino_graph import RuzinoGraph
import stage_py
import geometry_py as geom
import numpy as np


def test_write_grid_to_usd():
    """Test writing a simple grid to USD"""
    print("\n" + "="*70)
    print("TEST: Write Grid to USD")
    print("="*70)
    
    output_file = "test_grid.usdc"
    
    g = RuzinoGraph("GridUSDTest")
    g.loadConfiguration(os.path.join(binary_dir, "geometry_nodes.json"))
    
    grid_node = g.createNode("create_grid", name="grid")
    write_node = g.createNode("write_usd", name="writer")
    
    g.addEdge(grid_node, "Geometry", write_node, "Geometry")
    
    # Prepare inputs
    inputs = {
        (grid_node, "resolution"): 10,
        (grid_node, "size"): 5.0
    }
    
    # Create Stage and convert to GeomPayload
    stage = stage_py.Stage(output_file)
    geom_payload = stage_py.create_payload_from_stage(stage, "/geom")
    
    # Set global params
    g.setGlobalParams(geom_payload)
    
    # Execute
    g.prepare_and_execute(inputs, required_node=write_node)
    
    # Save the USD stage to file
    stage.save()
    
    # Verify the file was created
    assert os.path.exists(output_file), f"USD file not created: {output_file}"
    file_size = os.path.getsize(output_file)
    print(f"✓ USD file created: {output_file} ({file_size} bytes)")
    assert file_size > 1000, f"USD file seems empty: {file_size} bytes"
    
    print(f"✓ USD file verified (use 'usdcat {output_file}' to inspect)")


def test_write_uv_sphere_to_usd():
    """Test writing UV sphere to USD"""
    print("\n" + "="*70)
    print("TEST: Write UV Sphere to USD")
    print("="*70)
    
    output_file = "test_sphere.usdc"
    
    g = RuzinoGraph("SphereUSDTest")
    g.loadConfiguration(os.path.join(binary_dir, "geometry_nodes.json"))
    
    # Create UV sphere
    sphere = g.createNode("create_uv_sphere", name="sphere")
    write_node = g.createNode("write_usd", name="writer")
    
    # Connect
    g.addEdge(sphere, "Geometry", write_node, "Geometry")
    
    # Set up inputs
    inputs = {
        (sphere, "segments"): 32,
        (sphere, "rings"): 16,
        (sphere, "radius"): 1.5,
    }
    
    # Create Stage and convert to GeomPayload
    stage = stage_py.Stage(output_file)
    geom_payload = stage_py.create_payload_from_stage(stage, "/geom")
    g.setGlobalParams(geom_payload)
    
    # Execute
    g.prepare_and_execute(inputs, required_node=write_node)
    
    # Save the stage
    stage.save()
    
    # Verify
    assert os.path.exists(output_file), f"USD file not created: {output_file}"
    file_size = os.path.getsize(output_file)
    print(f"✓ USD file created: {output_file} ({file_size} bytes)")
    assert file_size > 1000, f"USD file seems empty: {file_size} bytes"
    print(f"✓ UV sphere exported (use 'usdcat {output_file}' to inspect)")


def test_write_ico_sphere_to_usd():
    """Test writing ico sphere to USD"""
    print("\n" + "="*70)
    print("TEST: Write Ico Sphere to USD")
    print("="*70)
    
    output_file = "test_ico.usdc"
    
    g = RuzinoGraph("IcoUSDTest")
    g.loadConfiguration(os.path.join(binary_dir, "geometry_nodes.json"))
    
    # Create ico sphere
    sphere = g.createNode("create_ico_sphere", name="ico")
    write_node = g.createNode("write_usd", name="writer")
    
    # Connect
    g.addEdge(sphere, "Geometry", write_node, "Geometry")
    
    # Set up inputs
    inputs = {
        (sphere, "subdivisions"): 3,
        (sphere, "radius"): 1.0,
    }
    
    # Create Stage and convert to GeomPayload
    stage = stage_py.Stage(output_file)
    geom_payload = stage_py.create_payload_from_stage(stage, "/geom")
    g.setGlobalParams(geom_payload)
    
    # Execute
    g.prepare_and_execute(inputs, required_node=write_node)
    
    # Save the stage
    stage.save()
    
    # Verify
    assert os.path.exists(output_file), f"USD file not created: {output_file}"
    file_size = os.path.getsize(output_file)
    print(f"✓ USD file created: {output_file} ({file_size} bytes)")
    assert file_size > 1000, f"USD file seems empty: {file_size} bytes"
    print(f"✓ Ico sphere exported (use 'usdcat {output_file}' to inspect)")


if __name__ == "__main__":
    try:
        test_write_grid_to_usd()
        test_write_uv_sphere_to_usd()
        test_write_ico_sphere_to_usd()
        
        print("\n" + "="*70)
        print("  ALL USD EXPORT TESTS PASSED! 🎉")
        print("="*70)
        
        # List created files
        print("\n📁 Created USD files:")
        for f in ["test_grid.usdc", "test_sphere.usdc", "test_ico.usdc"]:
            if os.path.exists(f):
                size = os.path.getsize(f)
                print(f"  • {f} ({size} bytes)")
        
    except Exception as e:
        print(f"\n✗ TEST FAILED: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

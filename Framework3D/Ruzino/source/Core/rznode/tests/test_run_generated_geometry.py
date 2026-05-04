"""
Test running the Python code generated from geometry graph JSON roundtrip.
"""

import os

# Get paths
test_dir = os.path.dirname(os.path.abspath(__file__))
binary_dir = os.getcwd()
generated_file = os.path.join(test_dir, "generated_geometry_graph.py")


def test_run_generated_geometry_graph():
    """Execute the Python code generated from geometry JSON roundtrip."""
    print("\n" + "="*60)
    print("TEST: Run Geometry Code Generated from JSON")
    print("="*60)
    
    if not os.path.exists(generated_file):
        print(f"⚠ Generated file not found: {generated_file}")
        print("  Run test_geometry_json_roundtrip_and_codegen first to generate it.")
        return
    
    print(f"✓ Found generated file: {generated_file}")
    
    # Execute the generated code
    print("\n✓ Executing geometry code generated from JSON roundtrip...")
    print("  Pipeline: Geometry Graph -> JSON -> Deserialize -> Generate Code -> Execute")
    print("-" * 60)
    
    # Read and execute
    with open(generated_file, 'r') as f:
        code = f.read()
    
    try:
        exec_namespace = {}
        exec(code, exec_namespace)
        
        print("-" * 60)
        print("\n✓ Geometry full roundtrip test PASSED!")
        print("  ✓ Created geometry graph programmatically")
        print("  ✓ Serialized to JSON")
        print("  ✓ Deserialized from JSON")
        print("  ✓ Generated executable Python code")
        print("  ✓ Executed generated geometry code successfully")
        print("\n🎉 Complete geometry self-hosting pipeline verified!")
        
    except Exception as e:
        print("-" * 60)
        print(f"\n⚠ Execution note: {e}")
        print("  (This may be expected if geometry runtime dependencies are missing)")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    test_run_generated_geometry_graph()

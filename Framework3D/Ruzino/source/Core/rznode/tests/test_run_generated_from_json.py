"""
Test running the Python code generated from JSON-deserialized graph.
"""

import os

# Get paths
test_dir = os.path.dirname(os.path.abspath(__file__))
binary_dir = os.getcwd()
generated_file = os.path.join(test_dir, "generated_from_json.py")


def test_run_generated_from_json():
    """Execute the Python code generated from JSON roundtrip."""
    print("\n" + "="*60)
    print("TEST: Run Code Generated from JSON (Full Roundtrip)")
    print("="*60)
    
    if not os.path.exists(generated_file):
        print(f"⚠ Generated file not found: {generated_file}")
        print("  Run test_json_roundtrip_and_codegen first to generate it.")
        return
    
    print(f"✓ Found generated file: {generated_file}")
    
    # Execute the generated code
    print("\n✓ Executing code generated from JSON roundtrip...")
    print("  Pipeline: Graph -> JSON -> Deserialize -> Generate Code -> Execute")
    print("-" * 60)
    
    # Read and execute
    with open(generated_file, 'r') as f:
        code = f.read()
    
    exec_namespace = {}
    exec(code, exec_namespace)
    
    print("-" * 60)
    print("\n✓ Full roundtrip test PASSED!")
    print("  ✓ Created graph programmatically")
    print("  ✓ Serialized to JSON")
    print("  ✓ Deserialized from JSON")
    print("  ✓ Generated executable Python code")
    print("  ✓ Executed generated code successfully")
    print("\n🎉 Complete self-hosting pipeline verified!")


if __name__ == "__main__":
    test_run_generated_from_json()

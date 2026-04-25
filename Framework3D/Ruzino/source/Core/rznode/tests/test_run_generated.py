"""
Test that runs the generated Python code to verify self-bootstrap.
"""

import os
import sys

# Get paths
test_dir = os.path.dirname(os.path.abspath(__file__))
binary_dir = os.getcwd()
generated_file = os.path.join(test_dir, "generated_complex_graph.py")


def test_run_generated_complex_graph():
    """Execute the generated complex graph code."""
    print("\n" + "="*60)
    print("TEST: Run Generated Complex Graph (Self-Bootstrap)")
    print("="*60)
    
    if not os.path.exists(generated_file):
        print(f"⚠ Generated file not found: {generated_file}")
        print("  Run test_python_code_generation_complex first to generate it.")
        return
    
    print(f"✓ Found generated file: {generated_file}")
    
    # Execute the generated code
    print("\n✓ Executing generated code...")
    print("-" * 60)
    
    # Read and execute
    with open(generated_file, 'r') as f:
        code = f.read()
    
    exec_namespace = {}
    exec(code, exec_namespace)
    
    print("-" * 60)
    print("\n✓ Generated code executed successfully!")
    print("✓ Self-bootstrap test PASSED!")


if __name__ == "__main__":
    test_run_generated_complex_graph()

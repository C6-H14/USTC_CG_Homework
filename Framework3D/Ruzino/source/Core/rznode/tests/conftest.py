"""
Pytest configuration for RuzinoGraph tests.

This configuration ensures that:
1. The working directory is changed to Binaries/Debug so DLLs can be loaded
2. The binary directory is added to Python path for compiled modules
3. The python/ directory is added to Python path for ruzino_graph.py
"""

import sys
import os

# Set up the environment BEFORE pytest starts importing test modules
# This must be done at module level, not in a fixture

# Determine directories
# From rznode/tests/ to other directories
tests_dir = os.path.dirname(os.path.abspath(__file__))
rznode_dir = os.path.abspath(os.path.join(tests_dir, ".."))
python_dir = os.path.join(rznode_dir, "python")
binary_dir = os.path.join(tests_dir, "..", "..", "..", "..", "Binaries", "Release")
binary_dir = os.path.abspath(binary_dir)

# Save original directory
_original_cwd = os.getcwd()

# Change working directory to binary_dir
# This is required so that node DLLs can be loaded correctly
os.chdir(binary_dir)

# Add directories to Python path
# 1. Binary directory for compiled Python modules (nodes_core_py, nodes_system_py)
if binary_dir not in sys.path:
    sys.path.insert(0, binary_dir)

# 2. Python directory for ruzino_graph.py (no copying needed!)
if python_dir not in sys.path:
    sys.path.insert(0, python_dir)

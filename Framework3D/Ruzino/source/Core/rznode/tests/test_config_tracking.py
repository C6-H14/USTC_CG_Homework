"""Quick test to check if get_loaded_configs works"""
import os
import nodes_system_py as system

binary_dir = os.getcwd()

def test_get_loaded_configs():
    s = system.create_dynamic_loading_system()
    s.init()
    config_path = os.path.join(binary_dir, 'geometry_nodes.json')
    s.load_configuration(config_path)
    configs = s.get_loaded_configs()
    print(f"Loaded configs: {configs}")
    print(f"Type: {type(configs)}")
    assert 'geometry_nodes.json' in configs
    print("✓ get_loaded_configs works!")

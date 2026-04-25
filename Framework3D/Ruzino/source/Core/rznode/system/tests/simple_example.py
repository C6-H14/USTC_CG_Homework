#!/usr/bin/env python3
"""
简单的Node System使用示例
展示基本的系统创建、初始化和执行流程
"""

import sys

try:
    import nodes_system_py as system
    print("✓ Node system module imported successfully")
except ImportError as e:
    print(f"✗ Cannot import nodes_system_py: {e}")
    sys.exit(1)

def main():
    """基本使用示例"""
    print("=== Node System Basic Usage Example ===")
    
    # 创建动态加载系统
    print("\n1. Creating dynamic loading system...")
    node_system = system.create_dynamic_loading_system()
    print(f"   Created: {type(node_system).__name__}")
    
    try:
        # 初始化系统
        print("\n2. Initializing system...")
        node_system.init()
        print("   System initialized")
        
        # 获取节点树
        print("\n3. Getting node tree...")
        tree = node_system.get_node_tree()
        if tree:
            print("   Node tree obtained")
        else:
            print("   No node tree available")
        
        # 执行系统
        print("\n4. Executing system...")
        node_system.execute()
        print("   Execution completed")
        
        # 清理
        print("\n5. Finalizing...")
        node_system.finalize()
        print("   System finalized")
        
    except Exception as e:
        print(f"   Error: {e}")
        try:
            node_system.finalize()
        except:
            pass

if __name__ == '__main__':
    main()
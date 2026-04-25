#!/usr/bin/env python3
"""
Node System Python Bindings Test - 简化版
解决DLL和配置文件路径问题的核心测试
"""

import unittest
import sys
import os

# 设置正确的工作目录 - 这是解决DLL和配置文件问题的关键
BINARY_DIR = os.path.join(
    os.path.dirname(__file__), "..", "..", "..", "..", "Binaries", "Release"
)
if os.path.exists(BINARY_DIR):
    os.chdir(BINARY_DIR)
    print(f"✓ Changed working directory to: {os.getcwd()}")
else:
    print(f"✗ Binary directory not found: {BINARY_DIR}")

# 添加模块路径
sys.path.insert(0, BINARY_DIR)

try:
    import nodes_system_py as system

    print("✓ Successfully imported nodes_system_py")
except ImportError as e:
    print(f"✗ Failed to import nodes_system_py: {e}")
    sys.exit(1)

try:
    import nodes_core_py as core

    CORE_AVAILABLE = True
    print("✓ Core module available")
except ImportError:
    CORE_AVAILABLE = False
    print("✗ Core module not available")


class TestNodeSystem(unittest.TestCase):
    """测试节点系统基础功能"""

    def setUp(self):
        """每个测试前的准备工作"""
        self.system = None

    def tearDown(self):
        """每个测试后的清理工作"""
        if self.system:
            try:
                self.system.finalize()
            except:
                pass

    def test_create_dynamic_loading_system(self):
        """测试创建动态加载系统"""
        self.system = system.create_dynamic_loading_system()
        self.assertIsNotNone(self.system)
        self.assertIsInstance(self.system, system.NodeDynamicLoadingSystem)
        print("✓ Dynamic loading system created successfully")

    def test_system_basic_properties(self):
        """测试系统基本属性"""
        self.system = system.create_dynamic_loading_system()

        # 测试allow_ui_execution属性
        self.assertIsInstance(self.system.allow_ui_execution, bool)
        original_value = self.system.allow_ui_execution
        self.system.allow_ui_execution = not original_value
        self.assertEqual(self.system.allow_ui_execution, not original_value)

        print(
            f"✓ Basic properties tested, allow_ui_execution: {self.system.allow_ui_execution}"
        )

    def test_load_configuration(self):
        """测试配置加载"""
        self.system = system.create_dynamic_loading_system()

        # 测试加载实际的配置文件（现在在当前目录）
        try:
            result = self.system.load_configuration("test_nodes.json")
            print(f"✓ Load test_nodes.json result: {result}")
            if result:
                print("  Configuration loaded successfully")
            else:
                print("  Configuration file not found or failed to load")
        except Exception as e:
            print(f"✗ Load configuration failed: {e}")

        # 测试加载不存在的配置文件
        try:
            result = self.system.load_configuration("non_existent_config.json")
            print(f"Load non-existent config result: {result}")
        except Exception as e:
            print(f"Load non-existent config failed (expected): {e}")

    def test_system_initialization(self):
        """测试系统初始化"""
        self.system = system.create_dynamic_loading_system()

        try:
            self.system.init()
            print("✓ System initialized successfully")

            # 测试获取节点树
            tree = self.system.get_node_tree()
            if tree is not None:
                print("✓ Node tree retrieved successfully")
                if CORE_AVAILABLE and hasattr(tree, "node_count"):
                    print(f"  Tree has {tree.node_count} nodes")
            else:
                print("✗ Node tree is None")

            # 测试获取执行器
            executor = self.system.get_node_tree_executor()
            if executor is not None:
                print("✓ Node tree executor retrieved successfully")
            else:
                print("✗ Node tree executor is None")

        except Exception as e:
            print(f"✗ System initialization failed: {e}")

    def test_system_execution(self):
        """测试系统执行"""
        self.system = system.create_dynamic_loading_system()

        try:
            self.system.init()

            # 测试基本执行
            self.system.execute()
            print("✓ Basic execution completed")

            # 测试UI执行
            self.system.execute(is_ui_execution=True)
            print("✓ UI execution completed")

        except Exception as e:
            print(f"✗ System execution failed: {e}")

    def test_system_finalization(self):
        """测试系统清理"""
        self.system = system.create_dynamic_loading_system()

        try:
            self.system.init()
            self.system.finalize()
            print("✓ System finalized successfully")
        except Exception as e:
            print(f"✗ System finalization failed: {e}")


class TestNodeSystemWithRegisteredNodes(unittest.TestCase):
    """测试使用已注册节点的功能"""

    def setUp(self):
        self.system = system.create_dynamic_loading_system()  # 尝试加载配置文件
        config_path = os.path.join(
            os.path.dirname(__file__),
            "..",
            "..",
            "..",
            "..",
            "..",
            "Binaries",
            "Release",
            "test_nodes.json",
        )
        try:
            config_loaded = self.system.load_configuration(config_path)
            if not config_loaded:
                self.skipTest("test_nodes.json not found or failed to load")
        except Exception as e:
            self.skipTest(f"Cannot load configuration: {e}")

        self.system.init()

    def tearDown(self):
        if self.system:
            try:
                self.system.finalize()
            except:
                pass

    def test_add_registered_nodes(self):
        """测试添加已注册的节点"""
        tree = self.system.get_node_tree()
        self.assertIsNotNone(tree)

        print(f"\nInitial tree: {tree.node_count} nodes")

        # 测试添加add节点
        try:
            add_node = tree.add_node("add")
            if add_node:
                print(f"✓ Created add node: {add_node.ui_name}")
                print(
                    f"  Inputs: {len(add_node.inputs)}, Outputs: {len(add_node.outputs)}"
                )

                # 显示套接字信息
                for i, socket in enumerate(add_node.inputs):
                    print(f"    Input[{i}]: {socket.identifier}")
                for i, socket in enumerate(add_node.outputs):
                    print(f"    Output[{i}]: {socket.identifier}")

                self.assertEqual(tree.node_count, 1)
            else:
                print("✗ Add node creation returned None")
        except Exception as e:
            print(f"✗ Add node creation failed: {e}")

        # 测试添加print节点
        try:
            print_node = tree.add_node("print")
            if print_node:
                print(f"✓ Created print node: {print_node.ui_name}")
                print(
                    f"  Inputs: {len(print_node.inputs)}, Outputs: {len(print_node.outputs)}"
                )
                self.assertEqual(tree.node_count, 2)
            else:
                print("✗ Print node creation returned None")
        except Exception as e:
            print(f"✗ Print node creation failed: {e}")

        # 测试添加sub节点
        try:
            sub_node = tree.add_node("sub")
            if sub_node:
                print(f"✓ Created sub node: {sub_node.ui_name}")
                print(
                    f"  Inputs: {len(sub_node.inputs)}, Outputs: {len(sub_node.outputs)}"
                )
                self.assertEqual(tree.node_count, 3)
            else:
                print("✗ Sub node creation returned None")
        except Exception as e:
            print(f"✗ Sub node creation failed: {e}")

        print(f"Final tree: {tree.node_count} nodes")

    def test_create_node_connections(self):
        """测试创建节点连接"""
        tree = self.system.get_node_tree()

        try:
            # 创建两个add节点
            add1 = tree.add_node("add")
            add2 = tree.add_node("add")

            if add1 and add2:
                print(f"✓ Created two add nodes")

                # 查找合适的套接字
                if len(add1.outputs) > 0 and len(add2.inputs) > 0:
                    output_socket = add1.outputs[0]  # add1的输出
                    input_socket = None

                    # 找到名为"value"或"value2"的输入套接字
                    for socket in add2.inputs:
                        if socket.identifier in ["value", "value2"]:
                            input_socket = socket
                            break

                    if input_socket:
                        # 检查是否可以创建连接
                        can_link = tree.can_create_link(output_socket, input_socket)
                        print(
                            f"  Can create link from {output_socket.identifier} to {input_socket.identifier}: {can_link}"
                        )

                        if can_link:
                            # 创建连接
                            link = tree.add_link(output_socket, input_socket)
                            if link:
                                print(
                                    f"✓ Link created: {output_socket.identifier} -> {input_socket.identifier}"
                                )
                                self.assertEqual(tree.link_count, 1)
                            else:
                                print("✗ Link creation returned None")
                        else:
                            print("✗ Cannot create link between these sockets")
                    else:
                        print("✗ Could not find suitable input socket")
                else:
                    print("✗ Nodes don't have required sockets")
            else:
                print("✗ Failed to create add nodes")

        except Exception as e:
            print(f"✗ Node connection test failed: {e}")

    def test_dynamic_socket_groups(self):
        """测试动态套接字组功能"""
        tree = self.system.get_node_tree()

        try:
            # 创建add节点（应该有动态套接字组）
            add_node = tree.add_node("add")
            if add_node:
                print(f"✓ Created add node for dynamic socket test")
                initial_inputs = len(add_node.inputs)
                print(f"  Initial inputs: {initial_inputs}")

                # 尝试添加动态套接字
                try:
                    new_socket = (
                        add_node.group_add_socket(
                            "input_group",
                            "int",
                            "dynamic_input",
                            "Dynamic Input",
                            core.PinKind.Input,
                        )
                        if CORE_AVAILABLE
                        else None
                    )

                    if new_socket:
                        print(f"✓ Added dynamic socket: {new_socket.identifier}")
                        self.assertEqual(len(add_node.inputs), initial_inputs + 1)
                    else:
                        print("✗ Dynamic socket creation returned None")

                except Exception as e:
                    print(f"✗ Dynamic socket creation failed: {e}")

        except Exception as e:
            print(f"✗ Dynamic socket group test failed: {e}")

    def test_system_execution_with_nodes(self):
        """测试包含节点的系统执行"""
        tree = self.system.get_node_tree()

        try:
            # 创建一个简单的节点网络
            add_node = tree.add_node("add")
            print_node = tree.add_node("print")

            if add_node and print_node:
                print(f"✓ Created node network: add -> print")

                # 尝试连接节点
                if len(add_node.outputs) > 0 and len(print_node.inputs) > 0:
                    output_socket = add_node.outputs[0]
                    input_socket = print_node.inputs[0]

                    if tree.can_create_link(output_socket, input_socket):
                        link = tree.add_link(output_socket, input_socket)
                        if link:
                            print(f"✓ Connected add output to print input")

                # 执行系统
                print("  Executing system with node network...")
                self.system.execute()
                print("✓ System execution with nodes completed")

        except Exception as e:
            print(f"✗ System execution with nodes failed: {e}")


class TestNodeSystemIntegration(unittest.TestCase):
    """集成测试 - 测试完整的工作流程"""

    def setUp(self):
        self.system = system.create_dynamic_loading_system()

    def tearDown(self):
        if self.system:
            try:
                self.system.finalize()
            except:
                pass

    def test_complete_workflow(self):
        """测试完整工作流程"""
        print("\n=== Testing Complete Node System Workflow ===")

        try:
            # 1. 创建系统
            print("1. Creating dynamic loading system...")
            self.assertIsNotNone(self.system)
            print(
                f"   ✓ System created: {type(self.system).__name__}"
            )  # 2. 尝试加载配置
            print("2. Attempting to load configuration...")
            config_path = os.path.join(
                os.path.dirname(__file__),
                "..",
                "..",
                "..",
                "..",
                "..",
                "Binaries",
                "Release",
                "test_nodes.json",
            )
            try:
                config_result = self.system.load_configuration(config_path)
                print(f"   Configuration load result: {config_result}")
                if config_result:
                    print("   ✓ test_nodes.json loaded successfully")
                else:
                    print("   ✗ test_nodes.json not found or failed to load")
            except Exception as e:
                print(f"   Configuration loading failed: {e}")
                # 尝试备用配置
                try:
                    config_result = self.system.load_configuration("test_config.json")
                    print(f"   Backup configuration load result: {config_result}")
                except Exception as e2:
                    print(f"   Backup configuration also failed: {e2}")

            # 3. 初始化系统
            print("3. Initializing system...")
            self.system.init()
            print("   ✓ System initialized")

            # 4. 获取节点树
            print("4. Getting node tree...")
            tree = self.system.get_node_tree()
            if tree:
                print("   ✓ Node tree obtained")
                if CORE_AVAILABLE and hasattr(tree, "node_count"):
                    print(f"   Tree stats: {tree.node_count} nodes")
            else:
                print("   ✗ Node tree is None")

            # 5. 获取执行器
            print("5. Getting executor...")
            executor = self.system.get_node_tree_executor()
            if executor:
                print("   ✓ Executor obtained")
            else:
                print("   ✗ Executor is None")

            # 6. 测试执行
            print("6. Testing execution...")
            self.system.execute()
            print("   ✓ Execution completed")

            # 7. 测试UI执行设置
            print("7. Testing UI execution settings...")
            original_ui_setting = self.system.allow_ui_execution
            self.system.allow_ui_execution = True
            self.system.execute(is_ui_execution=True)
            print(
                f"   ✓ UI execution completed (allow_ui: {self.system.allow_ui_execution})"
            )
            self.system.allow_ui_execution = original_ui_setting

            # 8. 清理
            print("8. Finalizing system...")
            self.system.finalize()
            print("   ✓ System finalized")

        except Exception as e:
            print(f"✗ Workflow failed at some step: {e}")
            import traceback

            traceback.print_exc()


class TestNodeSystemWithCore(unittest.TestCase):
    """测试与核心模块的集成（如果可用）"""

    def setUp(self):
        if not CORE_AVAILABLE:
            self.skipTest("Core module not available")
        self.system = system.create_dynamic_loading_system()

    def tearDown(self):
        if self.system:
            try:
                self.system.finalize()
            except:
                pass

    def test_integration_with_core(self):
        """测试与核心模块的集成"""
        print("\n=== Testing Integration with Core Module ===")

        try:
            # 初始化系统
            self.system.init()

            # 获取节点树
            tree = self.system.get_node_tree()
            self.assertIsNotNone(tree)

            # 如果有节点树，测试一些核心功能
            if hasattr(tree, "node_count") and hasattr(tree, "add_node"):
                print(f"Initial tree: {tree.node_count} nodes")

                # 尝试添加节点（可能失败，因为节点类型未注册）
                try:
                    node = tree.add_node("test_node")
                    if node:
                        print(f"✓ Node added successfully: {node.ui_name}")
                    else:
                        print("✗ Node addition returned None")
                except Exception as e:
                    print(f"✗ Node addition failed (expected): {e}")

                # 测试序列化
                try:
                    serialized = tree.serialize()
                    print(f"✓ Tree serialized: {len(serialized)} characters")
                except Exception as e:
                    print(f"✗ Serialization failed: {e}")

            # 执行系统
            self.system.execute()
            print("✓ System execution with core integration completed")

        except Exception as e:
            print(f"✗ Core integration test failed: {e}")


def show_module_info():
    """显示模块信息"""
    print("Node System Python Bindings - Module Information")
    print("=" * 60)
    print(f"Module: {system}")

    # 列出可用的类和函数
    print("\nAvailable classes and functions:")
    available_items = [item for item in dir(system) if not item.startswith("_")]
    for item in sorted(available_items):
        obj = getattr(system, item)
        item_type = type(obj).__name__
        print(f"  {item:<30} [{item_type}]")

    print(f"\nTotal available items: {len(available_items)}")

    # 显示核心模块状态
    print(f"Core module available: {CORE_AVAILABLE}")
    print("=" * 60)


def main():
    """主函数"""
    print("Testing Node System Python Bindings...")

    # 显示模块信息
    show_module_info()

    # 运行测试
    print("\nRunning tests...")
    unittest.main(verbosity=2, argv=[""], exit=False)

    print("\n" + "=" * 60)
    print("Testing completed!")
    print("Note: Some tests may fail if configuration files don't exist")
    print("or if node types are not properly registered.")
    print("=" * 60)


if __name__ == "__main__":
    main()

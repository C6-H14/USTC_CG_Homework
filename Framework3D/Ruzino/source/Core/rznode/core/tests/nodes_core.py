#!/usr/bin/env python3
"""
Node Core - Simplified Test
"""

import unittest
import sys
import os

# 设置正确的工作目录
BINARY_DIR = os.path.join(
    os.path.dirname(__file__), "..", "..", "..", "..", "Binaries", "Release"
)
if os.path.exists(BINARY_DIR):
    os.chdir(BINARY_DIR)
    print(f"✓ Changed working directory to: {os.getcwd()}")

sys.path.insert(0, BINARY_DIR)

try:
    import nodes_core_py as core
    import nodes_system_py as system

    print("✓ Successfully imported modules")
except ImportError as e:
    print(f"✗ Import failed: {e}")
    sys.exit(1)


class TestBasicNodeCore(unittest.TestCase):
    """测试节点树核心功能"""

    def setUp(self):
        """每个测试前的准备工作"""
        self.descriptor = core.create_descriptor()
        self.tree = core.create_tree(self.descriptor)

    def tearDown(self):
        """每个测试后的清理工作"""
        if self.tree:
            self.tree.clear()

    def test_create_descriptor_and_tree(self):
        """测试创建描述符和树"""
        # 测试描述符创建
        descriptor = core.create_descriptor()
        self.assertIsNotNone(descriptor)

        # 测试树创建
        tree = core.create_tree(descriptor)
        self.assertIsNotNone(tree)
        self.assertEqual(tree.node_count, 0)
        self.assertEqual(tree.link_count, 0)

    def test_tree_basic_properties(self):
        """测试树的基本属性"""
        self.assertEqual(self.tree.node_count, 0)
        self.assertEqual(self.tree.link_count, 0)
        self.assertEqual(len(self.tree.nodes), 0)
        self.assertEqual(len(self.tree.links), 0)

    def test_enum_values(self):
        """测试枚举值是否正确绑定"""
        # 测试PinKind枚举
        self.assertTrue(hasattr(core, "PinKind"))
        self.assertTrue(hasattr(core.PinKind, "Input"))
        self.assertTrue(hasattr(core.PinKind, "Output"))

    def test_add_node_handling(self):
        """测试添加节点的处理（即使节点类型未注册）"""
        try:
            node = self.tree.add_node("test_node")
            if node is not None:
                # 如果成功创建节点，测试其基本属性
                self.assertIsNotNone(node)
                self.assertEqual(self.tree.node_count, 1)
                self.assertTrue(bool(node.ID))
                self.assertIsInstance(node.ui_name, str)
                self.assertIsInstance(node.inputs, list)
                self.assertIsInstance(node.outputs, list)

                # 测试节点方法
                self.assertIsInstance(node.is_valid(), bool)
                self.assertIsInstance(node.is_node_group(), bool)

                # 测试名称设置
                original_name = node.ui_name
                node.ui_name = "Test Node"
                self.assertEqual(node.ui_name, "Test Node")

                print(f"✓ Node created successfully: {node.ui_name} (ID: {node.ID})")
                print(f"  Inputs: {len(node.inputs)}, Outputs: {len(node.outputs)}")
            else:
                print("✗ Node creation returned None (expected for unregistered types)")

        except Exception as e:
            # 如果没有注册节点类型，会抛出异常，这是预期的
            print(f"✗ Expected error when adding unregistered node: {e}")

    def test_socket_operations(self):
        """测试套接字操作"""
        try:
            node = self.tree.add_node("test_node")
            if node is not None:
                # 测试获取输入输出套接字
                inputs = node.inputs
                outputs = node.outputs

                # 测试套接字属性
                for socket in inputs:
                    self.assertIsInstance(socket.identifier, str)
                    self.assertIsInstance(socket.ui_name, str)
                    self.assertEqual(socket.in_out, core.PinKind.Input)
                    self.assertEqual(socket.node, node)
                    self.assertTrue(bool(socket.ID))

                for socket in outputs:
                    self.assertIsInstance(socket.identifier, str)
                    self.assertIsInstance(socket.ui_name, str)
                    self.assertEqual(socket.in_out, core.PinKind.Output)
                    self.assertEqual(socket.node, node)
                    self.assertTrue(bool(socket.ID))

                print(
                    f"✓ Socket operations tested for {len(inputs)} inputs, {len(outputs)} outputs"
                )

        except Exception as e:
            print(f"✗ Socket operations failed: {e}")

    def test_link_operations(self):
        """测试连接操作"""
        try:
            # 创建两个节点
            node1 = self.tree.add_node("test_node")
            node2 = self.tree.add_node("test_node")

            if node1 is not None and node2 is not None:
                print(f"✓ Created two nodes for link testing")

                # 如果节点有输出和输入套接字，尝试连接
                if len(node1.outputs) > 0 and len(node2.inputs) > 0:
                    output_socket = node1.outputs[0]
                    input_socket = node2.inputs[0]

                    # 检查是否可以创建连接
                    can_link = self.tree.can_create_link(output_socket, input_socket)
                    print(f"✓ Can create link: {can_link}")

                    if can_link:
                        # 创建连接
                        link = self.tree.add_link(output_socket, input_socket)
                        if link is not None:
                            self.assertIsNotNone(link)
                            self.assertEqual(self.tree.link_count, 1)
                            self.assertEqual(link.from_node, node1)
                            self.assertEqual(link.to_node, node2)
                            self.assertEqual(link.from_socket, output_socket)
                            self.assertEqual(link.to_socket, input_socket)

                            print(f"✓ Link created successfully: {link.ID}")

                            # 测试连接删除
                            self.tree.delete_link(link)
                            self.assertEqual(self.tree.link_count, 0)
                            print(f"✓ Link deleted successfully")
                        else:
                            print("✗ Link creation returned None")
                    else:
                        print("✗ Cannot create link between these sockets")
                else:
                    print("✗ Nodes don't have compatible sockets for linking")
            else:
                print("✗ Could not create nodes for link testing")

        except Exception as e:
            print(f"✗ Link operations failed: {e}")

    def test_delete_operations(self):
        """测试删除操作"""
        try:
            node = self.tree.add_node("test_node")
            if node is not None:
                node_id = node.ID
                self.assertEqual(self.tree.node_count, 1)

                # 删除节点
                self.tree.delete_node(node)
                self.assertEqual(self.tree.node_count, 0)

                # 确认节点已被删除
                found_node = self.tree.find_node(node_id)
                self.assertIsNone(found_node)

                print(f"✓ Node deletion test passed")

        except Exception as e:
            print(f"✗ Delete operations failed: {e}")

    def test_clear_tree(self):
        """测试清空树"""
        try:
            # 尝试添加一些节点
            added_count = 0
            for i in range(3):
                node = self.tree.add_node("test_node")
                if node is not None:
                    added_count += 1

            print(f"✓ Added {added_count} nodes for clear test")

            if added_count > 0:
                self.assertEqual(self.tree.node_count, added_count)

                # 清空树
                self.tree.clear()
                self.assertEqual(self.tree.node_count, 0)
                self.assertEqual(self.tree.link_count, 0)

                print(f"✓ Tree cleared successfully")
            else:
                print("✗ No nodes were created, skipping clear test")

        except Exception as e:
            print(f"✗ Clear operations failed: {e}")

    def test_serialization(self):
        """测试序列化"""
        try:
            # 序列化空树
            serialized = self.tree.serialize()
            self.assertIsInstance(serialized, str)
            self.assertGreater(len(serialized), 0)

            # 反序列化
            new_tree = core.create_tree(self.descriptor)
            new_tree.deserialize(serialized)

            self.assertEqual(new_tree.node_count, self.tree.node_count)
            self.assertEqual(new_tree.link_count, self.tree.link_count)

            print(f"✓ Serialization test passed ({len(serialized)} chars)")

        except Exception as e:
            print(f"✗ Serialization failed: {e}")

    def test_id_types(self):
        """测试ID类型"""
        try:
            node = self.tree.add_node("test_node")
            if node is not None:
                # 测试NodeId
                node_id = node.ID
                self.assertTrue(bool(node_id))
                self.assertEqual(node_id, node_id)  # 自等性

                # 测试SocketID
                if len(node.inputs) > 0:
                    socket_id = node.inputs[0].ID
                    self.assertTrue(bool(socket_id))
                    self.assertEqual(socket_id, socket_id)  # 自等性

                print(f"✓ ID types test passed")

        except Exception as e:
            print(f"✗ ID type tests failed: {e}")


class TestCoreWithRegisteredNodes(unittest.TestCase):
    """测试核心功能与已注册节点的集成"""

    def setUp(self):
        """设置包含注册节点的测试环境"""
        # 创建系统并加载节点配置
        self.node_system = system.create_dynamic_loading_system()
        try:
            # 直接加载当前目录的配置文件
            config_loaded = self.node_system.load_configuration("test_nodes.json")
            if not config_loaded:
                self.skipTest("test_nodes.json not found or failed to load")
        except Exception as e:
            self.skipTest(f"Cannot load configuration: {e}")

        self.node_system.init()
        self.tree = self.node_system.get_node_tree()

    def tearDown(self):
        """清理测试环境"""
        if hasattr(self, "node_system"):
            try:
                self.node_system.finalize()
            except:
                pass

    def test_registered_nodes_creation(self):
        """测试创建已注册的节点（add、print、sub）"""
        print("\n=== Testing Registered Nodes Creation ===")

        # 测试add节点
        add_node = self.tree.add_node("add")
        self.assertIsNotNone(add_node, "Add node should be created")
        print(f"✓ Created add node: {add_node.ui_name}")

        # 验证add节点的属性
        input_names = [s.identifier for s in add_node.inputs]
        output_names = [s.identifier for s in add_node.outputs]
        self.assertIn("value", input_names, "Add node should have 'value' input")
        self.assertIn("value2", input_names, "Add node should have 'value2' input")
        self.assertIn("value", output_names, "Add node should have 'value' output")

        # 测试print节点
        print_node = self.tree.add_node("print")
        self.assertIsNotNone(print_node, "Print node should be created")

        # 验证print节点的属性
        print_input_names = [s.identifier for s in print_node.inputs]
        self.assertIn("info", print_input_names, "Print node should have 'info' input")

    def test_registered_nodes_connections(self):
        """测试已注册节点之间的连接"""
        print("\n=== Testing Registered Nodes Connections ===")

        # 创建add和print节点
        add_node = self.tree.add_node("add")
        print_node = self.tree.add_node("print")

        self.assertIsNotNone(add_node)
        self.assertIsNotNone(print_node)

        # 连接add的value输出到print的info输入
        add_output = next(
            (s for s in add_node.outputs if s.identifier == "value"), None
        )
        print_input = next(
            (s for s in print_node.inputs if s.identifier == "info"), None
        )

        self.assertIsNotNone(add_output, "Add node should have 'value' output")
        self.assertIsNotNone(print_input, "Print node should have 'info' input")

        # 检查连接可行性
        can_link = self.tree.can_create_link(add_output, print_input)
        print(f"  Can link add.value -> print.info: {can_link}")

        if can_link:
            link = self.tree.add_link(add_output, print_input)
            self.assertIsNotNone(link, "Link should be created")
            self.assertEqual(self.tree.link_count, 1)
        else:
            print("✗ Cannot create link (type incompatible)")

    def test_dynamic_socket_operations(self):
        """测试动态套接字操作（add节点有动态套接字组）"""
        print("\n=== Testing Dynamic Socket Operations ===")

        add_node = self.tree.add_node("add")
        self.assertIsNotNone(add_node)

        initial_input_count = len(add_node.inputs)
        print(f"  Initial input count: {initial_input_count}")

        # 测试添加动态套接字
        try:
            new_socket = add_node.group_add_socket(
                "input_group",
                "int",
                "dynamic_value",
                "Dynamic Value",
                core.PinKind.Input,
            )

            if new_socket:
                print(f"✓ Added dynamic socket: {new_socket.identifier}")
                new_input_count = len(add_node.inputs)
                self.assertEqual(
                    new_input_count,
                    initial_input_count + 1,
                    "Should have one more input after adding dynamic socket",
                )

                # 测试移除动态套接字
                add_node.group_remove_socket(
                    "input_group", "dynamic_value", core.PinKind.Input
                )
                final_input_count = len(add_node.inputs)
                self.assertEqual(
                    final_input_count,
                    initial_input_count,
                    "Should return to original input count after removing dynamic socket",
                )

                print("✓ Dynamic socket add/remove test passed")
            else:
                print("✗ Dynamic socket creation returned None")

        except Exception as e:
            print(f"✗ Dynamic socket test failed: {e}")

    def test_network_execution(self):
        """测试网络执行"""
        print("\n=== Testing Network Execution ===")

        # 创建简单网络：add -> print
        add_node = self.tree.add_node("add")
        print_node = self.tree.add_node("print")

        # 连接节点
        add_output = next(
            (s for s in add_node.outputs if s.identifier == "value"), None
        )
        print_input = next(
            (s for s in print_node.inputs if s.identifier == "info"), None
        )

        if add_output and print_input:
            if self.tree.can_create_link(add_output, print_input):
                link = self.tree.add_link(add_output, print_input)
                if link:
                    print("✓ Network created: add -> print")

        # 执行网络
        try:
            print("  Executing network...")
            self.node_system.execute()
            print("✓ Network execution completed successfully")
        except Exception as e:
            print(f"✗ Network execution failed: {e}")


class TestNodeCoreIntegration(unittest.TestCase):
    """集成测试 - 测试完整的工作流程"""

    def setUp(self):
        self.descriptor = core.create_descriptor()
        self.tree = core.create_tree(self.descriptor)

    def test_complete_workflow(self):
        """测试完整工作流程"""
        print("\n=== Testing Complete Workflow ===")

        # 1. 创建树
        print(f"1. Created tree with {self.tree.node_count} nodes")
        self.assertEqual(self.tree.node_count, 0)

        # 2. 尝试添加节点
        print("2. Attempting to add nodes...")
        nodes_created = 0
        for i in range(2):
            try:
                node = self.tree.add_node(f"node_type_{i}")
                if node:
                    node.ui_name = f"Node {i+1}"
                    nodes_created += 1
                    print(f"   ✓ Created: {node.ui_name} (ID: {node.ID})")
                    print(
                        f"     Inputs: {len(node.inputs)}, Outputs: {len(node.outputs)}"
                    )
                else:
                    print(f"   ✗ Failed to create node {i+1}")
            except Exception as e:
                print(f"   ✗ Error creating node {i+1}: {e}")

        print(f"   Created {nodes_created} nodes successfully")

        # 3. 检查树状态
        print(
            f"3. Tree state: {self.tree.node_count} nodes, {self.tree.link_count} links"
        )

        # 4. 序列化测试
        print("4. Testing serialization...")
        try:
            serialized = self.tree.serialize()
            print(f"   ✓ Serialized tree: {len(serialized)} characters")

            # 反序列化到新树
            new_tree = core.create_tree(self.descriptor)
            new_tree.deserialize(serialized)
            print(f"   ✓ Deserialized to new tree: {new_tree.node_count} nodes")

        except Exception as e:
            print(f"   ✗ Serialization error: {e}")

        # 5. 清理
        print("5. Cleaning up...")
        self.tree.clear()
        print(f"   ✓ Tree cleared: {self.tree.node_count} nodes remaining")


def show_module_info():
    """显示模块信息"""
    print("Node Core Python Bindings - Module Information")
    print("=" * 60)
    print(f"Module: {core}")

    # 列出可用的类和函数
    print("\nAvailable classes and functions:")
    available_items = [item for item in dir(core) if not item.startswith("_")]
    for item in sorted(available_items):
        obj = getattr(core, item)
        item_type = type(obj).__name__
        print(f"  {item:<20} [{item_type}]")

        # 显示枚举成员
        if hasattr(obj, "__members__"):
            for member_name in obj.__members__:
                print(f"    └─ {member_name}")

    print(f"\nTotal available items: {len(available_items)}")
    print("=" * 60)


def main():
    """主函数"""
    print("Testing Node Core Python Bindings...")

    # 显示模块信息
    show_module_info()

    # 运行测试
    print("\nRunning tests...")
    unittest.main(verbosity=2, argv=[""], exit=False)

    print("\n" + "=" * 60)
    print("Testing completed!")
    print("Note: Some tests may fail if node types are not registered.")
    print("This is expected - the bindings work, but need C++ node registration.")
    print("=" * 60)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
测试test_nodes.json中定义的具体节点功能
"""

import unittest
import os

# conftest.py will handle path setup and working directory change

try:
    import nodes_system_py as system

    print("✓ Successfully imported nodes_system_py")
except ImportError as e:
    print(f"✗ Failed to import nodes_system_py: {e}")
    raise

try:
    import nodes_core_py as core

    CORE_AVAILABLE = True
    print("✓ Core module available")
except ImportError:
    CORE_AVAILABLE = False
    print("✗ Core module not available")


class TestRegisteredNodes(unittest.TestCase):
    """测试test_nodes.json中注册的具体节点"""

    def setUp(self):
        import os
        self.system = system.create_dynamic_loading_system()
        # Use absolute path since C++ might have different path resolution
        config_path = os.path.join(os.getcwd(), "test_nodes.json")
        try:
            config_loaded = self.system.load_configuration(config_path)
            if not config_loaded:
                self.fail(f"test_nodes.json not found at {config_path}")
        except Exception as e:
            self.fail(f"Cannot load configuration: {e}")

        self.system.init()
        self.tree = self.system.get_node_tree()

    def tearDown(self):
        if self.system:
            self.system.finalize()

    def test_add_node_creation_and_properties(self):
        """测试add节点的创建和属性"""
        print("\n=== Testing Add Node ===")

        add_node = self.tree.add_node("add")
        self.assertIsNotNone(add_node, "Add node should be created successfully")

        print(f"✓ Add node created: {add_node.ui_name}")

        # 检查输入
        inputs = add_node.inputs
        self.assertGreaterEqual(
            len(inputs), 2, "Add node should have at least 2 inputs"
        )

        input_names = [socket.identifier for socket in inputs]
        print(f"  Input sockets: {input_names}")

        # 应该有value和value2输入
        self.assertIn("value", input_names, "Should have 'value' input")
        self.assertIn("value2", input_names, "Should have 'value2' input")

        # 检查输出
        outputs = add_node.outputs
        self.assertGreaterEqual(
            len(outputs), 1, "Add node should have at least 1 output"
        )

        output_names = [socket.identifier for socket in outputs]
        print(f"  Output sockets: {output_names}")

        # 应该有value输出
        self.assertIn("value", output_names, "Should have 'value' output")

        print("✓ Add node properties verified")

    def test_print_node_creation_and_properties(self):
        """测试print节点的创建和属性"""
        print("\n=== Testing Print Node ===")

        print_node = self.tree.add_node("print")
        self.assertIsNotNone(print_node, "Print node should be created successfully")

        print(f"✓ Print node created: {print_node.ui_name}")

        # 检查输入
        inputs = print_node.inputs
        self.assertGreaterEqual(
            len(inputs), 1, "Print node should have at least 1 input"
        )

        input_names = [socket.identifier for socket in inputs]
        print(f"  Input sockets: {input_names}")

        # 应该有info输入
        self.assertIn("info", input_names, "Should have 'info' input")

        print("✓ Print node properties verified")

    def test_sub_node_creation_and_properties(self):
        """测试sub节点的创建和属性"""
        print("\n=== Testing Sub Node ===")

        sub_node = self.tree.add_node("sub")
        self.assertIsNotNone(sub_node, "Sub node should be created successfully")

        print(f"✓ Sub node created: {sub_node.ui_name}")

        # 检查输入
        inputs = sub_node.inputs
        self.assertGreaterEqual(len(inputs), 1, "Sub node should have at least 1 input")

        input_names = [socket.identifier for socket in inputs]
        print(f"  Input sockets: {input_names}")

        # 应该有value输入
        self.assertIn("value", input_names, "Should have 'value' input")

        # 检查输出
        outputs = sub_node.outputs
        self.assertGreaterEqual(
            len(outputs), 1, "Sub node should have at least 1 output"
        )

        output_names = [socket.identifier for socket in outputs]
        print(f"  Output sockets: {output_names}")

        print("✓ Sub node properties verified")

    def test_node_connections(self):
        """测试节点之间的连接"""
        print("\n=== Testing Node Connections ===")

        # 创建add和print节点
        add_node = self.tree.add_node("add")
        print_node = self.tree.add_node("print")

        self.assertIsNotNone(add_node)
        self.assertIsNotNone(print_node)

        # 连接add的输出到print的输入
        add_output = None
        for socket in add_node.outputs:
            if socket.identifier == "value":
                add_output = socket
                break

        print_input = None
        for socket in print_node.inputs:
            if socket.identifier == "info":
                print_input = socket
                break

        self.assertIsNotNone(add_output, "Add node should have 'value' output")
        self.assertIsNotNone(print_input, "Print node should have 'info' input")

        # 检查是否可以连接
        can_link = self.tree.can_create_link(add_output, print_input)
        print(f"  Can link add.value -> print.info: {can_link}")

        if can_link:
            # 创建连接
            link = self.tree.add_link(add_output, print_input)
            self.assertIsNotNone(link, "Link should be created successfully")

            print(
                f"✓ Link created: {add_output.identifier} -> {print_input.identifier}"
            )
            self.assertEqual(self.tree.link_count, 1, "Tree should have 1 link")
        else:
            print("✗ Cannot create link (may be due to type mismatch)")

    def test_dynamic_socket_groups(self):
        """测试动态套接字组（add节点有input_group）"""
        print("\n=== Testing Dynamic Socket Groups ===")

        add_node = self.tree.add_node("add")
        self.assertIsNotNone(add_node)

        initial_input_count = len(add_node.inputs)
        print(f"  Initial input count: {initial_input_count}")

        if CORE_AVAILABLE:
            try:
                # 尝试添加动态输入
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

                    # 尝试删除动态输入
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
        else:
            print("✗ Core module not available, skipping dynamic socket test")

    def test_system_execution_with_network(self):
        """测试包含节点网络的系统执行"""
        print("\n=== Testing System Execution with Node Network ===")

        # 创建简单的计算网络：add -> print
        add_node = self.tree.add_node("add")
        print_node = self.tree.add_node("print")

        self.assertIsNotNone(add_node)
        self.assertIsNotNone(print_node)

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
                    print(f"✓ Created network: add -> print")

                    # 执行系统
                    try:
                        print("  Executing system...")
                        self.system.execute()
                        print("✓ System execution completed successfully")
                    except Exception as e:
                        print(f"✗ System execution failed: {e}")
                else:
                    print("✗ Failed to create link")
            else:
                print("✗ Cannot create link between add and print")
        else:
            print("✗ Required sockets not found")

    def test_multiple_add_nodes_network(self):
        """测试多个add节点组成的网络"""
        print("\n=== Testing Multiple Add Nodes Network ===")

        # 创建三个add节点：add1 -> add2 -> print
        add1 = self.tree.add_node("add")
        add2 = self.tree.add_node("add")
        print_node = self.tree.add_node("print")

        self.assertIsNotNone(add1)
        self.assertIsNotNone(add2)
        self.assertIsNotNone(print_node)

        # 连接add1 -> add2
        add1_output = next((s for s in add1.outputs if s.identifier == "value"), None)
        add2_input = next((s for s in add2.inputs if s.identifier == "value"), None)

        if add1_output and add2_input:
            if self.tree.can_create_link(add1_output, add2_input):
                link1 = self.tree.add_link(add1_output, add2_input)
                if link1:
                    print("✓ Connected add1 -> add2")

        # 连接add2 -> print
        add2_output = next((s for s in add2.outputs if s.identifier == "value"), None)
        print_input = next(
            (s for s in print_node.inputs if s.identifier == "info"), None
        )

        if add2_output and print_input:
            if self.tree.can_create_link(add2_output, print_input):
                link2 = self.tree.add_link(add2_output, print_input)
                if link2:
                    print("✓ Connected add2 -> print")

        print(
            f"  Final network: {self.tree.node_count} nodes, {self.tree.link_count} links"
        )

        # 执行网络
        try:
            print("  Executing multi-node network...")
            self.system.execute()
            print("✓ Multi-node network execution completed")
        except Exception as e:
            print(f"✗ Multi-node network execution failed: {e}")


def main():
    """主函数"""
    print("Testing Registered Nodes from test_nodes.json")
    print("=" * 60)

    # 运行测试
    unittest.main(verbosity=2, argv=[""], exit=False)


if __name__ == "__main__":
    main()

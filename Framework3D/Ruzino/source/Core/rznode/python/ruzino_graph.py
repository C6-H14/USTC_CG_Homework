"""
Ruzino Graph API - A clean, Falcor-inspired API for node graph construction and execution.

Example:
    g = RuzinoGraph("MyGraph")
    g.loadConfiguration("path/to/config.json")
    
    # Create nodes
    add1 = g.createNode("add")
    add2 = g.createNode("add")
    
    # Connect nodes
    g.addEdge(add1, "result", add2, "a")
    
    # Set inputs
    g.setInput(add1, "a", 5)
    g.setInput(add1, "b", 3)
    
    # Execute
    g.execute()
    
    # Get outputs
    result = g.getOutput(add2, "result")
    print(f"Result: {result}")
"""

import nodes_core_py as core
import nodes_system_py as system
from typing import Any, Optional, Union, Dict, List, Tuple


class RuzinoGraph:
    """
    High-level interface for node graph construction and execution.
    Provides a clean, Falcor-style API.
    """
    
    def __init__(self, name: str = "Graph"):
        """
        Create a new render graph.
        
        Args:
            name: Name of the graph (for debugging/display)
        """
        self.name = name
        self._system: Optional[system.NodeDynamicLoadingSystem] = None
        self._tree: Optional[core.NodeTree] = None
        self._executor: Optional[system.NodeTreeExecutor] = None
        self._initialized = False
        self._node_name_counter = {}  # Track node names for auto-naming
        self._output_marks = []  # Track marked outputs
        
    def _ensure_initialized(self):
        """Ensure the graph system is initialized."""
        if not self._initialized:
            raise RuntimeError(
                "Graph not initialized. Call initialize() or load_configuration() first."
            )
    
    def initialize(self, config_path: Optional[str] = None) -> 'RuzinoGraph':
        """
        Initialize the graph system.
        
        Args:
            config_path: Optional path to JSON configuration file with node definitions
            
        Returns:
            self for chaining
        """
        # Only create system if not already initialized
        if self._system is None:
            self._system = system.create_dynamic_loading_system()
        
        if config_path:
            loaded = self._system.load_configuration(config_path)
            if not loaded:
                raise RuntimeError(f"Failed to load configuration from {config_path}")
        
        # Only init once
        if not self._initialized:
            self._system.init()
            self._tree = self._system.get_node_tree()
            self._executor = self._system.get_node_tree_executor()
            self._initialized = True
        
        return self
    
    def loadConfiguration(self, config_path: str) -> 'RuzinoGraph':
        """
        Load node definitions from a configuration file.
        Can be called multiple times to load multiple configuration files.
        
        Args:
            config_path: Path to the configuration JSON file
            
        Returns:
            self for method chaining
        """
        # If not initialized yet, do full initialization
        if not self._initialized:
            return self.initialize(config_path)
        
        # If already initialized, just load the additional configuration
        loaded = self._system.load_configuration(config_path)
        if not loaded:
            raise RuntimeError(f"Failed to load configuration from {config_path}")
        
        return self
    
    def createNode(self, node_type: str, properties: Optional[dict] = None, name: Optional[str] = None) -> core.Node:
        """
        Create a node in the graph (Falcor-style API).
        
        Args:
            node_type: Type of node to create (e.g., "add", "multiply")
            properties: Optional dictionary of properties to set on the node
            name: Optional custom name for the node. If not provided, auto-generates one.
            
        Returns:
            The created node
            
        Example:
            add_node = g.createNode("add", {"initial_value": 0}, "MyAdder")
        """
        self._ensure_initialized()
        
        # Auto-generate name if not provided
        if name is None:
            count = self._node_name_counter.get(node_type, 0)
            name = f"{node_type}_{count}"
            self._node_name_counter[node_type] = count + 1
        
        node = self._tree.add_node(node_type)
        if node is None:
            raise RuntimeError(f"Failed to create node of type '{node_type}'")
        
        node.ui_name = name
        
        # TODO: Apply properties if provided
        # This would require additional bindings for setting node properties
        if properties:
            print(f"Warning: Property setting not yet implemented. Properties ignored: {properties}")
        
        return node
    
    def addEdge(self, 
                from_node: Union[core.Node, str], 
                from_socket: str,
                to_node: Union[core.Node, str],
                to_socket: str) -> 'RuzinoGraph':
        """
        Connect two nodes (Falcor-style API).
        
        Args:
            from_node: Source node or node name
            from_socket: Name of output socket on source node
            to_node: Destination node or node name
            to_socket: Name of input socket on destination node
            
        Returns:
            self for chaining
            
        Example:
            g.addEdge(node1, "result", node2, "input")
            g.addEdge("node1", "result", "node2", "input")  # By name
        """
        self._ensure_initialized()
        
        # Resolve node references
        from_n = self._resolve_node(from_node)
        to_n = self._resolve_node(to_node)
        
        # Get sockets
        from_sock = from_n.get_output_socket(from_socket)
        to_sock = to_n.get_input_socket(to_socket)
        
        if from_sock is None:
            raise ValueError(f"Socket '{from_socket}' not found on node '{from_n.ui_name}'")
        if to_sock is None:
            raise ValueError(f"Socket '{to_socket}' not found on node '{to_n.ui_name}'")
        
        # Create link
        link = self._tree.add_link(from_sock, to_sock)
        if link is None:
            raise RuntimeError(
                f"Failed to create link from {from_n.ui_name}.{from_socket} "
                f"to {to_n.ui_name}.{to_socket}"
            )
        
        return self
    
    def addPass(self, node: core.Node, name: str) -> 'RuzinoGraph':
        """
        Add a pass to the graph (Falcor compatibility method).
        In Falcor, this names the pass. Here we just rename the node.
        
        Args:
            node: The node to name
            name: Name for the node
            
        Returns:
            self for chaining
        """
        node.ui_name = name
        return self
    
    def markOutput(self, node_or_spec: Union[core.Node, str], socket_name: Optional[str] = None) -> 'RuzinoGraph':
        """
        Mark an output for tracking (Falcor-style API).
        
        Args:
            node_or_spec: Either a Node object/name with socket_name parameter,
                         or a string in format "node_name.socket_name"
            socket_name: Socket name (optional if node_or_spec contains it)
            
        Returns:
            self for chaining
            
        Example:
            g.markOutput("AccumulatePass.output")
            g.markOutput(my_node, "output")
            g.markOutput("node_name", "socket_name")
        """
        if socket_name is None:
            # Falcor format: "node_name.socket_name"
            self._output_marks.append(node_or_spec)
        else:
            # Two-argument format
            if isinstance(node_or_spec, core.Node):
                output_spec = f"{node_or_spec.ui_name}.{socket_name}"
            else:
                output_spec = f"{node_or_spec}.{socket_name}"
            self._output_marks.append(output_spec)
        return self
    
    def setInput(self, 
                 node: Union[core.Node, str], 
                 socket_name: str, 
                 value: Any) -> 'RuzinoGraph':
        """
        Set an input value on a node.
        
        Args:
            node: Target node or node name
            socket_name: Name of input socket
            value: Value to set
            
        Returns:
            self for chaining
            
        Example:
            g.setInput(add_node, "a", 5)
            g.setInput("add_0", "b", 3)
        """
        self._ensure_initialized()
        
        n = self._resolve_node(node)
        socket = n.get_input_socket(socket_name)
        
        if socket is None:
            raise ValueError(f"Socket '{socket_name}' not found on node '{n.ui_name}'")
        
        # Convert Python value to meta_any
        meta_value = core.to_meta_any(value)
        self._executor.sync_node_from_external_storage(socket, meta_value)
        return self
    
    def setInputs(self, input_values: Dict[Tuple[Union[core.Node, str], str], Any]) -> 'RuzinoGraph':
        """
        Set multiple input values at once (batch operation).
        
        Args:
            input_values: Dictionary mapping (node, socket_name) tuples to values.
                         Example: {(add1, "a"): 5, (add1, "b"): 3, (add2, "a"): 10}
            
        Returns:
            self for chaining
            
        Example:
            g.setInputs({
                (ray_gen, "Aperture"): 0.0,
                (ray_gen, "Focus Distance"): 2.0,
                (accumulate, "Max Samples"): 16,
            })
        """
        self._ensure_initialized()
        
        for (node, socket_name), value in input_values.items():
            n = self._resolve_node(node)
            socket = n.get_input_socket(socket_name)
            if socket is None:
                raise ValueError(f"Socket '{socket_name}' not found on node '{n.ui_name}'")
            meta_value = core.to_meta_any(value)
            self._executor.sync_node_from_external_storage(socket, meta_value)
        
        return self
    
    def execute(self, required_node: Optional[Union[core.Node, str]] = None) -> 'RuzinoGraph':
        """
        Execute the graph.
        
        IMPORTANT: This will call prepare_tree() which initializes the executor.
        All inputs set via setInput() before execute() will be lost!
        Consider using execute_with_inputs() if you need to set inputs just before execution.
        
        Args:
            required_node: Optional node to execute up to. If None, executes entire graph.
            
        Returns:
            self for chaining
        """
        self._ensure_initialized()
        
        req_node = None
        if required_node is not None:
            req_node = self._resolve_node(required_node)
        
        # Note: execute() calls prepare_tree() which may reset the executor state
        # Inputs should be set AFTER prepare_tree() but BEFORE execute_tree()
        self._executor.execute(self._tree, req_node)
        return self
    
    def prepare_and_execute(self, input_values: Optional[Dict] = None, 
                           required_node: Optional[Union[core.Node, str]] = None,
                           auto_require_outputs: bool = True) -> 'RuzinoGraph':
        """
        Prepare the tree, set inputs, and execute - all in the correct order.
        
        This is the recommended way to execute the graph with inputs, as it ensures
        inputs are set AFTER prepare_tree() initializes the executor.
        
        Args:
            input_values: Dictionary mapping (node, socket_name) tuples to values.
                         Example: {(add1, "value"): 5, (add1, "value2"): 3}
            required_node: Optional node to execute up to. If None and auto_require_outputs
                          is True, will execute all nodes that have marked outputs.
            auto_require_outputs: If True and required_node is None, automatically
                                 execute nodes with marked outputs.
            
        Returns:
            self for chaining
            
        Example:
            g.prepare_and_execute({
                (add1, "value"): 5,
                (add1, "value2"): 3,
                (add2, "value2"): 10
            })
        """
        self._ensure_initialized()
        
        req_node = None
        if required_node is not None:
            req_node = self._resolve_node(required_node)
        elif auto_require_outputs and self._output_marks:
            # For multiple output marks, we need to execute all of them
            # Currently, prepare_tree only accepts ONE required_node, which will
            # propagate REQUIRED flag upstream.
            # For multi-branch graphs with multiple outputs, we have two options:
            # 1. Execute all nodes (req_node = None, but nodes without ALWAYS_REQUIRED won't run)
            # 2. Find the "furthest downstream" marked node (best effort)
            #
            # We use option 2: pick the last marked output (user should mark them in order)
            # Note: This may not execute all branches if they don't share common ancestors
            for mark in reversed(self._output_marks):
                if isinstance(mark, str) and '.' in mark:
                    node_name = mark.split('.')[0]
                    node = self.getNode(node_name)
                    if node:
                        req_node = node
                        break  # Use the last marked output node
        
        # Step 1: Prepare the tree (initializes executor state)
        self._executor.prepare_tree(self._tree, req_node)
        
        # Step 2: Set inputs AFTER preparation
        if input_values:
            self.setInputs(input_values)
        
        # Step 3: Execute the tree
        self._executor.execute_tree(self._tree)
        
        return self
    
    def getOutput(self, 
                  node: Union[core.Node, str], 
                  socket_name: str) -> Any:
        """
        Get an output value from a node.
        
        Args:
            node: Source node or node name
            socket_name: Name of output socket
            
        Returns:
            The output value. For now, returns a placeholder dict.
            TODO: Implement proper value extraction once entt_py is built.
            
        Example:
            result = g.getOutput(add_node, "result")
            result = g.getOutput("add_0", "result")
        """
        self._ensure_initialized()
        
        n = self._resolve_node(node)
        socket = n.get_output_socket(socket_name)
        
        if socket is None:
            raise ValueError(f"Socket '{socket_name}' not found on node '{n.ui_name}'")
        
        # Get value from executor
        result = core.meta_any()
        self._executor.sync_node_to_external_storage(socket, result)
        
        # Try to extract the value based on type
        type_name = result.type_name()
        if type_name == "int":
            return result.cast_int()
        elif type_name == "float":
            return result.cast_float()
        elif type_name == "double":
            return result.cast_double()
        elif type_name == "bool":
            return result.cast_bool()
        elif "string" in type_name.lower() or "basic_string" in type_name.lower():
            return result.cast_string()
        else:
            # Return the raw meta_any for complex types
            return result
    
    def getNode(self, name: str) -> Optional[core.Node]:
        """
        Get a node by its name.
        
        Args:
            name: Node name (ui_name)
            
        Returns:
            The node, or None if not found
        """
        self._ensure_initialized()
        
        for node in self._tree.nodes:
            if node.ui_name == name:
                return node
        return None
    
    def _resolve_node(self, node_ref: Union[core.Node, str]) -> core.Node:
        """
        Resolve a node reference (node object or name string).
        
        Args:
            node_ref: Node object or node name
            
        Returns:
            The resolved node
            
        Raises:
            ValueError: If node cannot be resolved
        """
        if isinstance(node_ref, str):
            node = self.getNode(node_ref)
            if node is None:
                raise ValueError(f"Node '{node_ref}' not found in graph")
            return node
        return node_ref
    
    def serialize(self) -> str:
        """
        Serialize the graph to JSON.
        
        Returns:
            JSON string representation of the graph
        """
        self._ensure_initialized()
        return self._tree.serialize()
    
    def deserialize(self, json_str: str) -> 'RuzinoGraph':
        """
        Deserialize a graph from JSON.
        
        Args:
            json_str: JSON string representation
            
        Returns:
            self for chaining
        """
        self._ensure_initialized()
        self._tree.deserialize(json_str)
        return self
    
    def clear(self) -> 'RuzinoGraph':
        """
        Clear all nodes and links from the graph.
        
        Returns:
            self for chaining
        """
        self._ensure_initialized()
        self._tree.clear()
        self._node_name_counter.clear()
        self._output_marks.clear()
        return self
    
    def setGlobalParams(self, params: Any) -> 'RuzinoGraph':
        """
        Set global parameters for node execution.
        Used for passing context like USD stages, time codes, etc.
        
        Args:
            params: Global parameters object (e.g., GeomPayload for geometry nodes)
            
        Returns:
            self for chaining
            
        Example:
            # For USD export - automatic conversion
            from stage_py import GeomPayload, create_payload_from_stage
            stage = stage_py.Stage(output_file)
            geom_payload = create_payload_from_stage(stage, "/geom")
            g.setGlobalParams(geom_payload)  # Automatically converts to meta_any
        """
        self._ensure_initialized()
        
        # Auto-convert GeomPayload to meta_any if needed
        # This allows seamless usage without explicit conversion
        try:
            # Check if it's a GeomPayload (from stage_py module)
            import stage_py
            if isinstance(params, stage_py.GeomPayload):
                params = stage_py.create_meta_any_from_payload(params)
        except (ImportError, AttributeError):
            # If stage_py not available or not a GeomPayload, pass as-is
            pass
        self._system.set_global_params(params)
        return self
    
    @property
    def nodes(self):
        """Get list of all nodes in the graph."""
        self._ensure_initialized()
        return self._tree.nodes
    
    @property
    def links(self):
        """Get list of all links in the graph."""
        self._ensure_initialized()
        return self._tree.links
    
    def to_python_code(self, 
                       required_node: Optional[Union[core.Node, str]] = None,
                       include_imports: bool = True,
                       include_comments: bool = True,
                       use_graph_api: bool = True) -> str:
        """
        Generate executable Python code that recreates this graph.
        
        This method generates a Python script that:
        - Imports necessary modules
        - Creates all nodes with their configuration
        - Sets up connections between nodes
        - Provides placeholders for input values
        - Marks outputs
        - Executes the graph
        - Retrieves and prints results
        
        Args:
            required_node: Optional node to generate code for (and its dependencies).
                          If None, generates code for the entire graph.
            include_imports: Include import statements at the top
            include_comments: Add explanatory comments
            use_graph_api: Use RuzinoGraph API (recommended). If False, uses raw node operations.
            
        Returns:
            Complete Python script as a string
            
        Example:
            # Create and build a graph
            g = RuzinoGraph("MyGraph")
            g.loadConfiguration("config.json")
            add1 = g.createNode("add", name="first_add")
            add2 = g.createNode("add", name="second_add")
            g.addEdge(add1, "value", add2, "value")
            g.markOutput(add2, "value")
            
            # Generate Python code
            code = g.to_python_code()
            print(code)
            
            # Save to file
            with open("generated_graph.py", "w") as f:
                f.write(code)
        """
        self._ensure_initialized()
        
        req_node = None
        if required_node is not None:
            req_node = self._resolve_node(required_node)
        
        # Call C++ to_python_code_with_options through the tree binding
        code = self._tree.to_python_code_with_options(
            include_imports, 
            include_comments,
            use_graph_api,
            req_node
        )
        
        # Post-process the generated code to replace hardcoded config with actual loaded configs
        if self._system is not None:
            loaded_configs = self._system.get_loaded_configs()
            if loaded_configs:
                # Replace the hardcoded config line with actual loaded configs
                import re
                # Find and replace the config loading section
                config_pattern = r'config_path = os\.path\.join\(binary_dir, "test_nodes\.json"\)\ng\.loadConfiguration\(config_path\)'
                
                # Generate proper config loading code
                config_lines = []
                for config_file in loaded_configs:
                    config_lines.append(f'config_path = os.path.join(binary_dir, "{config_file}")')
                    config_lines.append('g.loadConfiguration(config_path)')
                
                replacement = '\n'.join(config_lines)
                code = re.sub(config_pattern, replacement, code)
        
        return code
    
    def save_python_code(self, 
                        filepath: str,
                        required_node: Optional[Union[core.Node, str]] = None,
                        include_imports: bool = True,
                        include_comments: bool = True) -> 'RuzinoGraph':
        """
        Generate Python code and save it to a file.
        
        Args:
            filepath: Path to save the generated Python code
            required_node: Optional node to generate code for (and its dependencies)
            include_imports: Include import statements
            include_comments: Add explanatory comments
            
        Returns:
            self for chaining
            
        Example:
            g.save_python_code("my_generated_graph.py")
        """
        code = self.to_python_code(required_node, include_imports, include_comments)
        with open(filepath, 'w') as f:
            f.write(code)
        return self
    
    def __repr__(self):
        if not self._initialized:
            return f"RuzinoGraph('{self.name}', uninitialized)"
        return f"RuzinoGraph('{self.name}', nodes={len(self.nodes)}, links={len(self.links)})"


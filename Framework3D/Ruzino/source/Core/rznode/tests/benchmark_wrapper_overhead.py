"""
Performance benchmark for Python wrapper overhead analysis.

This benchmark compares the overhead of using Python wrappers vs direct C++ execution
for the node graph system. It performs multiple iterations with varying graph complexities
to ensure fair and consistent measurements.

Measurements include:
1. Graph construction time
2. Graph execution time
3. Total operation time
4. Per-node overhead
5. Memory overhead (if applicable)
"""

import os
import sys
import time
import statistics
from typing import Dict, List, Tuple
from dataclasses import dataclass

# Set up paths before importing modules
tests_dir = os.path.dirname(os.path.abspath(__file__))
rznode_dir = os.path.abspath(os.path.join(tests_dir, ".."))
python_dir = os.path.join(rznode_dir, "python")
binary_dir = os.path.join(tests_dir, "..", "..", "..", "..", "Binaries", "Release")
binary_dir = os.path.abspath(binary_dir)

# Change to binary directory so DLLs can be loaded
original_cwd = os.getcwd()
os.chdir(binary_dir)

# Add to Python path
if binary_dir not in sys.path:
    sys.path.insert(0, binary_dir)
if python_dir not in sys.path:
    sys.path.insert(0, python_dir)

# Import modules
from ruzino_graph import RuzinoGraph
import nodes_core_py as core
import nodes_system_py as system

# Get binary directory for test configuration files
binary_dir = os.getcwd()


@dataclass
class BenchmarkResult:
    """Container for benchmark results."""
    name: str
    mean: float
    median: float
    std_dev: float
    min_time: float
    max_time: float
    iterations: int
    
    def __repr__(self):
        return (f"{self.name}:\n"
                f"  Mean: {self.mean*1000:.4f}ms\n"
                f"  Median: {self.median*1000:.4f}ms\n"
                f"  Std Dev: {self.std_dev*1000:.4f}ms\n"
                f"  Min: {self.min_time*1000:.4f}ms\n"
                f"  Max: {self.max_time*1000:.4f}ms\n"
                f"  Iterations: {self.iterations}")


def benchmark_function(func, iterations: int = 100) -> BenchmarkResult:
    """
    Benchmark a function with multiple iterations.
    
    Args:
        func: Function to benchmark (callable with no arguments)
        iterations: Number of iterations to run
        
    Returns:
        BenchmarkResult with timing statistics
    """
    times = []
    
    # Warmup runs (not counted)
    for _ in range(5):
        func()
    
    # Actual benchmark runs
    for _ in range(iterations):
        start = time.perf_counter()
        func()
        end = time.perf_counter()
        times.append(end - start)
    
    return BenchmarkResult(
        name=func.__name__,
        mean=statistics.mean(times),
        median=statistics.median(times),
        std_dev=statistics.stdev(times) if len(times) > 1 else 0.0,
        min_time=min(times),
        max_time=max(times),
        iterations=iterations
    )


def create_simple_graph() -> Tuple[RuzinoGraph, List]:
    """
    Create a simple linear graph: add1 -> add2 -> add3
    
    Returns:
        Tuple of (graph, list of nodes)
    """
    g = RuzinoGraph("SimpleGraph")
    config_path = os.path.join(binary_dir, "test_nodes.json")
    g.loadConfiguration(config_path)
    
    add1 = g.createNode("add", name="add1")
    add2 = g.createNode("add", name="add2")
    add3 = g.createNode("add", name="add3")
    
    g.addEdge(add1, "value", add2, "value")
    g.addEdge(add2, "value", add3, "value")
    g.markOutput(add3, "value")
    
    return g, [add1, add2, add3]


def create_complex_graph(chain_length: int = 10) -> Tuple[RuzinoGraph, List]:
    """
    Create a more complex linear chain graph.
    
    Args:
        chain_length: Number of nodes in the chain
        
    Returns:
        Tuple of (graph, list of nodes)
    """
    g = RuzinoGraph(f"ComplexGraph_{chain_length}")
    config_path = os.path.join(binary_dir, "test_nodes.json")
    g.loadConfiguration(config_path)
    
    nodes = []
    for i in range(chain_length):
        node = g.createNode("add", name=f"add{i}")
        nodes.append(node)
        if i > 0:
            g.addEdge(nodes[i-1], "value", nodes[i], "value")
    
    g.markOutput(nodes[-1], "value")
    
    return g, nodes


def create_branching_graph(branches: int = 3, depth: int = 3) -> Tuple[RuzinoGraph, List]:
    """
    Create a branching graph with multiple outputs.
    
    Args:
        branches: Number of branches from root
        depth: Depth of each branch
        
    Returns:
        Tuple of (graph, list of all nodes)
    """
    g = RuzinoGraph(f"BranchingGraph_{branches}x{depth}")
    config_path = os.path.join(binary_dir, "test_nodes.json")
    g.loadConfiguration(config_path)
    
    # Create root node
    root = g.createNode("add", name="root")
    nodes = [root]
    
    # Create branches
    leaf_nodes = []
    for b in range(branches):
        prev_node = root
        for d in range(depth):
            node = g.createNode("add", name=f"branch{b}_depth{d}")
            nodes.append(node)
            g.addEdge(prev_node, "value", node, "value")
            prev_node = node
        leaf_nodes.append(prev_node)
        g.markOutput(prev_node, "value")
    
    return g, nodes


class GraphBenchmarkSuite:
    """Suite of benchmarks for graph operations."""
    
    def __init__(self, iterations: int = 100):
        self.iterations = iterations
        self.results: Dict[str, BenchmarkResult] = {}
    
    def benchmark_graph_creation(self, complexity: str = "simple"):
        """Benchmark graph creation time."""
        print(f"\n{'='*60}")
        print(f"Benchmarking {complexity} graph creation...")
        print(f"{'='*60}")
        
        if complexity == "simple":
            result = benchmark_function(
                lambda: create_simple_graph(),
                self.iterations
            )
        elif complexity == "medium":
            result = benchmark_function(
                lambda: create_complex_graph(20),
                self.iterations
            )
        elif complexity == "large":
            result = benchmark_function(
                lambda: create_complex_graph(50),
                self.iterations
            )
        else:
            raise ValueError(f"Unknown complexity: {complexity}")
        
        result.name = f"Graph Creation ({complexity})"
        self.results[result.name] = result
        print(result)
        return result
    
    def benchmark_graph_execution(self, complexity: str = "simple"):
        """Benchmark graph execution time."""
        print(f"\n{'='*60}")
        print(f"Benchmarking {complexity} graph execution...")
        print(f"{'='*60}")
        
        # Create the graph once
        if complexity == "simple":
            g, nodes = create_simple_graph()
            inputs = {
                (nodes[0], "value"): 1,
                (nodes[0], "value2"): 2,
                (nodes[1], "value2"): 3,
                (nodes[2], "value2"): 4,
            }
        elif complexity == "medium":
            g, nodes = create_complex_graph(20)
            inputs = {(nodes[0], "value"): 1, (nodes[0], "value2"): 2}
            for i in range(1, len(nodes)):
                inputs[(nodes[i], "value2")] = i + 1
        elif complexity == "large":
            g, nodes = create_complex_graph(50)
            inputs = {(nodes[0], "value"): 1, (nodes[0], "value2"): 2}
            for i in range(1, len(nodes)):
                inputs[(nodes[i], "value2")] = i + 1
        else:
            raise ValueError(f"Unknown complexity: {complexity}")
        
        # Benchmark execution
        def execute():
            g.prepare_and_execute(inputs)
            return g.getOutput(nodes[-1], "value")
        
        result = benchmark_function(execute, self.iterations)
        result.name = f"Graph Execution ({complexity})"
        self.results[result.name] = result
        print(result)
        return result
    
    def benchmark_full_cycle(self, complexity: str = "simple"):
        """Benchmark full create + execute + read cycle."""
        print(f"\n{'='*60}")
        print(f"Benchmarking {complexity} full cycle (create + execute + read)...")
        print(f"{'='*60}")
        
        def full_cycle():
            if complexity == "simple":
                g, nodes = create_simple_graph()
                inputs = {
                    (nodes[0], "value"): 1,
                    (nodes[0], "value2"): 2,
                    (nodes[1], "value2"): 3,
                    (nodes[2], "value2"): 4,
                }
            elif complexity == "medium":
                g, nodes = create_complex_graph(20)
                inputs = {(nodes[0], "value"): 1, (nodes[0], "value2"): 2}
                for i in range(1, len(nodes)):
                    inputs[(nodes[i], "value2")] = i + 1
            elif complexity == "large":
                g, nodes = create_complex_graph(50)
                inputs = {(nodes[0], "value"): 1, (nodes[0], "value2"): 2}
                for i in range(1, len(nodes)):
                    inputs[(nodes[i], "value2")] = i + 1
            else:
                raise ValueError(f"Unknown complexity: {complexity}")
            
            g.prepare_and_execute(inputs)
            result = g.getOutput(nodes[-1], "value")
            return result
        
        result = benchmark_function(full_cycle, self.iterations)
        result.name = f"Full Cycle ({complexity})"
        self.results[result.name] = result
        print(result)
        return result
    
    def benchmark_python_wrapper_overhead(self):
        """
        Benchmark specific Python wrapper operations to identify overhead sources.
        Focus on execution and data input/output overhead.
        """
        print(f"\n{'='*60}")
        print("Benchmarking Python wrapper operation overhead...")
        print(f"{'='*60}")
        
        # Setup
        g, nodes = create_simple_graph()
        add1, add2, add3 = nodes
        executor = g._executor
        
        # 1. Socket access overhead
        def socket_access():
            socket = add1.get_input_socket("value")
            return socket
        
        result = benchmark_function(socket_access, self.iterations * 10)
        result.name = "Socket Access"
        self.results[result.name] = result
        print(result)
        
        # 2. Input setting overhead (单个)
        socket = add1.get_input_socket("value")
        
        def set_input():
            meta_value = core.to_meta_any(42)
            executor.sync_node_from_external_storage(socket, meta_value)
        
        result = benchmark_function(set_input, self.iterations * 10)
        result.name = "Set Input (sync_node_from_external_storage)"
        self.results[result.name] = result
        print(result)
        
        # 3. Output reading overhead (单个)
        g.prepare_and_execute({
            (add1, "value"): 1,
            (add1, "value2"): 2,
            (add2, "value2"): 3,
            (add3, "value2"): 4,
        })
        
        output_socket = add3.get_output_socket("value")
        
        def read_output():
            result = core.meta_any()
            executor.sync_node_to_external_storage(output_socket, result)
            return result.cast_int()
        
        result = benchmark_function(read_output, self.iterations * 10)
        result.name = "Read Output (sync_node_to_external_storage)"
        self.results[result.name] = result
        print(result)
        
        # 4. Socket 缓存优化 - 预先获取所有 socket 减少查找
        print("\n--- Testing CACHED socket operations ---")
        
        cached_sockets = [
            add1.get_input_socket("value"),
            add1.get_input_socket("value2"),
            add2.get_input_socket("value2"),
            add3.get_input_socket("value2"),
        ]
        cached_values = [1, 2, 3, 4]
        
        def set_inputs_cached():
            for socket, value in zip(cached_sockets, cached_values):
                executor.sync_node_from_external_storage(socket, core.to_meta_any(value))
        
        result = benchmark_function(set_inputs_cached, self.iterations * 10)
        result.name = "Set Inputs CACHED (pre-fetched sockets)"
        self.results[result.name] = result
        print(result)
    
    def benchmark_branching_graph(self):
        """Benchmark branching graph with multiple outputs."""
        print(f"\n{'='*60}")
        print("Benchmarking branching graph...")
        print(f"{'='*60}")
        
        g, nodes = create_branching_graph(branches=3, depth=5)
        
        # Setup inputs
        inputs = {(nodes[0], "value"): 1, (nodes[0], "value2"): 2}
        for i, node in enumerate(nodes[1:], start=1):
            inputs[(node, "value2")] = i
        
        def execute_branching():
            g.prepare_and_execute(inputs)
            # Read all outputs
            results = []
            for node in nodes[-3:]:  # Last 3 are leaf nodes
                results.append(g.getOutput(node, "value"))
            return results
        
        result = benchmark_function(execute_branching, self.iterations)
        result.name = "Branching Graph (3x5)"
        self.results[result.name] = result
        print(result)
        return result
    
    def print_summary(self):
        """Print a summary of all benchmark results."""
        print(f"\n{'='*60}")
        print("BENCHMARK SUMMARY")
        print(f"{'='*60}")
        print(f"Total benchmarks run: {len(self.results)}")
        print(f"Iterations per benchmark: {self.iterations}")
        print(f"\n{'Benchmark':<50} {'Mean (ms)':<15} {'Std Dev (ms)':<15}")
        print("-" * 80)
        
        for name, result in self.results.items():
            print(f"{name:<50} {result.mean*1000:<15.4f} {result.std_dev*1000:<15.4f}")
        
        print(f"\n{'='*60}")
        
        # Calculate relative overheads
        print("\nRELATIVE OVERHEAD ANALYSIS:")
        print(f"{'='*60}")
        
        # Per-node overhead estimation
        if "Graph Execution (simple)" in self.results and "Graph Execution (large)" in self.results:
            simple_time = self.results["Graph Execution (simple)"].mean
            large_time = self.results["Graph Execution (large)"].mean
            
            # Simple has 3 nodes, large has 50 nodes
            simple_nodes = 3
            large_nodes = 50
            
            # Rough per-node overhead estimate
            per_node_overhead = (large_time - simple_time) / (large_nodes - simple_nodes)
            
            print(f"\nEstimated per-node execution overhead: {per_node_overhead*1000:.4f}ms")
            print(f"  (Based on difference between simple and large graph execution)")
        
        # Wrapper operation overhead as percentage
        if "Socket Access" in self.results and "Graph Execution (simple)" in self.results:
            socket_access_time = self.results["Socket Access"].mean
            simple_exec_time = self.results["Graph Execution (simple)"].mean
            
            # In simple graph execution, we access sockets ~12 times (3 nodes * 2 inputs * 2 access patterns)
            estimated_socket_overhead_pct = (socket_access_time * 12) / simple_exec_time * 100
            
            print(f"\nEstimated socket access overhead: {estimated_socket_overhead_pct:.2f}% of execution time")
            print(f"  (Based on simple graph with 3 nodes)")


def run_comprehensive_benchmark():
    """Run the comprehensive benchmark suite."""
    print("\n" + "="*60)
    print("PYTHON WRAPPER OVERHEAD BENCHMARK")
    print("="*60)
    print("\nThis benchmark suite measures the overhead introduced by Python")
    print("wrappers compared to direct C++ execution.")
    print("\nConfiguration:")
    print(f"  Working directory: {os.getcwd()}")
    print(f"  Test config: {os.path.join(binary_dir, 'test_nodes.json')}")
    
    # Verify test configuration exists
    config_path = os.path.join(binary_dir, "test_nodes.json")
    if not os.path.exists(config_path):
        print(f"\n❌ ERROR: Test configuration not found: {config_path}")
        return
    
    print(f"  ✓ Configuration file found")
    
    # Create benchmark suite with 100 iterations (can be increased for more precision)
    suite = GraphBenchmarkSuite(iterations=100)
    
    # Run all benchmarks
    try:
        # 1. Graph creation benchmarks
        suite.benchmark_graph_creation("simple")
        suite.benchmark_graph_creation("medium")
        suite.benchmark_graph_creation("large")
        
        # 2. Graph execution benchmarks
        suite.benchmark_graph_execution("simple")
        suite.benchmark_graph_execution("medium")
        suite.benchmark_graph_execution("large")
        
        # 3. Full cycle benchmarks
        suite.benchmark_full_cycle("simple")
        suite.benchmark_full_cycle("medium")
        suite.benchmark_full_cycle("large")
        
        # 4. Python wrapper overhead analysis
        suite.benchmark_python_wrapper_overhead()
        
        # 5. Branching graph benchmark
        suite.benchmark_branching_graph()
        
        # Print summary
        suite.print_summary()
        
        print("\n✓ Benchmark suite completed successfully!")
        
    except Exception as e:
        print(f"\n❌ Benchmark failed with error: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    run_comprehensive_benchmark()

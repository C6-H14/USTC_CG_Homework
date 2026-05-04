#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include "entt/meta/meta.hpp"
#include "nodes/core/node.hpp"
#include "nodes/core/node_exec_eager.hpp"
#include "nodes/core/node_tree.hpp"
#include "nodes/system/node_system.hpp"
#include "nodes/system/node_system_dl.hpp"

namespace nb = nanobind;
using namespace Ruzino;

NB_MODULE(nodes_system_py, m)
{
    // NodeTreeExecutor
    nb::class_<NodeTreeExecutor>(m, "NodeTreeExecutor")
        .def(
            "execute",
            static_cast<void (NodeTreeExecutor::*)(NodeTree*, Node*)>(
                &NodeTreeExecutor::execute),
            nb::arg("tree"),
            nb::arg("required_node") = nullptr,
            "Execute the node tree with specific tree and optional required "
            "node")
        .def(
            "prepare_tree",
            &NodeTreeExecutor::prepare_tree,
            nb::arg("tree"),
            nb::arg("required_node") = nullptr,
            "Prepare the tree for execution")
        .def(
            "execute_tree",
            &NodeTreeExecutor::execute_tree,
            nb::arg("tree"),
            "Execute the prepared tree")
        .def(
            "sync_node_from_external_storage",
            &NodeTreeExecutor::sync_node_from_external_storage,
            nb::arg("socket"),
            nb::arg("data"),
            "Set socket value from external data")
        .def(
            "sync_node_to_external_storage",
            &NodeTreeExecutor::sync_node_to_external_storage,
            nb::arg("socket"),
            nb::arg("data"),
            "Get socket value to external data")
        .def(
            "sync_batch_from_external",
            [](NodeTreeExecutor& exec, const nb::list& data) {
                for (size_t i = 0; i < data.size(); ++i) {
                    auto pair = nb::cast<nb::tuple>(data[i]);
                    auto* socket = nb::cast<NodeSocket*>(pair[0]);
                    auto value = nb::cast<entt::meta_any>(pair[1]);
                    exec.sync_node_from_external_storage(socket, value);
                }
            },
            nb::arg("data"),
            "Batch set socket values: [(socket, meta_any), ...]")
        .def(
            "sync_batch_to_external",
            [](NodeTreeExecutor& exec, const nb::list& sockets) {
                nb::list results;
                for (size_t i = 0; i < sockets.size(); ++i) {
                    auto* socket = nb::cast<NodeSocket*>(sockets[i]);
                    entt::meta_any data;
                    exec.sync_node_to_external_storage(socket, data);
                    results.append(data);
                }
                return results;
            },
            nb::arg("sockets"),
            "Batch get socket values: [socket, ...] -> [meta_any, ...]")
        .def(
            "notify_node_dirty",
            &NodeTreeExecutor::notify_node_dirty,
            nb::arg("node"),
            "Notify executor that a node has been modified")
        .def(
            "notify_socket_dirty",
            &NodeTreeExecutor::notify_socket_dirty,
            nb::arg("socket"),
            "Notify executor that a socket has been modified")
        .def(
            "reset_allocator",
            &NodeTreeExecutor::reset_allocator,
            "Reset the resource allocator (cleanup resources before "
            "rendering)");

    // Base NodeSystem class
    nb::class_<NodeSystem>(m, "NodeSystem")
        .def(
            "init",
            static_cast<void (NodeSystem::*)()>(&NodeSystem::init),
            "Initialize the node system with a default empty tree")
        .def(
            "load_configuration",
            &NodeSystem::load_configuration,
            nb::arg("config"),
            "Load node tree configuration from a file")
        .def(
            "execute",
            &NodeSystem::execute,
            nb::arg("is_ui_execution") = false,
            nb::arg("required_node") = nullptr,
            "Execute the node tree")
        .def(
            "get_node_tree",
            &NodeSystem::get_node_tree,
            nb::rv_policy::reference_internal,
            "Get the node tree associated with this system")
        .def(
            "get_node_tree_executor",
            &NodeSystem::get_node_tree_executor,
            nb::rv_policy::reference_internal,
            "Get the node tree executor")
        .def(
            "get_loaded_configs",
            [](const NodeSystem& self) {
                const auto& configs = self.get_loaded_configs();
                nb::list result;
                for (const auto& config : configs) {
                    result.append(config);
                }
                return result;
            },
            "Get list of loaded configuration files")
        .def_rw(
            "allow_ui_execution",
            &NodeSystem::allow_ui_execution,
            "Flag to allow execution triggered by UI interactions")
        .def("finalize", &NodeSystem::finalize, "Finalize the node system")
        .def(
            "set_global_params",
            &NodeSystem::set_global_params_any,
            nb::arg("params"),
            "Set global parameters (as meta_any) for node execution. "
            "Use create_meta_any_from_payload in stage_py module to convert "
            "GeomPayload.");

    // NodeDynamicLoadingSystem - concrete implementation
    nb::class_<NodeDynamicLoadingSystem, NodeSystem>(
        m, "NodeDynamicLoadingSystem")
        .def(
            "load_configuration",
            &NodeDynamicLoadingSystem::load_configuration,
            nb::arg("config"),
            "Load node tree configuration and dynamic libraries from a file");

    // Factory function
    m.def(
        "create_dynamic_loading_system",
        &create_dynamic_loading_system,
        nb::rv_policy::take_ownership,
        "Create a NodeSystem instance that supports dynamic loading of nodes");
}

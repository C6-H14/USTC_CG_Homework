#include <spdlog/spdlog.h>

#include "nodes/core/api.h"
#include "nodes/core/api.hpp"
#include "nodes/core/io/json.hpp"
#include "nodes/core/math/vec.hpp"
#include "nodes/core/node.hpp"
#include "nodes/core/node_tree.hpp"

RUZINO_NAMESPACE_OPEN_SCOPE

extern std::map<std::string, NodeTypeInfo*> conversion_node_registry;

bool NodeSocket::is_placeholder() const
{
    return !socket_group_identifier.empty() && socket_group->runtime_dynamic &&
           std::string(ui_name).empty();
}

// Hack...
#define PXR_NAMESPACE "pxrInternal_v0_25_5__pxrReserved__"

static bool is_vec2(const SocketType& type)
{
    if (type.info().name() == "class " PXR_NAMESPACE "::GfVec2f") {
        return true;
    }
    if (type.info().name() == "struct glm::vec<2,float,0>") {
        return true;
    }
    return false;
}

static bool is_vec3(const SocketType& type)
{
    if (type.info().name() == "class " PXR_NAMESPACE "::GfVec3f") {
        return true;
    }
    if (type.info().name() == "struct glm::vec<3,float,0>") {
        return true;
    }
    return false;
}

static bool is_vec4(const SocketType& type)
{
    if (type.info().name() == "class " PXR_NAMESPACE "::GfVec4f") {
        return true;
    }
    if (type.info().name() == "struct glm::vec<4,float,0>") {
        return true;
    }
    return false;
}

void NodeSocket::Serialize(nlohmann::json& value)
{
    auto& socket = value[std::to_string(ID.Get())];

    // Socket group treatment
    if (!socket_group_identifier.empty() && !std::string(ui_name).empty()) {
        socket["socket_group_identifier"] = socket_group_identifier;
    }

    // Repeated storage. Simpler code for iteration.
    socket["ID"] = ID.Get();
    socket["id_name"] = get_type_name(type_info);
    socket["identifier"] = identifier;
    socket["ui_name"] = ui_name;
    socket["in_out"] = in_out;
    socket["optional"] = optional;

    if (dataField.value) {
        switch (type_info.id()) {
            using namespace entt::literals;
            case entt::type_hash<int>().value():
                socket["value"] = default_value_typed<int>();
                break;
            case entt::type_hash<float>().value():
                socket["value"] = default_value_typed<float>();
                break;
            case entt::type_hash<double>().value():
                socket["value"] = default_value_typed<double>();
                break;
            case entt::type_hash<std::string>().value():
                socket["value"] = default_value_typed<std::string&>().c_str();
                break;
            case entt::type_hash<bool>().value():
                socket["value"] = default_value_typed<bool>();
                break;
            case entt::type_hash<Vec2f>().value():
                socket["value"] = { default_value_typed<Vec2f&>()[0],
                                    default_value_typed<Vec2f&>()[1] };
                break;

            case entt::type_hash<Vec3f>().value():
                socket["value"] = { default_value_typed<Vec3f&>()[0],
                                    default_value_typed<Vec3f&>()[1],
                                    default_value_typed<Vec3f&>()[2] };
                break;

            default:
                if (is_vec2(type_info)) {
                    auto vec = default_value_typed_force<Vec2f&>();
                    socket["value"] = { vec[0], vec[1] };
                }
                else if (is_vec3(type_info)) {
                    auto vec = default_value_typed_force<Vec3f&>();
                    socket["value"] = { vec[0], vec[1], vec[2] };
                }
                else if (is_vec4(type_info)) {
                    auto vec = default_value_typed_force<Vec4f&>();
                    socket["value"] = { vec[0], vec[1], vec[2], vec[3] };
                }
                else
                    spdlog::error(
                        "Unknown type {} in serialization",
                        type_info.info().name());
                break;
        }
    }
}

void NodeSocket::DeserializeInfo(nlohmann::json& socket_json)
{
    ID = socket_json["ID"].get<unsigned>();

    type_info =
        get_socket_type(socket_json["id_name"].get<std::string>().c_str());
    in_out = socket_json["in_out"].get<PinKind>();
    strcpy(ui_name, socket_json["ui_name"].get<std::string>().c_str());
    strcpy(identifier, socket_json["identifier"].get<std::string>().c_str());
    if (socket_json.find("optional") != socket_json.end()) {
        optional = socket_json["optional"].get<bool>();
    }
    if (socket_json.find("socket_group_identifier") != socket_json.end()) {
        socket_group_identifier =
            socket_json["socket_group_identifier"].get<std::string>();
    }
}

void NodeSocket::DeserializeValue(const nlohmann::json& value)
{
    if (dataField.value) {
        if (value.find("value") != value.end()) {
            switch (type_info.id()) {
                case entt::type_hash<int>():
                    default_value_typed<int&>() = value["value"];
                    break;
                case entt::type_hash<float>():
                    default_value_typed<float&>() = value["value"];
                    break;
                case entt::type_hash<std::string>(): {
                    std::string str = value["value"];
                    default_value_typed<std::string&>() = str;
                } break;
                case entt::type_hash<bool>():
                    default_value_typed<bool&>() = value["value"];
                    break;
                case entt::type_hash<Vec2f>():
                    default_value_typed<Vec2f&>() =
                        Vec2f(value["value"][0], value["value"][1]);
                    break;
                case entt::type_hash<Vec3f>():
                    default_value_typed<Vec3f&>() = Vec3f(
                        value["value"][0],
                        value["value"][1],
                        value["value"][2]);
                    break;
                case entt::type_hash<Vec4f>():
                    default_value_typed<Vec4f&>() = Vec4f(
                        value["value"][0],
                        value["value"][1],
                        value["value"][2],
                        value["value"][3]);

                default:
                    if (is_vec2(type_info)) {
                        default_value_typed_force<Vec2f&>() =
                            Vec2f(value["value"][0], value["value"][1]);
                    }
                    else if (is_vec3(type_info)) {
                        default_value_typed_force<Vec3f&>() = Vec3f(
                            value["value"][0],
                            value["value"][1],
                            value["value"][2]);
                    }
                    else if (is_vec4(type_info)) {
                        default_value_typed_force<Vec4f&>() = Vec4f(
                            value["value"][0],
                            value["value"][1],
                            value["value"][2],
                            value["value"][3]);
                    }
                    else
                        spdlog::error(
                            "Unknown type {} in deserialization",
                            type_info.info().name());

                    break;
            }
        }
    }
}

NodeSocket* SocketGroup::add_socket(
    const char* type_name,
    const char* socket_identifier,
    const char* name,
    bool need_to_propagate_sync)
{
    assert(!std::string(identifier).empty());

    if (need_to_propagate_sync && !synchronized_groups.empty()) {
        for (auto sync_group : synchronized_groups) {
            sync_group->add_socket(type_name, socket_identifier, name, false);
            sync_group->node->refresh_node();
        }
    }

    auto socket = node->add_socket(type_name, socket_identifier, name, kind);
    socket->socket_group = this;
    socket->socket_group_identifier = identifier;

    if (!std::string(name).empty())
        sockets.insert(sockets.end() - 1, socket);
    else
        sockets.push_back(socket);

    return socket;
}

void SocketGroup::add_sync_group(SocketGroup* group)
{
    synchronized_groups.emplace(group);
    group->synchronized_groups.emplace(this);
    for (auto sync_group : synchronized_groups) {
        assert(sync_group->sockets.size() == sockets.size());
    }
}

void SocketGroup::remove_socket(
    const char* socket_identifier,
    bool need_to_propagate_sync)
{
    auto it = std::find_if(
        sockets.begin(),
        sockets.end(),
        [socket_identifier](NodeSocket* socket) {
            return strcmp(socket->identifier, socket_identifier) == 0;
        });

    auto id = std::distance(sockets.begin(), it);

    if (it != sockets.end()) {
        bool can_delete = true;
        if (need_to_propagate_sync && !synchronized_groups.empty()) {
            for (auto sync_group : synchronized_groups) {
                if (!sync_group->sockets.empty()) {
                    auto socket_in_other = sync_group->sockets[id];
                    if (!socket_in_other->directly_linked_links.empty()) {
                        can_delete = false;
                    }
                }
            }

            if (!can_delete)
                return;
            else {
                for (auto sync_group : synchronized_groups) {
                    sync_group->remove_socket(socket_identifier, false);
                }
            }
        }

        sockets.erase(it);
        node->refresh_node();
    }
    else
        throw std::runtime_error(
            "Socket not found when deleting from a group.");
    if (need_to_propagate_sync)
        for (auto sync_group : synchronized_groups) {
            assert(sync_group->sockets.size() == sockets.size());
        }
}

void SocketGroup::remove_socket(NodeSocket* socket, bool need_to_propagate_sync)
{
    auto it = std::find(sockets.begin(), sockets.end(), socket);
    if (it != sockets.end()) {
        if (need_to_propagate_sync && !synchronized_groups.empty()) {
            size_t index = std::distance(sockets.begin(), it);
            for (auto sync_group : synchronized_groups) {
                auto socket_in_other = sync_group->sockets[index];
                if (!socket_in_other->directly_linked_links.empty()) {
                    return;
                }
            }
            for (auto sync_group : synchronized_groups) {
                sync_group->remove_socket(socket, false);
            }
        }

        sockets.erase(it);
        node->refresh_node();
    }
}

void SocketGroup::serialize(nlohmann::json& value)
{
    if (synchronized_groups.empty()) {
        return;
    }

    auto& group = value["socket_groups"][identifier];

    int i = 0;
    for (auto other_group : synchronized_groups) {
        auto other_group_node_id = other_group->node->ID.Get();
        auto other_group_inout = other_group->kind;
        auto other_group_name = other_group->identifier;

        group["synchronized_groups"][std::to_string(i)] = {
            { "node_id", other_group_node_id },
            { "in_out", other_group_inout },
            { "name", other_group_name },
        };
        ++i;
    }
}

void SocketGroup::deserialize(const nlohmann::json& json)
{
    if (!json.contains("socket_groups")) {
        return;
    }
    auto& group = json["socket_groups"][identifier];

    for (int i = 0; i < group["synchronized_groups"].size(); ++i) {
        auto& other_group = group["synchronized_groups"][std::to_string(i)];
        auto other_group_node_id = other_group["node_id"].get<unsigned>();
        auto other_group_inout = other_group["in_out"].get<PinKind>();
        auto other_group_name = other_group["name"].get<std::string>();

        Node* other_group_node = nullptr;
        if (this->node->ID == NodeId(other_group_node_id)) {
            other_group_node = this->node;
        }
        else {
            other_group_node = node->tree_->find_node(other_group_node_id);
        }

        if (other_group_node) {
            SocketGroup* other_group_ptr = other_group_node->find_socket_group(
                other_group_name, other_group_inout);

            add_sync_group(other_group_ptr);
        }
    }
}

RUZINO_NAMESPACE_CLOSE_SCOPE

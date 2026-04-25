#pragma once

#include "GCore/Components.h"
#include "GCore/GOP.h"
#include "GCore/api.h"
#include "GCore/geom_payload.hpp"
#include "openvdb/openvdb.h"

RUZINO_NAMESPACE_OPEN_SCOPE
class GEOMETRY_API VolumeComponent : public GeometryComponent {
   public:
    explicit VolumeComponent(Geometry* attached_operand)
        : GeometryComponent(attached_operand)
    {
    }

    GeometryComponentHandle copy(Geometry* operand) const override;
    std::string to_string() const override;
    
    size_t hash() const override
    {
        // Simple hash based on grid names and basic properties
        size_t h = 0;
        for (const auto& [name, grid] : grids_) {
            h ^= std::hash<std::string>{}(name) + 0x9e3779b9 + (h << 6) + (h >> 2);
            if (grid) {
                h ^= std::hash<size_t>{}(grid->activeVoxelCount()) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
        }
        return h;
    }
    
    void add_grid(openvdb::FloatGrid::Ptr grid);

    void apply_transform(const glm::mat4& transform) override;
    void write_disk(const std::string& file_name) const;
    openvdb::GridBase::Ptr get_grid();

   private:
    std::unordered_map<std::string, openvdb::FloatGrid::Ptr> grids_;
    openvdb::FloatGrid::Ptr grid;
};
RUZINO_NAMESPACE_CLOSE_SCOPE

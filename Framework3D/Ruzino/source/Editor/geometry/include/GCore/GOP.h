#pragma once

#include <memory>
#include <string>
#include <vector>

#include "GCore/api.h"

RUZINO_NAMESPACE_OPEN_SCOPE

#define USE_USD_SCRATCH_BUFFER 0

class Stage;
struct GeometryComponent;
class Geometry;
using GeometryHandle = std::shared_ptr<Geometry>;
using GeometryComponentHandle = std::shared_ptr<GeometryComponent>;

class GEOMETRY_API Geometry {
   public:
    Geometry();

    virtual ~Geometry();
    void apply_transform();

    Geometry(const Geometry& operand);
    Geometry(Geometry&& operand) noexcept;

    Geometry& operator=(const Geometry& operand);
    Geometry& operator=(Geometry&& operand) noexcept;

    static Geometry CreateMesh();
    static Geometry CreatePoints();
    static Geometry CreateCurve();
#ifdef GEOM_USD_EXTENSION
    static Geometry CreateVolume();
#endif

    friend bool operator==(const Geometry& lhs, const Geometry& rhs)
    {
        return lhs.components_ == rhs.components_;
        //&& lhs.stage == rhs.stage;
    }

    friend bool operator!=(const Geometry& lhs, const Geometry& rhs)
    {
        return !(lhs == rhs);
    }

    virtual std::string to_string() const;

    [[nodiscard]] size_t hash() const;

    // Non-const version: may trigger copy-on-write
    template<typename OperandType>
    std::shared_ptr<OperandType> get_component(size_t idx = 0);

    // Const version: read-only on Geometry itself, but returns mutable
    // component (for backward compatibility)
    template<typename OperandType>
    std::shared_ptr<const OperandType> get_component(size_t idx = 0) const;

    template<typename OperandType>
    std::shared_ptr<const OperandType> get_const_component(size_t idx = 0);

    void attach_component(const GeometryComponentHandle& component);
    void detach_component(const GeometryComponentHandle& component);

    [[nodiscard]] const std::vector<GeometryComponentHandle>& get_components()
        const
    {
        return *components_;
    }

   protected:
    void detach_shared_components();

    std::shared_ptr<std::vector<GeometryComponentHandle>> components_;
};

template<typename OperandType>
std::shared_ptr<OperandType> Geometry::get_component(size_t idx)
{
    detach_shared_components();

    size_t counter = 0;
    for (int i = 0; i < components_->size(); ++i) {
        auto ptr = std::dynamic_pointer_cast<OperandType>((*components_)[i]);
        if (ptr) {
            if (counter < idx) {
                counter++;
            }
            else {
                return ptr;
            }
        }
    }
    return nullptr;
}

template<typename OperandType>
std::shared_ptr<const OperandType> Geometry::get_component(size_t idx) const
{
    // Const version doesn't trigger COW, just returns the component
    size_t counter = 0;
    for (int i = 0; i < components_->size(); ++i) {
        auto ptr = std::dynamic_pointer_cast<OperandType>((*components_)[i]);
        if (ptr) {
            if (counter < idx) {
                counter++;
            }
            else {
                return ptr;
            }
        }
    }
    return nullptr;
}

template<typename OperandType>
std::shared_ptr<const OperandType> Geometry::get_const_component(size_t idx)
{
    return const_cast<const Geometry*>(this)->get_component<OperandType>(idx);
}

RUZINO_NAMESPACE_CLOSE_SCOPE

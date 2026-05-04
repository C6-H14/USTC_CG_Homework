#include "GCore/GOP.h"

#include <spdlog/spdlog.h>

#include <string>

#include "GCore/Components.h"
#include "GCore/Components/CurveComponent.h"
#include "GCore/Components/MeshComponent.h"
#include "GCore/Components/PointsComponent.h"
#include "GCore/Components/XformComponent.h"
#ifdef GEOM_USD_EXTENSION
#include "GCore/Components/VolumeComponent.h"
#endif

RUZINO_NAMESPACE_OPEN_SCOPE
Geometry::Geometry()
    : components_(std::make_shared<std::vector<GeometryComponentHandle>>())
{
}

Geometry::~Geometry()
{
}

void Geometry::apply_transform()
{
    auto xform_component = get_const_component<XformComponent>();
    if (!xform_component || xform_component->is_identity()) {
        return;
    }

    auto transform = xform_component->get_transform();

    for (auto&& component : *components_) {
        if (component) {
            component->apply_transform(transform);
        }
    }
}

Geometry::Geometry(const Geometry& operand)
{
    *(this) = operand;
}

Geometry::Geometry(Geometry&& operand) noexcept
{
    *(this) = std::move(operand);
}

Geometry& Geometry::operator=(const Geometry& operand)
{
    if (this != &operand) {
        // Simple shallow copy - share the components vector
        this->components_ = operand.components_;
    }
    return *this;
}

Geometry& Geometry::operator=(Geometry&& operand) noexcept
{
    this->components_ = std::move(operand.components_);
    return *this;
}

Geometry Geometry::CreateMesh()
{
    Geometry geometry;
    std::shared_ptr<MeshComponent> mesh =
        std::make_shared<MeshComponent>(&geometry);
    geometry.attach_component(mesh);
    return std::move(geometry);
}

Geometry Geometry::CreatePoints()
{
    Geometry geometry;
    std::shared_ptr<PointsComponent> points =
        std::make_shared<PointsComponent>(&geometry);
    geometry.attach_component(points);
    return std::move(geometry);
}

Geometry Geometry::CreateCurve()
{
    Geometry geometry;
    std::shared_ptr<CurveComponent> curve =
        std::make_shared<CurveComponent>(&geometry);
    geometry.attach_component(curve);
    return std::move(geometry);
}

#ifdef GEOM_USD_EXTENSION
Geometry Geometry::CreateVolume()
{
    Geometry geometry;
    std::shared_ptr<VolumeComponent> volume =
        std::make_shared<VolumeComponent>(&geometry);
    geometry.attach_component(volume);
    return std::move(geometry);
}
#endif

std::string Geometry::to_string() const
{
    std::ostringstream out;
    out << "Contains components:\n";
    for (auto&& component : *components_) {
        if (component) {
            out << "    " << component->to_string() << "\n";
        }
    }
    return out.str();
}
size_t Geometry::hash() const
{
    size_t h = 0;
    for (const auto& comp : *components_) {
        h ^= comp->hash() + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    return h;
}

void Geometry::attach_component(const GeometryComponentHandle& component)
{
    // if (component->get_attached_operand() != this) {
    //     spdlog::warn(
    //         "A component should never be attached to two operands, unless you
    //         " "know what you are doing");
    // }
    detach_shared_components();
    component->attached_operand = this;
    components_->push_back(component);
}

void Geometry::detach_component(const GeometryComponentHandle& component)
{
    detach_shared_components();
    auto iter = std::find(components_->begin(), components_->end(), component);
    components_->erase(iter);
}

void Geometry::detach_shared_components()
{
    if (components_.use_count() > 1) {
        // Deep copy when shared
        auto new_components =
            std::make_shared<std::vector<GeometryComponentHandle>>();
        new_components->reserve(components_->size());

        for (auto&& component : *components_) {
            new_components->push_back(component->copy(this));
        }

        components_ = new_components;
    }
}

RUZINO_NAMESPACE_CLOSE_SCOPE

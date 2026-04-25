#include "GCore/Components.h"
#include "global_stage.hpp"

RUZINO_NAMESPACE_OPEN_SCOPE

GeometryComponent::~GeometryComponent()
{
}

GeometryComponent::GeometryComponent(Geometry* attached_operand)
    : attached_operand(attached_operand)
{
}

Geometry* GeometryComponent::get_attached_operand() const
{
    return attached_operand;
}

RUZINO_NAMESPACE_CLOSE_SCOPE

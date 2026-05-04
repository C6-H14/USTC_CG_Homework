#pragma once

#include <string>

#include "GCore/api.h"
#include "GOP.h"

RUZINO_NAMESPACE_OPEN_SCOPE

Geometry GEOMETRY_API read_obj_geometry(const std::string& path);

RUZINO_NAMESPACE_CLOSE_SCOPE

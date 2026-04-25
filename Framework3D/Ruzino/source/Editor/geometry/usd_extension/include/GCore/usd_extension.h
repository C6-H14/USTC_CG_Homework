#pragma once

#include <pxr/usd/usd/stage.h>

#include "GCore/GOP.h"
#include "GCore/api.h"

RUZINO_NAMESPACE_OPEN_SCOPE class Stage;

void GEOMETRY_API init(Stage* stage);

// 从 USD prim 读取几何数据到 Geometry 对象
bool GEOMETRY_API read_geometry_from_usd(
    Geometry& geometry,
    const pxr::UsdPrim& prim,
    pxr::UsdTimeCode time);

// 将 Geometry 对象写入到 USD stage
bool GEOMETRY_API write_geometry_to_usd(
    const Geometry& geometry,
    pxr::UsdStageRefPtr stage,
    const pxr::SdfPath& sdf_path,
    pxr::UsdTimeCode time);

RUZINO_NAMESPACE_CLOSE_SCOPE

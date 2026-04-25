#pragma once

#include <memory>
#include <glm/vec3.hpp>
#ifdef GEOM_USD_EXTENSION
#include <pxr/usd/usd/common.h>
#include <pxr/usd/sdf/path.h>
#endif
struct PickEvent {
    glm::vec3 point;
    glm::vec3 normal;
#ifdef GEOM_USD_EXTENSION
    pxr::SdfPath prim_path;
    pxr::SdfPath instancer_path;
#endif

    PickEvent(
        const glm::vec3& pt,
        const glm::vec3& norm
#ifdef GEOM_USD_EXTENSION
        ,
        const pxr::SdfPath& path,
        const pxr::SdfPath& inst
#endif
        )
        : point(pt),
          normal(norm)
#ifdef GEOM_USD_EXTENSION
          ,
          prim_path(path),
          instancer_path(inst)
#endif

    {
    }
};

struct GeomPayload {
#ifdef GEOM_USD_EXTENSION
    pxr::UsdStageRefPtr stage;
    pxr::UsdTimeCode current_time = pxr::UsdTimeCode(0);
    pxr::SdfPath prim_path;
#endif

    std::shared_ptr<PickEvent> pick;
    float delta_time = 0.0f;
    bool has_simulation = false;
    bool is_simulating = false;

    std::string stage_filepath_;
};

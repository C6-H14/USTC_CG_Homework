#pragma once

#include "GCore/api.h"


RUZINO_NAMESPACE_OPEN_SCOPE
struct PickEvent;
struct GeomNodeGlobalParams {
    pxr::SdfPath prim_path;
    PickEvent* pick;
};
RUZINO_NAMESPACE_CLOSE_SCOPE
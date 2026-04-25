#pragma once

// Unified header that includes all mesh view types
// This is a convenience header - you can also include individual view headers
// directly:
// - MeshIGLView.h for Eigen/IGL views
// - MeshUSDView.h for USD views (requires GEOM_USD_EXTENSION)
// - MeshCUDAView.h for CUDA views (requires RUZINO_WITH_CUDA)
// - MeshNVRHIView.h for NVRHI views

#include "MeshIGLView.h"

#ifdef GEOM_USD_EXTENSION
#include "MeshUSDView.h"
#endif

#if RUZINO_WITH_CUDA
#include "MeshCUDAView.h"
#endif

#include "MeshNVRHIView.h"

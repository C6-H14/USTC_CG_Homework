#pragma once

#include "GCore/GOP.h"

RUZINO_NAMESPACE_OPEN_SCOPE

namespace geom_algorithm {

/// Parameters for tetrahedral mesh generation
struct TetgenParams {
    float quality_ratio = 2.0f;  // Quality ratio constraint (1.0 - 10.0)
    float max_volume = 0.01f;    // Maximum tetrahedron volume (default reduced
                                 // for better quality)
    float min_dihedral_angle =
        10.0f;           // Minimum dihedral angle in degrees (0 - 30)
    bool refine = true;  // Enable mesh refinement
    bool conforming_delaunay =
        true;           // Use conforming Delaunay tetrahedralization
    bool quiet = true;  // Suppress tetgen output
};

/// Generate tetrahedral mesh from a triangulated surface mesh using TetGen
/// @param geometry Input geometry containing a triangulated surface mesh
/// @param params TetGen parameters controlling mesh quality and behavior
/// @return New geometry containing the tetrahedral mesh (as triangle soup)
GEOMETRY_API Geometry tetrahedralize(
    const Geometry& geometry,
    const TetgenParams& params = TetgenParams());

}  // namespace geom_algorithm

RUZINO_NAMESPACE_CLOSE_SCOPE

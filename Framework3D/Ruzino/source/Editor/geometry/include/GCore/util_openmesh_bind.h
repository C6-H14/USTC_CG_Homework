#pragma once

#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
#include <OpenVolumeMesh/Mesh/PolyhedralMesh.hh>
#include <OpenVolumeMesh/Mesh/TetrahedralMesh.hh>
#include <memory>

#include "GCore/GOP.h"

RUZINO_NAMESPACE_OPEN_SCOPE
using PolyMesh = OpenMesh::PolyMesh_ArrayKernelT<>;
using VolumeMesh = OpenVolumeMesh::GeometricTetrahedralMeshV3d;

// Surface mesh binding
GEOMETRY_API std::shared_ptr<PolyMesh> operand_to_openmesh(
    Geometry* mesh_oeprand);

GEOMETRY_API std::shared_ptr<Geometry> openmesh_to_operand(PolyMesh* openmesh);

// Volume mesh binding - simplified to only support tetrahedral meshes
// Reconstructs tetrahedra from triangular faces that form a tetrahedral complex
GEOMETRY_API std::shared_ptr<VolumeMesh> operand_to_openvolumemesh(
    const Geometry* mesh_operand);

GEOMETRY_API std::shared_ptr<Geometry> openvolumemesh_to_operand(
    VolumeMesh* volumemesh);

// Helper function: Convert OpenMesh to VolumeMesh
GEOMETRY_API std::shared_ptr<VolumeMesh> openmesh_to_volumemesh(
    PolyMesh* openmesh);

RUZINO_NAMESPACE_CLOSE_SCOPE

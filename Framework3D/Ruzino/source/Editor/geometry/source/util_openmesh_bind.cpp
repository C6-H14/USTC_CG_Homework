#include "GCore/util_openmesh_bind.h"

#include <unordered_map>
#include <unordered_set>

#include "GCore/Components/MeshComponent.h"

RUZINO_NAMESPACE_OPEN_SCOPE
std::shared_ptr<PolyMesh> operand_to_openmesh(Geometry* mesh_oeprand)
{
    auto openmesh = std::make_shared<PolyMesh>();
    auto topology = mesh_oeprand->get_component<MeshComponent>();

    // Get mesh data
    const auto& vertices = topology->get_vertices();
    for (const auto& vv : vertices) {
        OpenMesh::Vec3f v;
        v[0] = vv[0];
        v[1] = vv[1];
        v[2] = vv[2];
        openmesh->add_vertex(v);
    }

    auto faceVertexIndices = topology->get_face_vertex_indices();
    auto faceVertexCounts = topology->get_face_vertex_counts();
    auto normals = topology->get_normals();
    auto texcoords = topology->get_texcoords_array();
    auto colors = topology->get_display_color();

    bool hasNormals = !normals.empty();
    bool perVertexNormals = hasNormals && (normals.size() == vertices.size());
    bool hasTexcoords = !texcoords.empty();
    bool perVertexTexcoords =
        hasTexcoords && (texcoords.size() == vertices.size());
    bool hasColors = !colors.empty();
    bool perVertexColors = hasColors && (colors.size() == vertices.size());

    // Request vertex normals, texcoords, and colors only when they exist
    if (hasNormals)
        openmesh->request_vertex_normals();
    if (hasTexcoords)
        openmesh->request_vertex_texcoords2D();
    if (hasColors)
        openmesh->request_vertex_colors();

    int vertexIndex = 0;
    for (int i = 0; i < faceVertexCounts.size(); i++) {
        // Create a vector of vertex handles for the face
        std::vector<PolyMesh::VertexHandle> face_vhandles;
        for (int j = 0; j < faceVertexCounts[i]; j++) {
            int index = faceVertexIndices[vertexIndex];
            // Get the vertex handle from the index
            PolyMesh::VertexHandle vh = openmesh->vertex_handle(index);
            // Add it to the vector
            face_vhandles.push_back(vh);

            // Set normal if available
            if (hasNormals) {
                if (perVertexNormals) {
                    // Use per-vertex normals
                    OpenMesh::Vec3f n(
                        normals[index][0],
                        normals[index][1],
                        normals[index][2]);
                    openmesh->set_normal(vh, n);
                }
                else {
                    // Use per-face-vertex normals
                    OpenMesh::Vec3f n(
                        normals[vertexIndex][0],
                        normals[vertexIndex][1],
                        normals[vertexIndex][2]);
                    openmesh->set_normal(vh, n);
                }
            }

            // Set texcoords if available
            if (hasTexcoords) {
                if (perVertexTexcoords) {
                    // Use per-vertex texcoords
                    OpenMesh::Vec2f t(texcoords[index][0], texcoords[index][1]);
                    openmesh->set_texcoord2D(vh, t);
                }
                else {
                    // Use per-face-vertex texcoords
                    OpenMesh::Vec2f t(
                        texcoords[vertexIndex][0], texcoords[vertexIndex][1]);
                    openmesh->set_texcoord2D(vh, t);
                }
            }

            // Set colors if available
            if (hasColors) {
                if (perVertexColors) {
                    // Use per-vertex colors
                    OpenMesh::Vec3f c(
                        colors[index][0], colors[index][1], colors[index][2]);
                    openmesh->set_color(
                        vh,
                        PolyMesh::Color(
                            static_cast<unsigned char>(c[0] * 255),
                            static_cast<unsigned char>(c[1] * 255),
                            static_cast<unsigned char>(c[2] * 255)));
                }
                else {
                    // Use per-face-vertex colors
                    OpenMesh::Vec3f c(
                        colors[vertexIndex][0],
                        colors[vertexIndex][1],
                        colors[vertexIndex][2]);
                    openmesh->set_color(
                        vh,
                        PolyMesh::Color(
                            static_cast<unsigned char>(c[0] * 255),
                            static_cast<unsigned char>(c[1] * 255),
                            static_cast<unsigned char>(c[2] * 255)));
                }
            }

            vertexIndex++;
        }
        // Add the face to the mesh
        openmesh->add_face(face_vhandles);
    }

    if (hasNormals) {
        // Update or compute vertex normals if necessary
        openmesh->update_normals();
    }

    return openmesh;
}
std::shared_ptr<Geometry> openmesh_to_operand(PolyMesh* openmesh)
{
    auto geometry = std::make_shared<Geometry>();
    std::shared_ptr<MeshComponent> mesh =
        std::make_shared<MeshComponent>(geometry.get());
    geometry->attach_component(mesh);

    std::vector<glm::vec3> points;
    std::vector<int> faceVertexIndices;
    std::vector<int> faceVertexCounts;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec3> colors;

    bool hasNormals = openmesh->has_vertex_normals();
    bool hasTexcoords = openmesh->has_vertex_texcoords2D();
    bool hasColors = openmesh->has_vertex_colors();

    if (hasNormals && !openmesh->has_vertex_normals())
        openmesh->request_vertex_normals();
    if (hasTexcoords && !openmesh->has_vertex_texcoords2D())
        openmesh->request_vertex_texcoords2D();
    if (hasColors && !openmesh->has_vertex_colors())
        openmesh->request_vertex_colors();

    // Ensure normals are updated
    if (hasNormals)
        openmesh->update_normals();

    // Set the points
    for (const auto& v : openmesh->vertices()) {
        const auto& p = openmesh->point(v);
        points.push_back(glm::vec3(p[0], p[1], p[2]));
    }

    // Determine if we should use per-vertex attributes
    bool usePerVertexNormals = hasNormals;
    bool usePerVertexTexcoords = hasTexcoords;
    bool usePerVertexColors = hasColors;

    // If using per-vertex attributes, populate them now
    if (usePerVertexNormals) {
        for (const auto& v : openmesh->vertices()) {
            const auto& n = openmesh->normal(v);
            normals.push_back(glm::vec3(n[0], n[1], n[2]));
        }
    }

    if (usePerVertexTexcoords) {
        for (const auto& v : openmesh->vertices()) {
            const auto& t = openmesh->texcoord2D(v);
            texcoords.push_back(glm::vec2(t[0], t[1]));
        }
    }

    if (usePerVertexColors) {
        for (const auto& v : openmesh->vertices()) {
            const auto& c = openmesh->color(v);
            colors.push_back(
                glm::vec3(c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f));
        }
    }

    // If we need per-face-vertex attributes, prepare for them
    std::vector<glm::vec3> faceVertexNormals;
    std::vector<glm::vec2> faceVertexTexcoords;
    std::vector<glm::vec3> faceVertexColors;
    bool hasFaceVertexNormals = hasNormals && !usePerVertexNormals;
    bool hasFaceVertexTexcoords = hasTexcoords && !usePerVertexTexcoords;
    bool hasFaceVertexColors = hasColors && !usePerVertexColors;

    // Set the topology
    for (const auto& f : openmesh->faces()) {
        size_t count = 0;
        for (const auto& vf : f.vertices()) {
            faceVertexIndices.push_back(vf.idx());
            count += 1;

            // If we need per-face-vertex attributes, collect them here
            if (hasFaceVertexNormals) {
                const auto& n = openmesh->normal(vf);
                faceVertexNormals.push_back(glm::vec3(n[0], n[1], n[2]));
            }

            if (hasFaceVertexTexcoords) {
                const auto& t = openmesh->texcoord2D(vf);
                faceVertexTexcoords.push_back(glm::vec2(t[0], t[1]));
            }

            if (hasFaceVertexColors) {
                const auto& c = openmesh->color(vf);
                faceVertexColors.push_back(
                    glm::vec3(c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f));
            }
        }
        faceVertexCounts.push_back(count);
    }

    mesh->set_vertices(points);
    mesh->set_face_vertex_indices(faceVertexIndices);
    mesh->set_face_vertex_counts(faceVertexCounts);

    if (hasNormals) {
        mesh->set_normals(usePerVertexNormals ? normals : faceVertexNormals);
    }

    if (hasTexcoords) {
        mesh->set_texcoords_array(
            usePerVertexTexcoords ? texcoords : faceVertexTexcoords);
    }

    if (hasColors) {
        mesh->set_display_color(usePerVertexColors ? colors : faceVertexColors);
    }

    return geometry;
}

// Optimized tetrahedral mesh reconstruction helpers
namespace {
// Fast hash function for triangle sorting
inline uint64_t hash_triangle(int v0, int v1, int v2)
{
    // Sort vertices to ensure consistent hashing
    if (v0 > v1)
        std::swap(v0, v1);
    if (v1 > v2)
        std::swap(v1, v2);
    if (v0 > v1)
        std::swap(v0, v1);

    // Use a simple but effective hash: combine indices with shifts
    return (static_cast<uint64_t>(v0) << 40) |
           (static_cast<uint64_t>(v1) << 20) | static_cast<uint64_t>(v2);
}

// Optimized tetrahedral reconstruction using hash maps
void fast_tetrahedral_reconstruction(
    const std::vector<std::array<int, 3>>& triangles,
    std::shared_ptr<OpenVolumeMesh::GeometricTetrahedralMeshV3d> volumemesh)
{
    if (triangles.size() < 4)
        return;

    // Pre-compute triangle hashes for O(1) lookup
    std::unordered_set<uint64_t> triangle_set;
    std::unordered_map<uint64_t, size_t> triangle_index_map;
    triangle_set.reserve(triangles.size());
    for (size_t idx = 0; idx < triangles.size(); ++idx) {
        const auto& tri = triangles[idx];
        uint64_t tri_hash = hash_triangle(tri[0], tri[1], tri[2]);
        triangle_set.insert(tri_hash);
        triangle_index_map[tri_hash] = idx;
    }

    // Track processed tetrahedra to avoid duplicates
    std::unordered_set<uint64_t> processed_tets;
    processed_tets.reserve(triangles.size() / 2);

    // Build edge-to-triangles mapping for efficient adjacency lookup
    std::unordered_map<uint64_t, std::vector<size_t>> edge_to_triangles;
    edge_to_triangles.reserve(triangles.size() * 3);

    auto encode_edge = [](int v1, int v2) -> uint64_t {
        if (v1 > v2)
            std::swap(v1, v2);
        return (static_cast<uint64_t>(v1) << 32) | static_cast<uint64_t>(v2);
    };

    for (size_t i = 0; i < triangles.size(); ++i) {
        const auto& tri = triangles[i];
        for (int j = 0; j < 3; ++j) {
            uint64_t edge_key = encode_edge(tri[j], tri[(j + 1) % 3]);
            edge_to_triangles[edge_key].push_back(i);
        }
    }

    // Helper: Normalize triangle for hash comparison
    auto normalize_tri = [](int v0, int v1, int v2) -> std::array<int, 3> {
        std::array<int, 3> tri = { v0, v1, v2 };
        std::sort(tri.begin(), tri.end());
        return tri;
    };

    // For each triangle, find candidate fourth vertices
    for (size_t i = 0; i < triangles.size(); ++i) {
        const auto& base_tri = triangles[i];
        std::vector<int> candidate_vertices;

        // Collect candidate vertices from adjacent triangles via edges
        for (int j = 0; j < 3; ++j) {
            uint64_t edge_key = encode_edge(base_tri[j], base_tri[(j + 1) % 3]);
            auto it = edge_to_triangles.find(edge_key);
            if (it != edge_to_triangles.end()) {
                for (size_t adj_idx : it->second) {
                    if (adj_idx != i) {
                        const auto& adj_tri = triangles[adj_idx];
                        for (int k = 0; k < 3; ++k) {
                            int vertex = adj_tri[k];
                            if (vertex != base_tri[j] &&
                                vertex != base_tri[(j + 1) % 3]) {
                                candidate_vertices.push_back(vertex);
                            }
                        }
                    }
                }
            }
        }

        // Remove duplicates from candidates
        std::sort(candidate_vertices.begin(), candidate_vertices.end());
        candidate_vertices.erase(
            std::unique(candidate_vertices.begin(), candidate_vertices.end()),
            candidate_vertices.end());

        // Test each candidate for valid tetrahedron
        for (int fourth_vertex : candidate_vertices) {
            // Use original order for orientation preservation
            std::array<int, 4> tet_verts = {
                base_tri[0], base_tri[1], base_tri[2], fourth_vertex
            };

            // Compute hash for duplicate checking (use normalized version)
            auto sorted_tet = tet_verts;
            std::sort(sorted_tet.begin(), sorted_tet.end());
            uint64_t tet_hash = 0;
            for (int v : sorted_tet) {
                tet_hash = tet_hash * 6364136223846793005ULL +
                           static_cast<uint64_t>(v);
            }

            if (processed_tets.count(tet_hash))
                continue;

            // Verify all 4 faces exist in the triangle set
            bool valid_tet = true;
            std::array<std::array<int, 3>, 4> faces = {
                { { tet_verts[0], tet_verts[1], tet_verts[2] },
                  { tet_verts[0], tet_verts[1], tet_verts[3] },
                  { tet_verts[0], tet_verts[2], tet_verts[3] },
                  { tet_verts[1], tet_verts[2], tet_verts[3] } }
            };

            for (const auto& face : faces) {
                if (triangle_set.find(hash_triangle(
                        face[0], face[1], face[2])) == triangle_set.end()) {
                    valid_tet = false;
                    break;
                }
            }

            if (valid_tet) {
                processed_tets.insert(tet_hash);
                std::vector<OpenVolumeMesh::VertexHandle> tet_handles;
                tet_handles.reserve(4);
                // Preserve original order for proper orientation and adjacency
                for (int v : tet_verts) {
                    tet_handles.emplace_back(v);
                }

                // Use permissive mode directly to avoid topology warnings
                // Since tetrahedral meshes inherently have shared faces between
                // adjacent cells
                auto cell_handle = volumemesh->add_cell(tet_handles, false);
            }
        }
    }
}
}  // namespace

std::shared_ptr<VolumeMesh> operand_to_openvolumemesh(
    const Geometry* mesh_operand)
{
    auto volumemesh = std::make_shared<VolumeMesh>();
    if (!mesh_operand)
        return volumemesh;

    auto topology = mesh_operand->get_component<MeshComponent>();
    if (!topology)
        return volumemesh;

    // Get mesh data
    const auto& vertices = topology->get_vertices();
    auto faceVertexIndices = topology->get_face_vertex_indices();
    auto faceVertexCounts = topology->get_face_vertex_counts();

    if (vertices.empty() || faceVertexIndices.empty()) {
        return volumemesh;
    }

    // Add vertices to volume mesh first
    for (const auto& vv : vertices) {
        VolumeMesh::PointT point(vv[0], vv[1], vv[2]);
        volumemesh->add_vertex(point);
    }

    // Check if input represents tetrahedra as 4-vertex "faces" (cells)
    bool allQuads = true;
    for (const auto& count : faceVertexCounts) {
        if (count != 4) {
            allQuads = false;
            break;
        }
    }

    if (allQuads && !faceVertexCounts.empty()) {
        // Fast path: Input data represents tetrahedra as 4-vertex cells
        size_t vertexIndex = 0;
        for (size_t i = 0; i < faceVertexCounts.size(); i++) {
            std::vector<OpenVolumeMesh::VertexHandle> tet_vertices;
            tet_vertices.reserve(4);

            bool valid_tet = true;
            for (int j = 0; j < 4; j++) {
                int index = faceVertexIndices[vertexIndex + j];
                if (index >= 0 && index < static_cast<int>(vertices.size())) {
                    tet_vertices.push_back(OpenVolumeMesh::VertexHandle(index));
                }
                else {
                    valid_tet = false;
                    break;
                }
            }

            if (valid_tet && tet_vertices.size() == 4) {
                // Fast path: Input is already 4-vertex cells
                // Use topology check = false since faces are already present
                volumemesh->add_cell(tet_vertices, false);
            }
            vertexIndex += 4;
        }
        return volumemesh;
    }

    // Handle triangular faces - direct approach without OpenMesh intermediate
    std::vector<std::array<int, 3>> triangles;
    triangles.reserve(faceVertexCounts.size());

    size_t vertexIndex = 0;
    for (size_t i = 0; i < faceVertexCounts.size(); i++) {
        if (faceVertexCounts[i] == 3) {
            triangles.push_back(
                { faceVertexIndices[vertexIndex],
                  faceVertexIndices[vertexIndex + 1],
                  faceVertexIndices[vertexIndex + 2] });
        }
        vertexIndex += faceVertexCounts[i];
    }

    if (triangles.empty()) {
        return volumemesh;
    }

    // Special case: single tetrahedron (4 triangles, 4 vertices)
    if (triangles.size() == 4 && vertices.size() == 4) {
        std::vector<OpenVolumeMesh::VertexHandle> tet_vertices;
        for (size_t k = 0; k < 4; k++) {
            tet_vertices.push_back(
                OpenVolumeMesh::VertexHandle(static_cast<int>(k)));
        }
        volumemesh->add_cell(tet_vertices);
        return volumemesh;
    }

    // Use direct tetrahedral reconstruction (more robust than OpenMesh
    // approach)
    fast_tetrahedral_reconstruction(triangles, volumemesh);

    return volumemesh;
}

std::shared_ptr<Geometry> openvolumemesh_to_operand(VolumeMesh* volumemesh)
{
    auto geometry = std::make_shared<Geometry>();
    std::shared_ptr<MeshComponent> mesh =
        std::make_shared<MeshComponent>(geometry.get());
    geometry->attach_component(mesh);

    std::vector<glm::vec3> points;
    std::vector<int> faceVertexIndices;
    std::vector<int> faceVertexCounts;

    // Extract vertices
    for (auto v_it = volumemesh->vertices_begin();
         v_it != volumemesh->vertices_end();
         ++v_it) {
        const auto& point = volumemesh->vertex(*v_it);
        points.push_back(glm::vec3(point[0], point[1], point[2]));
    }

    // Extract cells as face_vertex data
    // For volumetric meshes, represent each cell (e.g., tetrahedron) as a
    // single entry
    for (auto c_it = volumemesh->cells_begin(); c_it != volumemesh->cells_end();
         ++c_it) {
        std::vector<int> cell_verts;

        // Iterate through cell vertices
        for (auto cv_it = volumemesh->cv_iter(*c_it); cv_it.valid(); ++cv_it) {
            cell_verts.push_back((*cv_it).idx());
        }

        // Add the cell as a single face with N vertices (e.g., 4 for
        // tetrahedron)
        faceVertexCounts.push_back(static_cast<int>(cell_verts.size()));
        for (int vid : cell_verts) {
            faceVertexIndices.push_back(vid);
        }
    }

    mesh->set_vertices(points);
    mesh->set_face_vertex_indices(faceVertexIndices);
    mesh->set_face_vertex_counts(faceVertexCounts);

    return geometry;
}

// Convert OpenMesh to VolumeMesh using half-edge structure for efficient
// processing
std::shared_ptr<VolumeMesh> openmesh_to_volumemesh(PolyMesh* openmesh)
{
    auto volumemesh = std::make_shared<VolumeMesh>();
    if (!openmesh || openmesh->n_faces() == 0) {
        return volumemesh;
    }

    // Add vertices to volume mesh
    for (const auto& v : openmesh->vertices()) {
        const auto& p = openmesh->point(v);
        VolumeMesh::PointT point(p[0], p[1], p[2]);
        volumemesh->add_vertex(point);
    }

    // Special case: single tetrahedron (4 faces, 4 vertices)
    if (openmesh->n_faces() == 4 && openmesh->n_vertices() == 4) {
        std::vector<OpenVolumeMesh::VertexHandle> tet_vertices;
        for (int i = 0; i < 4; i++) {
            tet_vertices.push_back(OpenVolumeMesh::VertexHandle(i));
        }
        volumemesh->add_cell(tet_vertices);
        return volumemesh;
    }

    // Use efficient algorithm based on edge adjacency
    std::unordered_map<uint64_t, std::vector<PolyMesh::FaceHandle>>
        edge_to_faces;

    auto encode_edge = [](int v1, int v2) -> uint64_t {
        if (v1 > v2)
            std::swap(v1, v2);
        return (static_cast<uint64_t>(v1) << 32) | static_cast<uint64_t>(v2);
    };

    // Build edge-to-faces mapping using OpenMesh's efficient iteration
    std::vector<std::array<int, 3>> all_triangles;
    all_triangles.reserve(openmesh->n_faces());

    for (const auto& f : openmesh->faces()) {
        std::vector<int> face_verts;
        for (const auto& fv : f.vertices()) {
            face_verts.push_back(fv.idx());
        }

        if (face_verts.size() == 3) {  // Only process triangles
            std::array<int, 3> triangle = { face_verts[0],
                                            face_verts[1],
                                            face_verts[2] };
            all_triangles.push_back(triangle);

            for (int i = 0; i < 3; i++) {
                int v1 = face_verts[i];
                int v2 = face_verts[(i + 1) % 3];
                uint64_t edge_key = encode_edge(v1, v2);
                edge_to_faces[edge_key].push_back(f);
            }
        }
    }

    if (all_triangles.empty()) {
        return volumemesh;
    }

    // Use the optimized tetrahedral reconstruction from namespace
    fast_tetrahedral_reconstruction(all_triangles, volumemesh);

    return volumemesh;
}

RUZINO_NAMESPACE_CLOSE_SCOPE

#include "GCore/create_geom.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <vector>

#include "GCore/Components/CurveComponent.h"
#include "GCore/Components/MeshComponent.h"
#include "GCore/Components/PointsComponent.h"
#include "glm/geometric.hpp"

RUZINO_NAMESPACE_OPEN_SCOPE

Geometry create_grid(int resolution, float size)
{
    resolution = resolution + 1;
    Geometry geometry;
    std::shared_ptr<MeshComponent> mesh =
        std::make_shared<MeshComponent>(&geometry);
    geometry.attach_component(mesh);

    std::vector<glm::vec3> points;
    std::vector<glm::vec2> texcoord;
    std::vector<glm::vec3> normals;
    std::vector<int> faceVertexIndices;
    std::vector<int> faceVertexCounts;

    for (int i = 0; i < resolution; ++i) {
        for (int j = 0; j < resolution; ++j) {
            float x =
                size * static_cast<float>(i) / (resolution - 1) - size * 0.5f;
            float y =
                size * static_cast<float>(j) / (resolution - 1) - size * 0.5f;

            float u = static_cast<float>(i) / (resolution - 1);
            float v = static_cast<float>(j) / (resolution - 1);
            points.push_back(glm::vec3(x, y, 0));
            texcoord.push_back(glm::vec2(u, v));
            normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
        }
    }

    for (int i = 0; i < resolution - 1; ++i) {
        for (int j = 0; j < resolution - 1; ++j) {
            faceVertexCounts.push_back(4);
            faceVertexIndices.push_back(i * resolution + j);
            faceVertexIndices.push_back(i * resolution + j + 1);
            faceVertexIndices.push_back((i + 1) * resolution + j + 1);
            faceVertexIndices.push_back((i + 1) * resolution + j);
        }
    }

    mesh->set_vertices(points);
    mesh->set_face_vertex_indices(faceVertexIndices);
    mesh->set_face_vertex_counts(faceVertexCounts);
    mesh->set_texcoords_array(texcoord);
    mesh->set_normals(normals);

    return geometry;
}

Geometry create_circle(int resolution, float radius)
{
    Geometry geometry;
    std::shared_ptr<CurveComponent> curve =
        std::make_shared<CurveComponent>(&geometry);
    geometry.attach_component(curve);

    std::vector<glm::vec3> points;
    glm::vec3 center(0.0f, 0.0f, 0.0f);
    float angleStep = 2.0f * static_cast<float>(M_PI) / resolution;

    for (int i = 0; i < resolution; ++i) {
        float angle = i * angleStep;
        glm::vec3 point(
            radius * std::cos(angle) + center[0],
            radius * std::sin(angle) + center[1],
            center[2]);
        points.push_back(point);
    }

    std::vector<glm::vec3> normals;
    for (int i = 0; i < resolution; ++i) {
        normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
    }

    curve->set_vertices(points);
    curve->set_curve_normals(normals);
    curve->set_vert_count({ resolution });
    curve->set_periodic(true);

    return geometry;
}
Geometry create_circle_face(int resolution, float radius)
{
    Geometry geometry;
    std::shared_ptr<MeshComponent> mesh =
        std::make_shared<MeshComponent>(&geometry);
    geometry.attach_component(mesh);

    std::vector<glm::vec3> points;
    std::vector<glm::vec2> texcoord;
    std::vector<glm::vec3> normals;
    std::vector<int> faceVertexIndices;
    std::vector<int> faceVertexCounts;

    for (int i = 0; i < resolution; ++i) {
        float angle = static_cast<float>(i) * 2.0f * static_cast<float>(M_PI) /
                      resolution;
        float x = radius * std::cos(angle);
        float y = radius * std::sin(angle);
        points.push_back(glm::vec3(x, y, 0.0f));
        texcoord.push_back(
            glm::vec2(0.5f + x / (2.0f * radius), 0.5f + y / (2.0f * radius)));
        normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
    }

    // Center point
    points.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
    texcoord.push_back(glm::vec2(0.5f, 0.5f));
    normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));

    for (int i = 0; i < resolution; ++i) {
        faceVertexCounts.push_back(3);
        faceVertexIndices.push_back(i);
        faceVertexIndices.push_back((i + 1) % resolution);
        faceVertexIndices.push_back(resolution);  // Center point index
    }

    mesh->set_vertices(points);
    mesh->set_face_vertex_indices(faceVertexIndices);
    mesh->set_face_vertex_counts(faceVertexCounts);
    mesh->set_texcoords_array(texcoord);
    mesh->set_normals(normals);

    return geometry;
}

Geometry
create_cylinder_section(float height, float radius, float angle, int resolution)
{
    Geometry geometry;
    std::shared_ptr<MeshComponent> mesh =
        std::make_shared<MeshComponent>(&geometry);
    geometry.attach_component(mesh);

    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoord;
    std::vector<int> faceVertexIndices;
    std::vector<int> faceVertexCounts;

    int rows = resolution;
    int cols = resolution;
    float arcLength = radius * angle;
    float aspectRatio = height / arcLength;
    float uScale, vScale;

    if (arcLength >= height) {
        uScale = 1.0f;
        vScale = height / arcLength;
    }
    else {
        uScale = arcLength / height;
        vScale = 1.0f;
    }

    for (int i = 0; i <= rows; ++i) {
        float rowFactor = static_cast<float>(i) / rows;
        float z = height * rowFactor;

        for (int j = 0; j <= cols; ++j) {
            float colFactor = static_cast<float>(j) / cols;
            float theta = angle * (colFactor - 0.5f);

            float x = radius * std::cos(theta);
            float y = radius * std::sin(theta);

            points.push_back(glm::vec3(x, y, z));

            glm::vec3 normal(x, y, 0);
            normal = glm::normalize(normal);
            normals.push_back(normal);

            float u = colFactor * uScale;
            float v = rowFactor * vScale;
            texcoord.push_back(glm::vec2(u, v));
        }
    }

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            int idx0 = i * (cols + 1) + j;
            int idx1 = idx0 + 1;
            int idx2 = (i + 1) * (cols + 1) + j + 1;
            int idx3 = (i + 1) * (cols + 1) + j;

            faceVertexCounts.push_back(4);
            faceVertexIndices.push_back(idx0);
            faceVertexIndices.push_back(idx1);
            faceVertexIndices.push_back(idx2);
            faceVertexIndices.push_back(idx3);
        }
    }

    mesh->set_vertices(points);
    mesh->set_normals(normals);
    mesh->set_face_vertex_indices(faceVertexIndices);
    mesh->set_face_vertex_counts(faceVertexCounts);
    mesh->set_texcoords_array(texcoord);

    return geometry;
}

Geometry create_spiral(
    int resolution,
    float R1,
    float R2,
    float circle_count,
    float height)
{
    Geometry geometry;
    std::shared_ptr<CurveComponent> curve =
        std::make_shared<CurveComponent>(&geometry);
    geometry.attach_component(curve);

    std::vector<glm::vec3> points;

    float angleStep =
        circle_count * 2.0f * static_cast<float>(M_PI) / resolution;
    float radiusIncrement = (R2 - R1) / resolution;
    float heightIncrement = height / resolution;

    for (int i = 0; i < resolution; ++i) {
        float angle = i * angleStep;
        float radius = R1 + radiusIncrement * i;
        float z = heightIncrement * i;
        glm::vec3 point(radius * std::cos(angle), radius * std::sin(angle), z);
        points.push_back(point);
    }

    std::vector<glm::vec3> normals;
    for (int i = 0; i < resolution; ++i) {
        normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
    }

    curve->set_vertices(points);
    curve->set_vert_count({ resolution });
    curve->set_curve_normals(normals);

    return geometry;
}

Geometry create_uv_sphere(int segments, int rings, float radius)
{
    Geometry geometry;
    std::shared_ptr<MeshComponent> mesh =
        std::make_shared<MeshComponent>(&geometry);
    geometry.attach_component(mesh);

    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoord;
    std::vector<int> faceVertexIndices;
    std::vector<int> faceVertexCounts;

    // Add top vertex
    points.push_back(glm::vec3(0, 0, radius));
    normals.push_back(glm::vec3(0, 0, 1));
    texcoord.push_back(glm::vec2(0.5f, 1.0f));

    // Generate vertices for each ring
    for (int ring = 0; ring < rings - 1; ++ring) {
        float phi = static_cast<float>(M_PI) * (float)(ring + 1) / rings;
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);
        float v = 1.0f - (float)(ring + 1) / rings;

        for (int segment = 0; segment < segments; ++segment) {
            float theta =
                2.0f * static_cast<float>(M_PI) * (float)segment / segments;
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);
            float u = (float)segment / segments;

            float x = radius * sinPhi * cosTheta;
            float y = radius * sinPhi * sinTheta;
            float z = radius * cosPhi;

            points.push_back(glm::vec3(x, y, z));
            normals.push_back(glm::vec3(x / radius, y / radius, z / radius));
            texcoord.push_back(glm::vec2(u, v));
        }
    }

    // Add bottom vertex
    points.push_back(glm::vec3(0, 0, -radius));
    normals.push_back(glm::vec3(0, 0, -1));
    texcoord.push_back(glm::vec2(0.5f, 0.0f));

    // Add top cap faces
    for (int segment = 0; segment < segments; ++segment) {
        faceVertexCounts.push_back(3);
        faceVertexIndices.push_back(0);
        faceVertexIndices.push_back(1 + (segment + 1) % segments);
        faceVertexIndices.push_back(1 + segment);
    }

    // Add middle faces
    for (int ring = 0; ring < rings - 2; ++ring) {
        int ringStart = 1 + ring * segments;
        int nextRingStart = 1 + (ring + 1) * segments;
        for (int segment = 0; segment < segments; ++segment) {
            int nextSegment = (segment + 1) % segments;

            faceVertexCounts.push_back(4);
            faceVertexIndices.push_back(ringStart + segment);
            faceVertexIndices.push_back(ringStart + nextSegment);
            faceVertexIndices.push_back(nextRingStart + nextSegment);
            faceVertexIndices.push_back(nextRingStart + segment);
        }
    }

    // Add bottom cap faces
    int bottomVertex = points.size() - 1;
    int lastRingStart = 1 + (rings - 2) * segments;
    for (int segment = 0; segment < segments; ++segment) {
        faceVertexCounts.push_back(3);
        faceVertexIndices.push_back(bottomVertex);
        faceVertexIndices.push_back(lastRingStart + segment);
        faceVertexIndices.push_back(lastRingStart + (segment + 1) % segments);
    }

    mesh->set_vertices(points);
    mesh->set_normals(normals);
    mesh->set_face_vertex_indices(faceVertexIndices);
    mesh->set_face_vertex_counts(faceVertexCounts);
    mesh->set_texcoords_array(texcoord);

    return geometry;
}

Geometry create_ico_sphere(int subdivisions, float radius)
{
    Geometry geometry;
    std::shared_ptr<MeshComponent> mesh =
        std::make_shared<MeshComponent>(&geometry);
    geometry.attach_component(mesh);

    std::vector<glm::vec3> vertices;
    std::vector<std::vector<int>> faces;

    // Create icosahedron base shape
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;

    // Add initial 12 vertices of icosahedron
    vertices.push_back(normalize(glm::vec3(-1, t, 0)));
    vertices.push_back(normalize(glm::vec3(1, t, 0)));
    vertices.push_back(normalize(glm::vec3(-1, -t, 0)));
    vertices.push_back(normalize(glm::vec3(1, -t, 0)));

    vertices.push_back(normalize(glm::vec3(0, -1, t)));
    vertices.push_back(normalize(glm::vec3(0, 1, t)));
    vertices.push_back(normalize(glm::vec3(0, -1, -t)));
    vertices.push_back(normalize(glm::vec3(0, 1, -t)));

    vertices.push_back(normalize(glm::vec3(t, 0, -1)));
    vertices.push_back(normalize(glm::vec3(t, 0, 1)));
    vertices.push_back(normalize(glm::vec3(-t, 0, -1)));
    vertices.push_back(normalize(glm::vec3(-t, 0, 1)));

    // Add initial 20 faces
    faces.push_back({ 0, 11, 5 });
    faces.push_back({ 0, 5, 1 });
    faces.push_back({ 0, 1, 7 });
    faces.push_back({ 0, 7, 10 });
    faces.push_back({ 0, 10, 11 });
    faces.push_back({ 1, 5, 9 });
    faces.push_back({ 5, 11, 4 });
    faces.push_back({ 11, 10, 2 });
    faces.push_back({ 10, 7, 6 });
    faces.push_back({ 7, 1, 8 });
    faces.push_back({ 3, 9, 4 });
    faces.push_back({ 3, 4, 2 });
    faces.push_back({ 3, 2, 6 });
    faces.push_back({ 3, 6, 8 });
    faces.push_back({ 3, 8, 9 });
    faces.push_back({ 4, 9, 5 });
    faces.push_back({ 2, 4, 11 });
    faces.push_back({ 6, 2, 10 });
    faces.push_back({ 8, 6, 7 });
    faces.push_back({ 9, 8, 1 });

    // Helper function to get middle point of two vertices
    std::map<std::pair<int, int>, int> middlePointCache;
    auto getMiddlePoint = [&](int p1, int p2) -> int {
        if (p1 > p2)
            std::swap(p1, p2);
        std::pair<int, int> key(p1, p2);
        auto it = middlePointCache.find(key);
        if (it != middlePointCache.end())
            return it->second;

        glm::vec3 middle = (vertices[p1] + vertices[p2]) * 0.5f;
        int i = vertices.size();
        vertices.push_back(normalize(middle));
        middlePointCache[key] = i;
        return i;
    };

    // Perform subdivision
    for (int i = 0; i < subdivisions; i++) {
        std::vector<std::vector<int>> newFaces;
        for (const auto& face : faces) {
            int a = getMiddlePoint(face[0], face[1]);
            int b = getMiddlePoint(face[1], face[2]);
            int c = getMiddlePoint(face[2], face[0]);

            newFaces.push_back({ face[0], a, c });
            newFaces.push_back({ face[1], b, a });
            newFaces.push_back({ face[2], c, b });
            newFaces.push_back({ a, b, c });
        }
        faces = newFaces;
    }

    // Project vertices to sphere
    for (auto& v : vertices)
        v *= radius;

    // Convert to mesh format
    std::vector<glm::vec3> points(vertices.begin(), vertices.end());
    std::vector<int> faceVertexIndices;
    std::vector<int> faceVertexCounts;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoord;

    for (const auto& face : faces) {
        faceVertexCounts.push_back(face.size());
        for (int idx : face)
            faceVertexIndices.push_back(idx);
    }

    normals.resize(vertices.size());
    texcoord.resize(vertices.size());
    for (size_t i = 0; i < vertices.size(); i++) {
        glm::vec3 normal = normalize(vertices[i]);
        normals[i] = normal;

        float u = 0.5f + std::atan2(normal[1], normal[0]) / (2.0f * M_PI);
        float v = 0.5f - std::asin(normal[2]) / M_PI;
        texcoord[i] = glm::vec2(u, v);
    }

    mesh->set_vertices(points);
    mesh->set_normals(normals);
    mesh->set_face_vertex_indices(faceVertexIndices);
    mesh->set_face_vertex_counts(faceVertexCounts);
    mesh->set_texcoords_array(texcoord);

    return geometry;
}

Geometry create_point(float x, float y, float z, float size)
{
    Geometry geometry;
    std::shared_ptr<PointsComponent> points =
        std::make_shared<PointsComponent>(&geometry);
    geometry.attach_component(points);

    std::vector<glm::vec3> vertices;
    vertices.push_back(glm::vec3(x, y, z));

    std::vector<float> widths;
    widths.push_back(size);

    points->set_vertices(vertices);
    points->set_normals({ glm::vec3(0.0f, 1.0f, 0.0f) });
    points->set_width(widths);

    return geometry;
}

Geometry create_wave_mesh(
    int resolution,
    float size,
    float period_count,
    float wave_height)
{
    Geometry geometry;
    std::shared_ptr<MeshComponent> mesh =
        std::make_shared<MeshComponent>(&geometry);
    geometry.attach_component(mesh);

    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoord;
    std::vector<int> faceVertexIndices;
    std::vector<int> faceVertexCounts;

    float step = size / (resolution - 1);

    for (int i = 0; i < resolution; ++i) {
        for (int j = 0; j < resolution; ++j) {
            float x = i * step;
            float y = j * step;
            float z = wave_height * std::sin(
                                        y * period_count * 2.0f *
                                        static_cast<float>(M_PI) / size);

            points.push_back(glm::vec3(x, y, z));

            float dz_dy =
                wave_height *
                (2.0f * static_cast<float>(M_PI) * period_count / size) *
                std::cos(
                    y * period_count * 2.0f * static_cast<float>(M_PI) / size);
            glm::vec3 normal(-0.0f, -dz_dy, 1.0f);
            normal = normalize(normal);
            normals.push_back(-normal);

            float u = static_cast<float>(i) / (resolution - 1);
            float v = static_cast<float>(j) / (resolution - 1);
            texcoord.push_back(glm::vec2(u, v));
        }
    }

    for (int i = 0; i < resolution - 1; ++i) {
        for (int j = 0; j < resolution - 1; ++j) {
            faceVertexCounts.push_back(4);
            int baseIdx = i * resolution + j;
            faceVertexIndices.push_back(baseIdx);
            faceVertexIndices.push_back(baseIdx + 1);
            faceVertexIndices.push_back(baseIdx + resolution + 1);
            faceVertexIndices.push_back(baseIdx + resolution);
        }
    }

    mesh->set_vertices(points);
    mesh->set_normals(normals);
    mesh->set_face_vertex_indices(faceVertexIndices);
    mesh->set_face_vertex_counts(faceVertexCounts);
    mesh->set_texcoords_array(texcoord);

    return geometry;
}

Geometry create_diamond(
    float height,
    float section_height,
    float top_width,
    float section_width,
    int segments)
{
    Geometry geometry;
    std::shared_ptr<MeshComponent> mesh =
        std::make_shared<MeshComponent>(&geometry);
    geometry.attach_component(mesh);

    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoord;
    std::vector<int> faceVertexIndices;
    std::vector<int> faceVertexCounts;

    float halfHeight = height / 2.0f;
    float middleZ = halfHeight - section_height;
    float bottomZ = -halfHeight;

    // Add top vertex
    points.push_back(glm::vec3(0.0f, 0.0f, halfHeight));
    texcoord.push_back(glm::vec2(0.5f, 1.0f));

    // Add middle ring vertices
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * static_cast<float>(M_PI) * i / segments;
        float x = section_width * std::cos(angle);
        float y = section_width * std::sin(angle);
        points.push_back(glm::vec3(x, y, middleZ));

        float u = static_cast<float>(i) / segments;
        texcoord.push_back(glm::vec2(u, 0.66f));
    }

    // Add bottom ring vertices
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * static_cast<float>(M_PI) * i / segments;
        float x = top_width * std::cos(angle);
        float y = top_width * std::sin(angle);
        points.push_back(glm::vec3(x, y, bottomZ));

        float u = static_cast<float>(i) / segments;
        texcoord.push_back(glm::vec2(u, 0.0f));
    }

    // Add bottom vertex
    points.push_back(glm::vec3(0.0f, 0.0f, bottomZ));
    texcoord.push_back(glm::vec2(0.5f, 0.0f));

    // Create faces and compute normals
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;

        // Top pyramid faces
        faceVertexCounts.push_back(3);
        faceVertexIndices.push_back(0);
        faceVertexIndices.push_back(1 + i);
        faceVertexIndices.push_back(1 + next);

        glm::vec3 v1 = points[1 + i] - points[0];
        glm::vec3 v2 = points[1 + next] - points[0];
        glm::vec3 faceNormal = normalize(glm::cross(v1, v2));
        normals.push_back(faceNormal);

        // Middle section faces
        faceVertexCounts.push_back(4);
        faceVertexIndices.push_back(1 + i);
        faceVertexIndices.push_back(1 + next);
        faceVertexIndices.push_back(1 + segments + next);
        faceVertexIndices.push_back(1 + segments + i);

        v1 = points[1 + next] - points[1 + i];
        v2 = points[1 + segments + i] - points[1 + i];
        faceNormal = normalize(glm::cross(v1, v2));
        normals.push_back(faceNormal);

        // Bottom pyramid faces
        int bottomVertex = points.size() - 1;
        faceVertexCounts.push_back(3);
        faceVertexIndices.push_back(bottomVertex);
        faceVertexIndices.push_back(1 + segments + next);
        faceVertexIndices.push_back(1 + segments + i);

        v1 = points[1 + segments + next] - points[bottomVertex];
        v2 = points[1 + segments + i] - points[bottomVertex];
        faceNormal = normalize(glm::cross(v2, v1));
        normals.push_back(faceNormal);
    }

    mesh->set_vertices(points);
    mesh->set_face_vertex_indices(faceVertexIndices);
    mesh->set_face_vertex_counts(faceVertexCounts);
    mesh->set_normals(normals);
    mesh->set_texcoords_array(texcoord);

    return geometry;
}

Geometry create_trefoil(int resolution, float radius, float tube_radius)
{
    Geometry geometry;
    std::shared_ptr<CurveComponent> curve =
        std::make_shared<CurveComponent>(&geometry);
    geometry.attach_component(curve);

    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;

    for (int i = 0; i <= resolution; ++i) {
        float t =
            2.0f * static_cast<float>(M_PI) * (i % resolution) / resolution;

        float x = radius * (sin(t) + 2 * sin(2 * t));
        float y = radius * (cos(t) - 2 * cos(2 * t));
        float z = radius * (-sin(3 * t)) * 1.5f;

        points.push_back(glm::vec3(x, y, z));

        float dx = radius * (cos(t) + 4 * cos(2 * t));
        float dy = radius * (-sin(t) + 4 * sin(2 * t));
        float dz = radius * (-3 * cos(3 * t)) * 1.5f;

        glm::vec3 tangent(dx, dy, dz);
        tangent = normalize(tangent);

        glm::vec3 normal(0, 0, 1);
        if (std::abs(glm::dot(tangent, normal)) > 0.9f) {
            normal = glm::vec3(1, 0, 0);
        }
        normal = normalize(glm::cross(tangent, normal));

        normals.push_back(normal);
    }

    curve->set_vertices(points);
    curve->set_curve_normals(normals);
    curve->set_vert_count({ int(points.size()) });
    curve->set_periodic(true);
    curve->set_width({ tube_radius });

    return geometry;
}

Geometry create_cube(float width, float height, float depth)
{
    Geometry geometry;
    std::shared_ptr<MeshComponent> mesh =
        std::make_shared<MeshComponent>(&geometry);
    geometry.attach_component(mesh);

    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoord;
    std::vector<int> faceVertexIndices;
    std::vector<int> faceVertexCounts;

    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;
    float halfDepth = depth * 0.5f;

    std::vector<glm::vec3> vertices = {
        glm::vec3(-halfWidth, -halfHeight, -halfDepth),  // 0
        glm::vec3(halfWidth, -halfHeight, -halfDepth),   // 1
        glm::vec3(halfWidth, halfHeight, -halfDepth),    // 2
        glm::vec3(-halfWidth, halfHeight, -halfDepth),   // 3
        glm::vec3(-halfWidth, -halfHeight, halfDepth),   // 4
        glm::vec3(halfWidth, -halfHeight, halfDepth),    // 5
        glm::vec3(halfWidth, halfHeight, halfDepth),     // 6
        glm::vec3(-halfWidth, halfHeight, halfDepth)     // 7
    };

    std::vector<glm::vec3> faceNormals = {
        glm::vec3(0, 0, -1),  // Front
        glm::vec3(0, 0, 1),   // Back
        glm::vec3(-1, 0, 0),  // Left
        glm::vec3(1, 0, 0),   // Right
        glm::vec3(0, -1, 0),  // Bottom
        glm::vec3(0, 1, 0)    // Top
    };

    int faces[6][4] = {
        { 0, 1, 2, 3 },  // Front
        { 7, 6, 5, 4 },  // Back
        { 4, 0, 3, 7 },  // Left
        { 1, 5, 6, 2 },  // Right
        { 4, 5, 1, 0 },  // Bottom
        { 3, 2, 6, 7 }   // Top
    };

    std::vector<glm::vec2> faceUVs = {
        glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(1, 1), glm::vec2(0, 1)
    };

    for (int face = 0; face < 6; ++face) {
        for (int vert = 0; vert < 4; ++vert) {
            points.push_back(vertices[faces[face][vert]]);
            normals.push_back(faceNormals[face]);
            texcoord.push_back(faceUVs[vert]);
        }

        faceVertexCounts.push_back(4);
        int baseIdx = face * 4;
        faceVertexIndices.push_back(baseIdx);
        faceVertexIndices.push_back(baseIdx + 1);
        faceVertexIndices.push_back(baseIdx + 2);
        faceVertexIndices.push_back(baseIdx + 3);
    }

    mesh->set_vertices(points);
    mesh->set_normals(normals);
    mesh->set_face_vertex_indices(faceVertexIndices);
    mesh->set_face_vertex_counts(faceVertexCounts);
    mesh->set_texcoords_array(texcoord);

    return geometry;
}

Geometry create_cube2(
    float minx,
    float miny,
    float minz,
    float maxx,
    float maxy,
    float maxz)
{
    Geometry geometry;
    std::shared_ptr<MeshComponent> mesh =
        std::make_shared<MeshComponent>(&geometry);
    geometry.attach_component(mesh);

    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoord;
    std::vector<int> faceVertexIndices;
    std::vector<int> faceVertexCounts;

    std::vector<glm::vec3> vertices = {
        glm::vec3(minx, miny, minz),  // 0
        glm::vec3(maxx, miny, minz),  // 1
        glm::vec3(maxx, maxy, minz),  // 2
        glm::vec3(minx, maxy, minz),  // 3
        glm::vec3(minx, miny, maxz),  // 4
        glm::vec3(maxx, miny, maxz),  // 5
        glm::vec3(maxx, maxy, maxz),  // 6
        glm::vec3(minx, maxy, maxz)   // 7
    };

    std::vector<glm::vec3> faceNormals = {
        glm::vec3(0, 0, -1),  // Front
        glm::vec3(0, 0, 1),   // Back
        glm::vec3(-1, 0, 0),  // Left
        glm::vec3(1, 0, 0),   // Right
        glm::vec3(0, -1, 0),  // Bottom
        glm::vec3(0, 1, 0)    // Top
    };

    int faces[6][4] = {
        { 0, 1, 2, 3 },  // Front
        { 7, 6, 5, 4 },  // Back
        { 4, 0, 3, 7 },  // Left
        { 1, 5, 6, 2 },  // Right
        { 4, 5, 1, 0 },  // Bottom
        { 3, 2, 6, 7 }   // Top
    };

    std::vector<glm::vec2> faceUVs = {
        glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(1, 1), glm::vec2(0, 1)
    };

    for (int face = 0; face < 6; ++face) {
        for (int vert = 0; vert < 4; ++vert) {
            points.push_back(vertices[faces[face][vert]]);
            normals.push_back(faceNormals[face]);
            texcoord.push_back(faceUVs[vert]);
        }

        faceVertexCounts.push_back(4);
        int baseIdx = face * 4;
        faceVertexIndices.push_back(baseIdx);
        faceVertexIndices.push_back(baseIdx + 1);
        faceVertexIndices.push_back(baseIdx + 2);
        faceVertexIndices.push_back(baseIdx + 3);
    }

    mesh->set_vertices(points);
    mesh->set_normals(normals);
    mesh->set_face_vertex_indices(faceVertexIndices);
    mesh->set_face_vertex_counts(faceVertexCounts);
    mesh->set_texcoords_array(texcoord);

    return geometry;
}

Geometry create_box_grid(
    int resolution_x,
    int resolution_y,
    int resolution_z,
    float width,
    float height,
    float depth,
    bool add_diagonal)
{
    Geometry geometry;
    std::shared_ptr<MeshComponent> mesh =
        std::make_shared<MeshComponent>(&geometry);
    geometry.attach_component(mesh);

    std::vector<glm::vec3> points;
    std::vector<glm::vec2> texcoord;
    std::vector<int> faceVertexIndices;
    std::vector<int> faceVertexCounts;

    resolution_x = std::max(1, resolution_x);
    resolution_y = std::max(1, resolution_y);
    resolution_z = std::max(1, resolution_z);

    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;
    float halfDepth = depth * 0.5f;

    // Create 3D grid of vertices filling the entire volume
    int nx = resolution_x + 1;
    int ny = resolution_y + 1;
    int nz = resolution_z + 1;

    // Generate all vertices in 3D grid
    std::vector<int> vertexGrid(nx * ny * nz);
    int vertexIndex = 0;

    for (int iz = 0; iz < nz; ++iz) {
        float z = -halfDepth + (depth * iz) / resolution_z;
        for (int iy = 0; iy < ny; ++iy) {
            float y = -halfHeight + (height * iy) / resolution_y;
            for (int ix = 0; ix < nx; ++ix) {
                float x = -halfWidth + (width * ix) / resolution_x;

                points.push_back(glm::vec3(x, y, z));
                texcoord.push_back(
                    glm::vec2(0.0f, 0.0f));  // Will be updated per face

                vertexGrid[iz * ny * nx + iy * nx + ix] = vertexIndex++;
            }
        }
    }

    // Helper to get vertex index from 3D grid position
    auto getVertexIndex = [&](int ix, int iy, int iz) -> int {
        return vertexGrid[iz * ny * nx + iy * nx + ix];
    };

    // Helper to add a quad face
    auto addQuadFace =
        [&](int v0, int v1, int v2, int v3, const glm::vec3& normal) {
            if (add_diagonal) {
                // Split into two triangles
                faceVertexCounts.push_back(3);
                faceVertexIndices.push_back(v0);
                faceVertexIndices.push_back(v1);
                faceVertexIndices.push_back(v2);

                faceVertexCounts.push_back(3);
                faceVertexIndices.push_back(v0);
                faceVertexIndices.push_back(v2);
                faceVertexIndices.push_back(v3);
            }
            else {
                // Keep as quad
                faceVertexCounts.push_back(4);
                faceVertexIndices.push_back(v0);
                faceVertexIndices.push_back(v1);
                faceVertexIndices.push_back(v2);
                faceVertexIndices.push_back(v3);
            }
        };

    // Generate hexahedral cells (cubes) filling the volume
    for (int iz = 0; iz < resolution_z; ++iz) {
        for (int iy = 0; iy < resolution_y; ++iy) {
            for (int ix = 0; ix < resolution_x; ++ix) {
                // Get the 8 vertices of this hexahedron
                int v000 = getVertexIndex(ix, iy, iz);
                int v100 = getVertexIndex(ix + 1, iy, iz);
                int v110 = getVertexIndex(ix + 1, iy + 1, iz);
                int v010 = getVertexIndex(ix, iy + 1, iz);
                int v001 = getVertexIndex(ix, iy, iz + 1);
                int v101 = getVertexIndex(ix + 1, iy, iz + 1);
                int v111 = getVertexIndex(ix + 1, iy + 1, iz + 1);
                int v011 = getVertexIndex(ix, iy + 1, iz + 1);

                // Add all 6 faces of the hexahedron
                // Front face (Z-)
                addQuadFace(v000, v010, v110, v100, glm::vec3(0, 0, -1));

                // Back face (Z+)
                addQuadFace(v101, v111, v011, v001, glm::vec3(0, 0, 1));

                // Right face (X+)
                addQuadFace(v100, v110, v111, v101, glm::vec3(1, 0, 0));

                // Left face (X-)
                addQuadFace(v001, v011, v010, v000, glm::vec3(-1, 0, 0));

                // Top face (Y+)
                addQuadFace(v010, v011, v111, v110, glm::vec3(0, 1, 0));

                // Bottom face (Y-)
                addQuadFace(v001, v000, v100, v101, glm::vec3(0, -1, 0));
            }
        }
    }

    mesh->set_vertices(points);
    mesh->set_face_vertex_indices(faceVertexIndices);
    mesh->set_face_vertex_counts(faceVertexCounts);
    mesh->set_texcoords_array(texcoord);

    return geometry;
}

Geometry create_subdivided_tetrahedron(int subdivisions, float size)
{
    Geometry geometry;
    std::shared_ptr<MeshComponent> mesh =
        std::make_shared<MeshComponent>(&geometry);
    geometry.attach_component(mesh);

    // Structure to represent a tetrahedron by 4 vertex indices
    struct Tet {
        int v[4];
        Tet(int v0, int v1, int v2, int v3)
        {
            v[0] = v0;
            v[1] = v1;
            v[2] = v2;
            v[3] = v3;
        }
    };

    // Optimized triangle key using 64-bit hash
    struct TriKey {
        uint64_t key;
        TriKey(int a, int b, int c)
        {
            int v[3] = { a, b, c };
            std::sort(v, v + 3);
            key = ((uint64_t)v[0] << 42) | ((uint64_t)v[1] << 21) |
                  (uint64_t)v[2];
        }
        bool operator<(const TriKey& o) const
        {
            return key < o.key;
        }
    };

    // Store original face data with key
    struct FaceData {
        int v[3];
        int tetIdx;
    };

    std::vector<glm::vec3> vertices;
    std::vector<Tet> tets;

    // Create initial regular tetrahedron vertices
    float a = size / std::sqrt(2.0f) / 2.0f;
    vertices.push_back(glm::vec3(a, a, a));
    vertices.push_back(glm::vec3(a, -a, -a));
    vertices.push_back(glm::vec3(-a, a, -a));
    vertices.push_back(glm::vec3(-a, -a, a));

    // Initial tetrahedron
    tets.push_back(Tet(0, 1, 2, 3));

    // Helper to get or create midpoint vertex
    std::map<std::pair<int, int>, int> edgeMidpoints;
    auto getMidpoint = [&](int v1, int v2) -> int {
        if (v1 > v2)
            std::swap(v1, v2);
        std::pair<int, int> edge(v1, v2);
        auto it = edgeMidpoints.find(edge);
        if (it != edgeMidpoints.end()) {
            return it->second;
        }
        glm::vec3 mid = (vertices[v1] + vertices[v2]) * 0.5f;
        int newIdx = vertices.size();
        vertices.push_back(mid);
        edgeMidpoints[edge] = newIdx;
        return newIdx;
    };

    // Subdivide tetrahedra recursively
    for (int sub = 0; sub < subdivisions; ++sub) {
        std::vector<Tet> newTets;
        edgeMidpoints.clear();

        for (const Tet& tet : tets) {
            int v0 = tet.v[0], v1 = tet.v[1], v2 = tet.v[2], v3 = tet.v[3];

            // Get midpoints of all 6 edges
            int m01 = getMidpoint(v0, v1);
            int m02 = getMidpoint(v0, v2);
            int m03 = getMidpoint(v0, v3);
            int m12 = getMidpoint(v1, v2);
            int m13 = getMidpoint(v1, v3);
            int m23 = getMidpoint(v2, v3);

            // Create 8 sub-tetrahedra:
            // 4 corner tets
            newTets.push_back(Tet(v0, m01, m02, m03));
            newTets.push_back(Tet(v1, m01, m12, m13));
            newTets.push_back(Tet(v2, m02, m12, m23));
            newTets.push_back(Tet(v3, m03, m13, m23));

            // 4 octahedral tets (center region split into 4 tets)
            newTets.push_back(Tet(m01, m02, m12, m23));
            newTets.push_back(Tet(m01, m02, m03, m23));
            newTets.push_back(Tet(m01, m03, m13, m23));
            newTets.push_back(Tet(m01, m12, m13, m23));
        }

        tets = newTets;
    }

    // Output unique tetrahedral faces only (remove duplicates)
    // This is required for compute_volume_adjacency_gpu to work correctly
    std::vector<int> faceVertexIndices;
    std::vector<int> faceVertexCounts;

    // Track face occurrences to deduplicate and identify surface faces
    // Use map to store unique faces with their first occurrence data
    std::map<TriKey, std::array<int, 3>> uniqueFaces;

    for (size_t tetIdx = 0; tetIdx < tets.size(); ++tetIdx) {
        const Tet& tet = tets[tetIdx];

        // Four faces of tetrahedron with consistent winding
        // Following TetGen convention (opposite vertex determines face)
        int faces[4][3] = {
            { tet.v[1], tet.v[2], tet.v[3] },  // opposite to v0
            { tet.v[0], tet.v[3], tet.v[2] },  // opposite to v1
            { tet.v[0], tet.v[1], tet.v[3] },  // opposite to v2
            { tet.v[0], tet.v[2], tet.v[1] }   // opposite to v3
        };

        for (int f = 0; f < 4; ++f) {
            TriKey key(faces[f][0], faces[f][1], faces[f][2]);

            // Only add face if not seen before (deduplication)
            if (uniqueFaces.find(key) == uniqueFaces.end()) {
                uniqueFaces[key] = { faces[f][0], faces[f][1], faces[f][2] };
            }
        }
    }

    // Convert unique faces to output arrays
    faceVertexIndices.reserve(uniqueFaces.size() * 3);
    faceVertexCounts.reserve(uniqueFaces.size());

    // Track face occurrences for surface marker (scan all tets again)
    std::map<TriKey, int> faceOccurrences;
    for (size_t tetIdx = 0; tetIdx < tets.size(); ++tetIdx) {
        const Tet& tet = tets[tetIdx];
        int faces[4][3] = { { tet.v[1], tet.v[2], tet.v[3] },
                            { tet.v[0], tet.v[3], tet.v[2] },
                            { tet.v[0], tet.v[1], tet.v[3] },
                            { tet.v[0], tet.v[2], tet.v[1] } };
        for (int f = 0; f < 4; ++f) {
            TriKey key(faces[f][0], faces[f][1], faces[f][2]);
            faceOccurrences[key]++;
        }
    }

    // Output unique faces
    for (const auto& [key, verts] : uniqueFaces) {
        faceVertexCounts.push_back(3);
        faceVertexIndices.push_back(verts[0]);
        faceVertexIndices.push_back(verts[1]);
        faceVertexIndices.push_back(verts[2]);
    }

    // Create surface markers: 1.0 for surface faces, 0.0 for interior faces
    std::vector<float> surface_markers;
    surface_markers.reserve(faceVertexCounts.size());

    size_t faceIdx = 0;
    for (const auto& [key, verts] : uniqueFaces) {
        // Surface faces appear only once in the mesh
        float marker = (faceOccurrences[key] == 1) ? 1.0f : 0.0f;
        surface_markers.push_back(marker);
        faceIdx++;
    }

    // Compute normals per face
    std::vector<glm::vec3> normals;
    normals.reserve(faceVertexCounts.size());

    for (size_t i = 0; i < faceVertexCounts.size(); ++i) {
        int v0 = faceVertexIndices[i * 3 + 0];
        int v1 = faceVertexIndices[i * 3 + 1];
        int v2 = faceVertexIndices[i * 3 + 2];

        glm::vec3 p0 = vertices[v0];
        glm::vec3 p1 = vertices[v1];
        glm::vec3 p2 = vertices[v2];
        glm::vec3 edge1 = p1 - p0;
        glm::vec3 edge2 = p2 - p0;
        glm::vec3 normal = normalize(glm::cross(edge1, edge2));

        normals.push_back(normal);
    }

    // Generate texture coordinates (simple spherical mapping)
    std::vector<glm::vec2> texcoord(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        glm::vec3 normalized = normalize(vertices[i]);
        float u = 0.5f + std::atan2(normalized.y, normalized.x) / (2.0f * M_PI);
        float v = 0.5f - std::asin(normalized.z) / M_PI;
        texcoord[i] = glm::vec2(u, v);
    }

    mesh->set_vertices(vertices);
    mesh->set_normals(normals);
    mesh->set_face_vertex_indices(faceVertexIndices);
    mesh->set_face_vertex_counts(faceVertexCounts);
    mesh->set_texcoords_array(texcoord);

    // Add surface marker as face scalar quantity
    mesh->add_face_scalar_quantity("surf_of_vol", surface_markers);

    return geometry;
}

Geometry create_double_tetrahedron(float size)
{
    Geometry geometry;
    std::shared_ptr<MeshComponent> mesh =
        std::make_shared<MeshComponent>(&geometry);
    geometry.attach_component(mesh);

    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoord;
    std::vector<int> faceVertexIndices;
    std::vector<int> faceVertexCounts;

    // Create a SINGLE regular tetrahedron centered at origin for perfect
    // symmetry This ensures strictly vertical motion under gravity
    float height = size * std::sqrt(2.0f / 3.0f);
    float radius = size / std::sqrt(3.0f);

    // Base triangle vertices (in xy-plane, z = -h/4 for center of mass at
    // origin)
    float base_z = -height / 4.0f;
    points.push_back(glm::vec3(radius, 0.0f, base_z));
    points.push_back(
        glm::vec3(-radius / 2.0f, radius * std::sqrt(3.0f) / 2.0f, base_z));
    points.push_back(
        glm::vec3(-radius / 2.0f, -radius * std::sqrt(3.0f) / 2.0f, base_z));

    // Apex vertex (on z-axis, positioned for center of mass at origin)
    // For tetrahedron: CoM = (v0+v1+v2+v3)/4 should be at origin
    // base_z * 3 + apex_z = 0  =>  apex_z = -3*base_z = 3h/4
    float apex_z = 3.0f * height / 4.0f;
    points.push_back(glm::vec3(0.0f, 0.0f, apex_z));

    // Define 4 faces with consistent outward-pointing normals
    // Using right-hand rule: vertices ordered counter-clockwise when viewed
    // from outside
    int faces[4][3] = {
        { 0, 2, 1 },  // Bottom face (base triangle, normal pointing down)
        { 0, 1, 3 },  // Side face 1
        { 1, 2, 3 },  // Side face 2
        { 2, 0, 3 }   // Side face 3
    };

    for (int i = 0; i < 4; ++i) {
        faceVertexCounts.push_back(3);
        faceVertexIndices.push_back(faces[i][0]);
        faceVertexIndices.push_back(faces[i][1]);
        faceVertexIndices.push_back(faces[i][2]);

        glm::vec3 p0 = points[faces[i][0]];
        glm::vec3 p1 = points[faces[i][1]];
        glm::vec3 p2 = points[faces[i][2]];
        glm::vec3 edge1 = p1 - p0;
        glm::vec3 edge2 = p2 - p0;
        glm::vec3 normal = normalize(glm::cross(edge1, edge2));
        normals.push_back(normal);
    }

    // Generate texture coordinates
    texcoord.resize(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        glm::vec3 normalized = normalize(points[i]);
        float u = 0.5f + std::atan2(normalized.y, normalized.x) / (2.0f * M_PI);
        float v = 0.5f - std::asin(normalized.z) / M_PI;
        texcoord[i] = glm::vec2(u, v);
    }

    mesh->set_vertices(points);
    mesh->set_normals(normals);
    mesh->set_face_vertex_indices(faceVertexIndices);
    mesh->set_face_vertex_counts(faceVertexCounts);
    mesh->set_texcoords_array(texcoord);

    return geometry;
}

RUZINO_NAMESPACE_CLOSE_SCOPE

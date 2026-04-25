#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/vector.h>

#include "GCore/Components.h"
#include "GCore/Components/CurveComponent.h"
#include "GCore/Components/MeshComponent.h"
#include "GCore/Components/MeshViews.h"
#include "GCore/Components/PointsComponent.h"
#include "GCore/Components/XformComponent.h"
#include "GCore/GOP.h"

// Include glm bindings
#include <glm/glm.hpp>

// Include for meta_any interop
#include <entt/meta/meta.hpp>

#if RUZINO_WITH_CUDA
#include <RHI/internal/cuda_extension.hpp>
#endif

namespace nb = nanobind;
using namespace Ruzino;

// Helper function to convert Python list to glm::vec3
glm::vec3 to_vec3(const nb::tuple& t)
{
    if (t.size() != 3) {
        throw std::runtime_error("Expected tuple of size 3 for vec3");
    }
    return glm::vec3(
        nb::cast<float>(t[0]), nb::cast<float>(t[1]), nb::cast<float>(t[2]));
}

// Helper function to convert Python list to glm::vec2
glm::vec2 to_vec2(const nb::tuple& t)
{
    if (t.size() != 2) {
        throw std::runtime_error("Expected tuple of size 2 for vec2");
    }
    return glm::vec2(nb::cast<float>(t[0]), nb::cast<float>(t[1]));
}

NB_MODULE(geometry_py, m)
{
    m.doc() = "Ruzino Geometry Python bindings";

    // Bind glm types
    nb::class_<glm::vec3>(m, "vec3")
        .def(nb::init<float, float, float>())
        .def_rw("x", &glm::vec3::x)
        .def_rw("y", &glm::vec3::y)
        .def_rw("z", &glm::vec3::z)
        .def(
            "__repr__",
            [](const glm::vec3& v) {
                return "vec3(" + std::to_string(v.x) + ", " +
                       std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
            })
        .def(
            "__getitem__",
            [](const glm::vec3& v, size_t i) {
                if (i >= 3)
                    throw std::out_of_range("vec3 index out of range");
                return v[i];
            })
        .def("__setitem__", [](glm::vec3& v, size_t i, float value) {
            if (i >= 3)
                throw std::out_of_range("vec3 index out of range");
            v[i] = value;
        });

    nb::class_<glm::vec2>(m, "vec2")
        .def(nb::init<float, float>())
        .def_rw("x", &glm::vec2::x)
        .def_rw("y", &glm::vec2::y)
        .def(
            "__repr__",
            [](const glm::vec2& v) {
                return "vec2(" + std::to_string(v.x) + ", " +
                       std::to_string(v.y) + ")";
            })
        .def(
            "__getitem__",
            [](const glm::vec2& v, size_t i) {
                if (i >= 2)
                    throw std::out_of_range("vec2 index out of range");
                return v[i];
            })
        .def("__setitem__", [](glm::vec2& v, size_t i, float value) {
            if (i >= 2)
                throw std::out_of_range("vec2 index out of range");
            v[i] = value;
        });

    // GeometryComponent base class
    nb::class_<GeometryComponent>(m, "GeometryComponent")
        .def("to_string", &GeometryComponent::to_string)
        .def("hash", &GeometryComponent::hash)
        .def(
            "get_attached_operand",
            &GeometryComponent::get_attached_operand,
            nb::rv_policy::reference);

    // Helper function to create CUDA view as unique_ptr (since views are
    // non-copyable)
#if RUZINO_WITH_CUDA
    auto make_cuda_view = [](MeshComponent& mesh) {
        return std::make_unique<MeshCUDAView>(mesh);
    };
#endif

    // MeshComponent with all methods including view accessors
    nb::class_<MeshComponent, GeometryComponent>(m, "MeshComponent")
#if RUZINO_WITH_CUDA
        // CUDA view accessor for PyTorch GPU tensor access (readable and
        // writable)
        .def(
            "get_cuda_view",
            make_cuda_view,
            "Get CUDA view for PyTorch CUDA tensor access (in-place "
            "modification supported)")
#endif
        // NumPy array interface (zero-copy wrapper around internal vector)
        .def(
            "get_vertices",
            [](MeshComponent& self) -> nb::ndarray<nb::numpy, float> {
                const auto& verts = self.get_vertices();
                size_t shape[2] = { verts.size(), 3 };
                return nb::ndarray<nb::numpy, float>(
                    const_cast<float*>(
                        reinterpret_cast<const float*>(verts.data())),
                    2,
                    shape,
                    nb::handle());
            },
            nb::rv_policy::reference_internal,
            "Get vertices as numpy array (zero-copy view)")
        .def(
            "get_normals",
            [](MeshComponent& self) -> nb::ndarray<nb::numpy, float> {
                const auto& normals = self.get_normals();
                size_t shape[2] = { normals.size(), 3 };
                return nb::ndarray<nb::numpy, float>(
                    const_cast<float*>(
                        reinterpret_cast<const float*>(normals.data())),
                    2,
                    shape,
                    nb::handle());
            },
            nb::rv_policy::reference_internal,
            "Get normals as numpy array (zero-copy view)")
        .def(
            "get_display_color",
            [](MeshComponent& self) -> nb::ndarray<nb::numpy, float> {
                const auto& colors = self.get_display_color();
                size_t shape[2] = { colors.size(), 3 };
                return nb::ndarray<nb::numpy, float>(
                    const_cast<float*>(
                        reinterpret_cast<const float*>(colors.data())),
                    2,
                    shape,
                    nb::handle());
            },
            nb::rv_policy::reference_internal,
            "Get display colors as numpy array (zero-copy view)")
        .def(
            "get_texcoords",
            [](MeshComponent& self) -> nb::ndarray<nb::numpy, float> {
                const auto& texcoords = self.get_texcoords_array();
                size_t shape[2] = { texcoords.size(), 2 };
                return nb::ndarray<nb::numpy, float>(
                    const_cast<float*>(
                        reinterpret_cast<const float*>(texcoords.data())),
                    2,
                    shape,
                    nb::handle());
            },
            nb::rv_policy::reference_internal,
            "Get texcoords as numpy array (zero-copy view)")
        .def(
            "set_vertices",
            [](MeshComponent& self, nb::ndarray<nb::numpy, float> arr) {
                if (arr.ndim() != 2 || arr.shape(1) != 3) {
                    throw std::runtime_error("Expected Nx3 array");
                }
                size_t n = arr.shape(0);
                std::vector<glm::vec3> verts(n);
                std::memcpy(verts.data(), arr.data(), n * sizeof(glm::vec3));
                self.set_vertices(verts);
            },
            "Set vertices from numpy array")
        .def(
            "set_normals",
            [](MeshComponent& self, nb::ndarray<nb::numpy, float> arr) {
                if (arr.ndim() != 2 || arr.shape(1) != 3) {
                    throw std::runtime_error("Expected Nx3 array");
                }
                size_t n = arr.shape(0);
                std::vector<glm::vec3> normals(n);
                std::memcpy(normals.data(), arr.data(), n * sizeof(glm::vec3));
                self.set_normals(normals);
            },
            "Set normals from numpy array")
        .def(
            "set_display_color",
            [](MeshComponent& self, nb::ndarray<nb::numpy, float> arr) {
                if (arr.ndim() != 2 || arr.shape(1) != 3) {
                    throw std::runtime_error("Expected Nx3 array");
                }
                size_t n = arr.shape(0);
                std::vector<glm::vec3> colors(n);
                std::memcpy(colors.data(), arr.data(), n * sizeof(glm::vec3));
                self.set_display_color(colors);
            },
            "Set display colors from numpy array")
        .def(
            "set_texcoords",
            [](MeshComponent& self, nb::ndarray<nb::numpy, float> arr) {
                if (arr.ndim() != 2 || arr.shape(1) != 2) {
                    throw std::runtime_error("Expected Nx2 array");
                }
                size_t n = arr.shape(0);
                std::vector<glm::vec2> texcoords(n);
                std::memcpy(
                    texcoords.data(), arr.data(), n * sizeof(glm::vec2));
                self.set_texcoords_array(texcoords);
            },
            "Set texcoords from numpy array")
        .def(
            "set_face_vertex_counts",
            [](MeshComponent& self, nb::ndarray<nb::numpy, int> arr) {
                if (arr.ndim() != 1) {
                    throw std::runtime_error("Expected 1D array");
                }
                size_t n = arr.shape(0);
                std::vector<int> counts(arr.data(), arr.data() + n);
                self.set_face_vertex_counts(counts);
            },
            "Set face vertex counts from numpy array")
        .def(
            "set_face_vertex_indices",
            [](MeshComponent& self, nb::ndarray<nb::numpy, int> arr) {
                if (arr.ndim() != 1) {
                    throw std::runtime_error("Expected 1D array");
                }
                size_t n = arr.shape(0);
                std::vector<int> indices(arr.data(), arr.data() + n);
                self.set_face_vertex_indices(indices);
            },
            "Set face vertex indices from numpy array")
        // Other methods
        .def("to_string", &MeshComponent::to_string)
        // Vertex scalar quantities (numpy arrays)
        .def(
            "get_vertex_scalar_quantity",
            [](MeshComponent& self,
               const std::string& name) -> nb::ndarray<nb::numpy, float> {
                const auto& data = self.get_vertex_scalar_quantity(name);
                if (data.empty()) {
                    return nb::ndarray<nb::numpy, float>(
                        nullptr, 1, new size_t[1]{ 0 }, nb::handle());
                }
                size_t shape[1] = { data.size() };
                return nb::ndarray<nb::numpy, float>(
                    const_cast<float*>(data.data()), 1, shape, nb::handle());
            },
            nb::rv_policy::reference_internal,
            "Get vertex scalar quantity as numpy array (zero-copy)")
        .def(
            "set_vertex_scalar_quantity",
            [](MeshComponent& self,
               const std::string& name,
               nb::ndarray<nb::numpy, float> arr) {
                if (arr.ndim() != 1) {
                    throw std::runtime_error("Expected 1D array");
                }
                size_t n = arr.shape(0);
                std::vector<float> data(arr.data(), arr.data() + n);
                self.add_vertex_scalar_quantity(name, data);
            },
            "Set vertex scalar quantity from numpy array")
        .def(
            "get_vertex_scalar_quantity_names",
            &MeshComponent::get_vertex_scalar_quantity_names)
        .def(
            "add_vertex_scalar_quantity",
            &MeshComponent::add_vertex_scalar_quantity)
        .def(
            "set_vertex_scalar_quantities",
            &MeshComponent::set_vertex_scalar_quantities)
        // Face scalar quantities (numpy arrays)
        .def(
            "get_face_scalar_quantity",
            [](MeshComponent& self,
               const std::string& name) -> nb::ndarray<nb::numpy, float> {
                const auto& data = self.get_face_scalar_quantity(name);
                if (data.empty()) {
                    return nb::ndarray<nb::numpy, float>(
                        nullptr, 1, new size_t[1]{ 0 }, nb::handle());
                }
                size_t shape[1] = { data.size() };
                return nb::ndarray<nb::numpy, float>(
                    const_cast<float*>(data.data()), 1, shape, nb::handle());
            },
            nb::rv_policy::reference_internal,
            "Get face scalar quantity as numpy array (zero-copy)")
        .def(
            "set_face_scalar_quantity",
            [](MeshComponent& self,
               const std::string& name,
               nb::ndarray<nb::numpy, float> arr) {
                if (arr.ndim() != 1) {
                    throw std::runtime_error("Expected 1D array");
                }
                size_t n = arr.shape(0);
                std::vector<float> data(arr.data(), arr.data() + n);
                self.add_face_scalar_quantity(name, data);
            },
            "Set face scalar quantity from numpy array")
        .def(
            "get_face_scalar_quantity_names",
            &MeshComponent::get_face_scalar_quantity_names)
        .def(
            "add_face_scalar_quantity",
            &MeshComponent::add_face_scalar_quantity)
        .def(
            "set_face_scalar_quantities",
            &MeshComponent::set_face_scalar_quantities)
        // Vertex vector quantities (numpy arrays)
        .def(
            "get_vertex_vector_quantity",
            [](MeshComponent& self,
               const std::string& name) -> nb::ndarray<nb::numpy, float> {
                const auto& data = self.get_vertex_vector_quantity(name);
                if (data.empty()) {
                    return nb::ndarray<nb::numpy, float>(
                        nullptr, 2, new size_t[2]{ 0, 3 }, nb::handle());
                }
                size_t shape[2] = { data.size(), 3 };
                return nb::ndarray<nb::numpy, float>(
                    const_cast<float*>(
                        reinterpret_cast<const float*>(data.data())),
                    2,
                    shape,
                    nb::handle());
            },
            nb::rv_policy::reference_internal,
            "Get vertex vector quantity as numpy array (zero-copy)")
        .def(
            "set_vertex_vector_quantity",
            [](MeshComponent& self,
               const std::string& name,
               nb::ndarray<nb::numpy, float> arr) {
                if (arr.ndim() != 2 || arr.shape(1) != 3) {
                    throw std::runtime_error("Expected Nx3 array");
                }
                size_t n = arr.shape(0);
                std::vector<glm::vec3> data(n);
                std::memcpy(data.data(), arr.data(), n * sizeof(glm::vec3));
                self.add_vertex_vector_quantity(name, data);
            },
            "Set vertex vector quantity from numpy array")
        .def(
            "get_vertex_vector_quantity_names",
            &MeshComponent::get_vertex_vector_quantity_names)
        .def(
            "add_vertex_vector_quantity",
            &MeshComponent::add_vertex_vector_quantity)
        .def(
            "set_vertex_vector_quantities",
            &MeshComponent::set_vertex_vector_quantities)
        // Face vector quantities (numpy arrays)
        .def(
            "get_face_vector_quantity",
            [](MeshComponent& self,
               const std::string& name) -> nb::ndarray<nb::numpy, float> {
                const auto& data = self.get_face_vector_quantity(name);
                if (data.empty()) {
                    return nb::ndarray<nb::numpy, float>(
                        nullptr, 2, new size_t[2]{ 0, 3 }, nb::handle());
                }
                size_t shape[2] = { data.size(), 3 };
                return nb::ndarray<nb::numpy, float>(
                    const_cast<float*>(
                        reinterpret_cast<const float*>(data.data())),
                    2,
                    shape,
                    nb::handle());
            },
            nb::rv_policy::reference_internal,
            "Get face vector quantity as numpy array (zero-copy)")
        .def(
            "set_face_vector_quantity",
            [](MeshComponent& self,
               const std::string& name,
               nb::ndarray<nb::numpy, float> arr) {
                if (arr.ndim() != 2 || arr.shape(1) != 3) {
                    throw std::runtime_error("Expected Nx3 array");
                }
                size_t n = arr.shape(0);
                std::vector<glm::vec3> data(n);
                std::memcpy(data.data(), arr.data(), n * sizeof(glm::vec3));
                self.add_face_vector_quantity(name, data);
            },
            "Set face vector quantity from numpy array")
        .def(
            "get_face_vector_quantity_names",
            &MeshComponent::get_face_vector_quantity_names)
        .def(
            "add_face_vector_quantity",
            &MeshComponent::add_face_vector_quantity)
        .def(
            "set_face_vector_quantities",
            &MeshComponent::set_face_vector_quantities)
        // Parameterization quantities (numpy arrays)
        .def(
            "get_face_corner_parameterization_quantity",
            [](MeshComponent& self,
               const std::string& name) -> nb::ndarray<nb::numpy, float> {
                const auto& data =
                    self.get_face_corner_parameterization_quantity(name);
                if (data.empty()) {
                    return nb::ndarray<nb::numpy, float>(
                        nullptr, 2, new size_t[2]{ 0, 2 }, nb::handle());
                }
                size_t shape[2] = { data.size(), 2 };
                return nb::ndarray<nb::numpy, float>(
                    const_cast<float*>(
                        reinterpret_cast<const float*>(data.data())),
                    2,
                    shape,
                    nb::handle());
            },
            nb::rv_policy::reference_internal,
            "Get face corner parameterization as numpy array (zero-copy)")
        .def(
            "set_face_corner_parameterization_quantity",
            [](MeshComponent& self,
               const std::string& name,
               nb::ndarray<nb::numpy, float> arr) {
                if (arr.ndim() != 2 || arr.shape(1) != 2) {
                    throw std::runtime_error("Expected Nx2 array");
                }
                size_t n = arr.shape(0);
                std::vector<glm::vec2> data(n);
                std::memcpy(data.data(), arr.data(), n * sizeof(glm::vec2));
                self.add_face_corner_parameterization_quantity(name, data);
            },
            "Set face corner parameterization from numpy array")
        .def(
            "get_face_corner_parameterization_quantity_names",
            &MeshComponent::get_face_corner_parameterization_quantity_names)
        .def(
            "add_face_corner_parameterization_quantity",
            &MeshComponent::add_face_corner_parameterization_quantity)
        .def(
            "set_face_corner_parameterization_quantities",
            &MeshComponent::set_face_corner_parameterization_quantities)
        .def(
            "get_vertex_parameterization_quantity",
            [](MeshComponent& self,
               const std::string& name) -> nb::ndarray<nb::numpy, float> {
                const auto& data =
                    self.get_vertex_parameterization_quantity(name);
                if (data.empty()) {
                    return nb::ndarray<nb::numpy, float>(
                        nullptr, 2, new size_t[2]{ 0, 2 }, nb::handle());
                }
                size_t shape[2] = { data.size(), 2 };
                return nb::ndarray<nb::numpy, float>(
                    const_cast<float*>(
                        reinterpret_cast<const float*>(data.data())),
                    2,
                    shape,
                    nb::handle());
            },
            nb::rv_policy::reference_internal,
            "Get vertex parameterization as numpy array (zero-copy)")
        .def(
            "set_vertex_parameterization_quantity",
            [](MeshComponent& self,
               const std::string& name,
               nb::ndarray<nb::numpy, float> arr) {
                if (arr.ndim() != 2 || arr.shape(1) != 2) {
                    throw std::runtime_error("Expected Nx2 array");
                }
                size_t n = arr.shape(0);
                std::vector<glm::vec2> data(n);
                std::memcpy(data.data(), arr.data(), n * sizeof(glm::vec2));
                self.add_vertex_parameterization_quantity(name, data);
            },
            "Set vertex parameterization from numpy array")
        .def(
            "get_vertex_parameterization_quantity_names",
            &MeshComponent::get_vertex_parameterization_quantity_names)
        .def(
            "add_vertex_parameterization_quantity",
            &MeshComponent::add_vertex_parameterization_quantity)
        .def(
            "set_vertex_parameterization_quantities",
            &MeshComponent::set_vertex_parameterization_quantities)
        .def("append_mesh", &MeshComponent::append_mesh);

    // PointsComponent
    nb::class_<PointsComponent, GeometryComponent>(m, "PointsComponent")
        .def("to_string", &PointsComponent::to_string)
        .def("get_vertices", &PointsComponent::get_vertices)
        .def("get_display_color", &PointsComponent::get_display_color)
        .def("get_width", &PointsComponent::get_width)
        .def("get_normals", &PointsComponent::get_normals)
        .def("set_vertices", &PointsComponent::set_vertices)
        .def("set_display_color", &PointsComponent::set_display_color)
        .def("set_width", &PointsComponent::set_width)
        .def("set_normals", &PointsComponent::set_normals);

    // CurveComponent
    nb::class_<CurveComponent, GeometryComponent>(m, "CurveComponent")
        .def("to_string", &CurveComponent::to_string)
        .def("get_vertices", &CurveComponent::get_vertices)
        .def("get_width", &CurveComponent::get_width)
        .def("get_widths", &CurveComponent::get_widths)
        .def("get_vert_count", &CurveComponent::get_vert_count)
        .def("get_curve_counts", &CurveComponent::get_curve_counts)
        .def("get_display_color", &CurveComponent::get_display_color)
        .def("get_periodic", &CurveComponent::get_periodic)
        .def("get_curve_normals", &CurveComponent::get_curve_normals)
        .def("get_type", &CurveComponent::get_type)
        .def("set_vertices", &CurveComponent::set_vertices)
        .def("set_width", &CurveComponent::set_width)
        .def("set_widths", &CurveComponent::set_widths)
        .def("set_vert_count", &CurveComponent::set_vert_count)
        .def("set_curve_counts", &CurveComponent::set_curve_counts)
        .def("set_display_color", &CurveComponent::set_display_color)
        .def("set_periodic", &CurveComponent::set_periodic)
        .def("set_curve_normals", &CurveComponent::set_curve_normals)
        .def("set_type", &CurveComponent::set_type);

    // CurveType enum
    nb::enum_<CurveComponent::CurveType>(m, "CurveType")
        .value("Linear", CurveComponent::CurveType::Linear)
        .value("Cubic", CurveComponent::CurveType::Cubic);

    // XformComponent
    nb::class_<XformComponent, GeometryComponent>(m, "XformComponent")
        .def("to_string", &XformComponent::to_string);

    // Geometry class
    nb::class_<Geometry>(m, "Geometry")
        .def(nb::init<>())
        .def(nb::init<const Geometry&>())
        .def("apply_transform", &Geometry::apply_transform)
        .def("to_string", &Geometry::to_string)
        .def("hash", &Geometry::hash)
        .def("attach_component", &Geometry::attach_component)
        .def("detach_component", &Geometry::detach_component)
        .def("get_components", &Geometry::get_components)
        // get_component with type - need Python-friendly interface
        .def(
            "get_mesh_component",
            [](const Geometry& g, size_t idx) {
                return g.get_component<MeshComponent>(idx);
            },
            nb::arg("idx") = 0,
            "Get MeshComponent at index")
        .def(
            "get_points_component",
            [](const Geometry& g, size_t idx) {
                return g.get_component<PointsComponent>(idx);
            },
            nb::arg("idx") = 0,
            "Get PointsComponent at index")
        .def(
            "get_curve_component",
            [](const Geometry& g, size_t idx) {
                return g.get_component<CurveComponent>(idx);
            },
            nb::arg("idx") = 0,
            "Get CurveComponent at index")
        .def(
            "get_xform_component",
            [](const Geometry& g, size_t idx) {
                return g.get_component<XformComponent>(idx);
            },
            nb::arg("idx") = 0,
            "Get XformComponent at index")
        .def(
            "__eq__",
            [](const Geometry& a, const Geometry& b) { return a == b; })
        .def(
            "__ne__",
            [](const Geometry& a, const Geometry& b) { return a != b; })
        .def("__repr__", &Geometry::to_string);

    // Static factory methods
    m.def("CreateMesh", &Geometry::CreateMesh, "Create a mesh geometry");
    m.def("CreatePoints", &Geometry::CreatePoints, "Create a points geometry");
    m.def("CreateCurve", &Geometry::CreateCurve, "Create a curve geometry");
#ifdef GEOM_USD_EXTENSION
    m.def("CreateVolume", &Geometry::CreateVolume, "Create a volume geometry");
#endif

    // Helper functions for creating geometries from Python data
    m.def(
        "create_mesh_from_arrays",
        [](const std::vector<glm::vec3>& vertices,
           const std::vector<int>& face_vertex_counts,
           const std::vector<int>& face_vertex_indices) {
            auto geom = Geometry::CreateMesh();
            auto mesh = geom.get_component<MeshComponent>();
            if (mesh) {
                mesh->set_vertices(vertices);
                mesh->set_face_vertex_counts(face_vertex_counts);
                mesh->set_face_vertex_indices(face_vertex_indices);
            }
            return geom;
        },
        nb::arg("vertices"),
        nb::arg("face_vertex_counts"),
        nb::arg("face_vertex_indices"),
        "Create a mesh from vertex and face arrays");

    m.def(
        "create_points_from_array",
        [](const std::vector<glm::vec3>& vertices) {
            auto geom = Geometry::CreatePoints();
            auto points = geom.get_component<PointsComponent>();
            if (points) {
                points->set_vertices(vertices);
            }
            return geom;
        },
        nb::arg("vertices"),
        "Create points from vertex array");

    // NumPy array support functions for zero-copy data transfer
    // Get vertices as numpy array (zero-copy with ownership management)
    m.def(
        "get_vertices_as_array",
        [](std::shared_ptr<MeshComponent> mesh)
            -> nb::ndarray<nb::numpy, float, nb::shape<-1, 3>> {
            auto verts = mesh->get_vertices();
            size_t size = verts.size();
            float* data = new float[size * 3];
            for (size_t i = 0; i < size; ++i) {
                data[i * 3] = verts[i].x;
                data[i * 3 + 1] = verts[i].y;
                data[i * 3 + 2] = verts[i].z;
            }
            size_t shape[2] = { size, 3 };
            nb::capsule owner(
                data, [](void* p) noexcept { delete[] (float*)p; });
            return nb::ndarray<nb::numpy, float, nb::shape<-1, 3>>(
                data, 2, shape, owner);
        },
        nb::arg("mesh"),
        "Get vertices as numpy array (zero-copy)");

    // Set vertices from numpy array
    m.def(
        "set_vertices_from_array",
        [](std::shared_ptr<MeshComponent> mesh,
           nb::ndarray<float, nb::shape<-1, 3>> arr) {
            size_t n = arr.shape(0);
            std::vector<glm::vec3> vertices(n);
            const float* data = arr.data();
            for (size_t i = 0; i < n; ++i) {
                vertices[i] =
                    glm::vec3(data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
            }
            mesh->set_vertices(vertices);
        },
        nb::arg("mesh"),
        nb::arg("array"),
        "Set vertices from numpy array");

    // Get face indices as numpy array (zero-copy)
    m.def(
        "get_face_indices_as_array",
        [](std::shared_ptr<MeshComponent> mesh) -> nb::ndarray<nb::numpy, int> {
            auto indices = mesh->get_face_vertex_indices();
            size_t size = indices.size();
            int* data = new int[size];
            std::copy(indices.begin(), indices.end(), data);
            size_t shape[1] = { size };
            nb::capsule owner(data, [](void* p) noexcept { delete[] (int*)p; });
            return nb::ndarray<nb::numpy, int>(data, 1, shape, owner);
        },
        nb::arg("mesh"),
        "Get face indices as numpy array (zero-copy)");

    // Helper function to extract Geometry from entt::meta_any
    // This is useful when getting outputs from node graphs
    m.def(
        "extract_geometry_from_meta_any",
        [](const entt::meta_any& any) -> std::shared_ptr<Geometry> {
            if (!any) {
                throw std::runtime_error("meta_any is empty");
            }

            // Try shared_ptr first
            if (auto ptr = any.try_cast<std::shared_ptr<Geometry>>()) {
                return *ptr;
            }

            // Try raw pointer
            if (auto ptr = any.try_cast<Geometry*>()) {
                // Wrap in shared_ptr (non-owning)
                return std::shared_ptr<Geometry>(*ptr, [](Geometry*) { });
            }

            // Try reference
            if (auto ptr = any.try_cast<Geometry>()) {
                // Wrap in shared_ptr (non-owning)
                return std::shared_ptr<Geometry>(
                    const_cast<Geometry*>(&(*ptr)), [](Geometry*) { });
            }

            throw std::runtime_error(
                "meta_any does not contain a Geometry (type: " +
                std::string(any.type().info().name()) + ")");
        },
        nb::arg("meta_any"),
        "Extract Geometry from entt::meta_any (from node graph outputs)");

    // ===== MeshViews for efficient data access =====

#if RUZINO_WITH_CUDA
    // CUDA View - PyTorch CUDA tensor interface (readable and writable)
    nb::class_<MeshCUDAView>(m, "MeshCUDAView")
        // Get methods return tensors that can be modified in-place
        .def(
            "get_vertices",
            [](MeshCUDAView& self)
                -> nb::ndarray<nb::pytorch, float, nb::device::cuda> {
                auto buffer = self.get_vertices();
                auto desc = buffer->getDesc();
                size_t num_verts = desc.element_count;
                size_t shape[2] = { num_verts, 3 };
                int64_t stride[2] = {
                    static_cast<int64_t>(desc.element_size / sizeof(float)), 1
                };
                void* ptr = reinterpret_cast<void*>(buffer->get_device_ptr());
                return nb::ndarray<nb::pytorch, float, nb::device::cuda>(
                    ptr, 2, shape, nb::handle(), stride);
            },
            nb::rv_policy::reference,
            "Get vertices as PyTorch CUDA tensor (zero-copy, modifiable "
            "in-place)")
        .def(
            "get_normals",
            [](MeshCUDAView& self)
                -> nb::ndarray<nb::pytorch, float, nb::device::cuda> {
                auto buffer = self.get_normals();
                auto desc = buffer->getDesc();
                size_t num_verts = desc.element_count;
                size_t shape[2] = { num_verts, 3 };
                int64_t stride[2] = {
                    static_cast<int64_t>(desc.element_size / sizeof(float)), 1
                };
                void* ptr = reinterpret_cast<void*>(buffer->get_device_ptr());
                return nb::ndarray<nb::pytorch, float, nb::device::cuda>(
                    ptr, 2, shape, nb::handle(), stride);
            },
            nb::rv_policy::reference,
            "Get normals as PyTorch CUDA tensor (zero-copy, modifiable "
            "in-place)")
        .def(
            "get_display_color",
            [](MeshCUDAView& self)
                -> nb::ndarray<nb::pytorch, float, nb::device::cuda> {
                auto buffer = self.get_display_colors();
                auto desc = buffer->getDesc();
                size_t num_verts = desc.element_count;
                size_t shape[2] = { num_verts, 3 };
                int64_t stride[2] = {
                    static_cast<int64_t>(desc.element_size / sizeof(float)), 1
                };
                void* ptr = reinterpret_cast<void*>(buffer->get_device_ptr());
                return nb::ndarray<nb::pytorch, float, nb::device::cuda>(
                    ptr, 2, shape, nb::handle(), stride);
            },
            nb::rv_policy::reference,
            "Get display colors as PyTorch CUDA tensor (zero-copy, modifiable "
            "in-place)")
        .def(
            "get_texcoords",
            [](MeshCUDAView& self)
                -> nb::ndarray<nb::pytorch, float, nb::device::cuda> {
                auto buffer = self.get_uv_coordinates();
                auto desc = buffer->getDesc();
                size_t num_verts = desc.element_count;
                size_t shape[2] = { num_verts, 2 };
                int64_t stride[2] = {
                    static_cast<int64_t>(desc.element_size / sizeof(float)), 1
                };
                void* ptr = reinterpret_cast<void*>(buffer->get_device_ptr());
                return nb::ndarray<nb::pytorch, float, nb::device::cuda>(
                    ptr, 2, shape, nb::handle(), stride);
            },
            nb::rv_policy::reference,
            "Get texcoords as PyTorch CUDA tensor (zero-copy, modifiable "
            "in-place)")
        // Quantity getters - return PyTorch CUDA tensors
        .def(
            "get_vertex_scalar_quantity",
            [](MeshCUDAView& self, const std::string& name)
                -> nb::ndarray<nb::pytorch, float, nb::device::cuda> {
                auto buffer = self.get_vertex_scalar_quantity(name);
                size_t n = buffer->getDesc().element_count;
                size_t shape[1] = { n };
                return nb::ndarray<nb::pytorch, float, nb::device::cuda>(
                    reinterpret_cast<float*>(buffer->get_device_ptr()),
                    1,
                    shape,
                    nb::handle());
            },
            nb::arg("name"),
            nb::rv_policy::reference,
            "Get vertex scalar quantity as PyTorch CUDA tensor (zero-copy)")
        .def(
            "get_face_scalar_quantity",
            [](MeshCUDAView& self, const std::string& name)
                -> nb::ndarray<nb::pytorch, float, nb::device::cuda> {
                auto buffer = self.get_face_scalar_quantity(name);
                size_t n = buffer->getDesc().element_count;
                size_t shape[1] = { n };
                return nb::ndarray<nb::pytorch, float, nb::device::cuda>(
                    reinterpret_cast<float*>(buffer->get_device_ptr()),
                    1,
                    shape,
                    nb::handle());
            },
            nb::arg("name"),
            nb::rv_policy::reference,
            "Get face scalar quantity as PyTorch CUDA tensor (zero-copy)")
        .def(
            "get_vertex_vector_quantity",
            [](MeshCUDAView& self, const std::string& name)
                -> nb::ndarray<nb::pytorch, float, nb::device::cuda> {
                auto buffer = self.get_vertex_vector_quantity(name);
                size_t n = buffer->getDesc().element_count;
                size_t shape[2] = { n, 3 };
                return nb::ndarray<nb::pytorch, float, nb::device::cuda>(
                    reinterpret_cast<float*>(buffer->get_device_ptr()),
                    2,
                    shape,
                    nb::handle());
            },
            nb::arg("name"),
            nb::rv_policy::reference,
            "Get vertex vector quantity as PyTorch CUDA tensor (zero-copy)")
        .def(
            "get_face_vector_quantity",
            [](MeshCUDAView& self, const std::string& name)
                -> nb::ndarray<nb::pytorch, float, nb::device::cuda> {
                auto buffer = self.get_face_vector_quantity(name);
                size_t n = buffer->getDesc().element_count;
                size_t shape[2] = { n, 3 };
                return nb::ndarray<nb::pytorch, float, nb::device::cuda>(
                    reinterpret_cast<float*>(buffer->get_device_ptr()),
                    2,
                    shape,
                    nb::handle());
            },
            nb::arg("name"),
            nb::rv_policy::reference,
            "Get face vector quantity as PyTorch CUDA tensor (zero-copy)")
        .def(
            "get_vertex_parameterization_quantity",
            [](MeshCUDAView& self, const std::string& name)
                -> nb::ndarray<nb::pytorch, float, nb::device::cuda> {
                auto buffer = self.get_vertex_parameterization_quantity(name);
                size_t n = buffer->getDesc().element_count;
                size_t shape[2] = { n, 2 };
                return nb::ndarray<nb::pytorch, float, nb::device::cuda>(
                    reinterpret_cast<float*>(buffer->get_device_ptr()),
                    2,
                    shape,
                    nb::handle());
            },
            nb::arg("name"),
            nb::rv_policy::reference,
            "Get vertex parameterization quantity as PyTorch CUDA tensor "
            "(zero-copy)")
        .def(
            "get_face_corner_parameterization_quantity",
            [](MeshCUDAView& self, const std::string& name)
                -> nb::ndarray<nb::pytorch, float, nb::device::cuda> {
                auto buffer =
                    self.get_face_corner_parameterization_quantity(name);
                size_t n = buffer->getDesc().element_count;
                size_t shape[2] = { n, 2 };
                return nb::ndarray<nb::pytorch, float, nb::device::cuda>(
                    reinterpret_cast<float*>(buffer->get_device_ptr()),
                    2,
                    shape,
                    nb::handle());
            },
            nb::arg("name"),
            nb::rv_policy::reference,
            "Get face corner parameterization quantity as PyTorch CUDA tensor "
            "(zero-copy)")
        // Set methods (optional - can also modify tensors in-place)
        .def(
            "set_vertices",
            [](MeshCUDAView& self,
               nb::ndarray<nb::pytorch, float, nb::device::cuda> arr) {
                if (arr.ndim() != 2 || arr.shape(1) != 3) {
                    throw std::runtime_error("Expected Nx3 tensor");
                }
                size_t n = arr.shape(0);
                auto buffer =
                    Ruzino::cuda::create_cuda_linear_buffer<glm::vec3>(n);
                void* dst = reinterpret_cast<void*>(buffer->get_device_ptr());
                cudaMemcpy(
                    dst,
                    arr.data(),
                    n * sizeof(glm::vec3),
                    cudaMemcpyDeviceToDevice);
                self.set_vertices(buffer);
            },
            "Set vertices from PyTorch CUDA tensor")
        .def(
            "set_normals",
            [](MeshCUDAView& self,
               nb::ndarray<nb::pytorch, float, nb::device::cuda> arr) {
                if (arr.ndim() != 2 || arr.shape(1) != 3) {
                    throw std::runtime_error("Expected Nx3 tensor");
                }
                size_t n = arr.shape(0);
                auto buffer =
                    Ruzino::cuda::create_cuda_linear_buffer<glm::vec3>(n);
                void* dst = reinterpret_cast<void*>(buffer->get_device_ptr());
                cudaMemcpy(
                    dst,
                    arr.data(),
                    n * sizeof(glm::vec3),
                    cudaMemcpyDeviceToDevice);
                self.set_normals(buffer);
            },
            "Set normals from PyTorch CUDA tensor")
        .def(
            "set_display_color",
            [](MeshCUDAView& self,
               nb::ndarray<nb::pytorch, float, nb::device::cuda> arr) {
                if (arr.ndim() != 2 || arr.shape(1) != 3) {
                    throw std::runtime_error("Expected Nx3 tensor");
                }
                size_t n = arr.shape(0);
                auto buffer =
                    Ruzino::cuda::create_cuda_linear_buffer<glm::vec3>(n);
                void* dst = reinterpret_cast<void*>(buffer->get_device_ptr());
                cudaMemcpy(
                    dst,
                    arr.data(),
                    n * sizeof(glm::vec3),
                    cudaMemcpyDeviceToDevice);
                self.set_display_colors(buffer);
            },
            "Set display colors from PyTorch CUDA tensor")
        .def(
            "set_texcoords",
            [](MeshCUDAView& self,
               nb::ndarray<nb::pytorch, float, nb::device::cuda> arr) {
                if (arr.ndim() != 2 || arr.shape(1) != 2) {
                    throw std::runtime_error("Expected Nx2 tensor");
                }
                size_t n = arr.shape(0);
                auto buffer =
                    Ruzino::cuda::create_cuda_linear_buffer<glm::vec2>(n);
                void* dst = reinterpret_cast<void*>(buffer->get_device_ptr());
                cudaMemcpy(
                    dst,
                    arr.data(),
                    n * sizeof(glm::vec2),
                    cudaMemcpyDeviceToDevice);
                self.set_uv_coordinates(buffer);
            },
            "Set texcoords from PyTorch CUDA tensor")
        .def(
            "set_face_vertex_counts",
            [](MeshCUDAView& self,
               nb::ndarray<nb::pytorch, int, nb::device::cuda> arr) {
                if (arr.ndim() != 1) {
                    throw std::runtime_error("Expected 1D tensor");
                }
                size_t n = arr.shape(0);
                auto buffer = Ruzino::cuda::create_cuda_linear_buffer<int>(n);
                void* dst = reinterpret_cast<void*>(buffer->get_device_ptr());
                cudaMemcpy(
                    dst, arr.data(), n * sizeof(int), cudaMemcpyDeviceToDevice);
                auto indices = self.get_face_vertex_indices();
                self.set_face_topology(buffer, indices);
            },
            "Set face vertex counts from PyTorch CUDA tensor")
        .def(
            "set_face_vertex_indices",
            [](MeshCUDAView& self,
               nb::ndarray<nb::pytorch, int, nb::device::cuda> arr) {
                if (arr.ndim() != 1) {
                    throw std::runtime_error("Expected 1D tensor");
                }
                size_t n = arr.shape(0);
                auto buffer = Ruzino::cuda::create_cuda_linear_buffer<int>(n);
                void* dst = reinterpret_cast<void*>(buffer->get_device_ptr());
                cudaMemcpy(
                    dst, arr.data(), n * sizeof(int), cudaMemcpyDeviceToDevice);
                auto counts = self.get_face_vertex_counts();
                self.set_face_topology(counts, buffer);
            },
            "Set face vertex indices from PyTorch CUDA tensor")
        // Quantity methods - all accept PyTorch CUDA tensors
        .def(
            "add_vertex_scalar_quantity",
            [](MeshCUDAView& self,
               const std::string& name,
               nb::ndarray<nb::pytorch, float, nb::device::cuda> arr) {
                if (arr.ndim() != 1) {
                    throw std::runtime_error("Expected 1D tensor");
                }
                size_t n = arr.shape(0);
                auto buffer = Ruzino::cuda::create_cuda_linear_buffer<float>(n);
                void* dst = reinterpret_cast<void*>(buffer->get_device_ptr());
                cudaMemcpy(
                    dst,
                    arr.data(),
                    n * sizeof(float),
                    cudaMemcpyDeviceToDevice);
                self.set_vertex_scalar_quantity(name, buffer);
            },
            nb::arg("name"),
            nb::arg("data"),
            "Add vertex scalar quantity from PyTorch CUDA tensor")
        .def(
            "add_face_scalar_quantity",
            [](MeshCUDAView& self,
               const std::string& name,
               nb::ndarray<nb::pytorch, float, nb::device::cuda> arr) {
                if (arr.ndim() != 1) {
                    throw std::runtime_error("Expected 1D tensor");
                }
                size_t n = arr.shape(0);
                auto buffer = Ruzino::cuda::create_cuda_linear_buffer<float>(n);
                void* dst = reinterpret_cast<void*>(buffer->get_device_ptr());
                cudaMemcpy(
                    dst,
                    arr.data(),
                    n * sizeof(float),
                    cudaMemcpyDeviceToDevice);
                self.set_face_scalar_quantity(name, buffer);
            },
            nb::arg("name"),
            nb::arg("data"),
            "Add face scalar quantity from PyTorch CUDA tensor")
        .def(
            "add_vertex_vector_quantity",
            [](MeshCUDAView& self,
               const std::string& name,
               nb::ndarray<nb::pytorch, float, nb::device::cuda> arr) {
                if (arr.ndim() != 2 || arr.shape(1) != 3) {
                    throw std::runtime_error("Expected Nx3 tensor");
                }
                size_t n = arr.shape(0);
                auto buffer =
                    Ruzino::cuda::create_cuda_linear_buffer<glm::vec3>(n);
                void* dst = reinterpret_cast<void*>(buffer->get_device_ptr());
                cudaMemcpy(
                    dst,
                    arr.data(),
                    n * sizeof(glm::vec3),
                    cudaMemcpyDeviceToDevice);
                self.set_vertex_vector_quantity(name, buffer);
            },
            nb::arg("name"),
            nb::arg("data"),
            "Add vertex vector quantity from PyTorch CUDA tensor")
        .def(
            "add_face_vector_quantity",
            [](MeshCUDAView& self,
               const std::string& name,
               nb::ndarray<nb::pytorch, float, nb::device::cuda> arr) {
                if (arr.ndim() != 2 || arr.shape(1) != 3) {
                    throw std::runtime_error("Expected Nx3 tensor");
                }
                size_t n = arr.shape(0);
                auto buffer =
                    Ruzino::cuda::create_cuda_linear_buffer<glm::vec3>(n);
                void* dst = reinterpret_cast<void*>(buffer->get_device_ptr());
                cudaMemcpy(
                    dst,
                    arr.data(),
                    n * sizeof(glm::vec3),
                    cudaMemcpyDeviceToDevice);
                self.set_face_vector_quantity(name, buffer);
            },
            nb::arg("name"),
            nb::arg("data"),
            "Add face vector quantity from PyTorch CUDA tensor")
        .def(
            "add_vertex_parameterization_quantity",
            [](MeshCUDAView& self,
               const std::string& name,
               nb::ndarray<nb::pytorch, float, nb::device::cuda> arr) {
                if (arr.ndim() != 2 || arr.shape(1) != 2) {
                    throw std::runtime_error("Expected Nx2 tensor");
                }
                size_t n = arr.shape(0);
                auto buffer =
                    Ruzino::cuda::create_cuda_linear_buffer<glm::vec2>(n);
                void* dst = reinterpret_cast<void*>(buffer->get_device_ptr());
                cudaMemcpy(
                    dst,
                    arr.data(),
                    n * sizeof(glm::vec2),
                    cudaMemcpyDeviceToDevice);
                self.set_vertex_parameterization_quantity(name, buffer);
            },
            nb::arg("name"),
            nb::arg("data"),
            "Add vertex parameterization quantity from PyTorch CUDA tensor")
        .def(
            "add_face_corner_parameterization_quantity",
            [](MeshCUDAView& self,
               const std::string& name,
               nb::ndarray<nb::pytorch, float, nb::device::cuda> arr) {
                if (arr.ndim() != 2 || arr.shape(1) != 2) {
                    throw std::runtime_error("Expected Nx2 tensor");
                }
                size_t n = arr.shape(0);
                auto buffer =
                    Ruzino::cuda::create_cuda_linear_buffer<glm::vec2>(n);
                void* dst = reinterpret_cast<void*>(buffer->get_device_ptr());
                cudaMemcpy(
                    dst,
                    arr.data(),
                    n * sizeof(glm::vec2),
                    cudaMemcpyDeviceToDevice);
                self.set_face_corner_parameterization_quantity(name, buffer);
            },
            nb::arg("name"),
            nb::arg("data"),
            "Add face corner parameterization quantity from PyTorch CUDA "
            "tensor");
#endif
}

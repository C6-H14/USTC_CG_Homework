#include <GCore/usd_extension.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/usd/usdGeom/curves.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/points.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdSkel/bindingAPI.h>
#include <pxr/usd/usdSkel/cache.h>
#include <pxr/usd/usdSkel/skeleton.h>
#include <pxr/usd/usdSkel/skeletonQuery.h>
#include <pxr/usd/usdVol/openVDBAsset.h>
#include <pxr/usd/usdVol/volume.h>
#include <spdlog/spdlog.h>

#include "GCore/Components/CurveComponent.h"
#include "GCore/Components/InstancerComponent.h"
#include "GCore/Components/MaterialComponent.h"
#include "GCore/Components/MeshComponent.h"
#include "GCore/Components/MeshUSDView.h"  // Only need USD view
#include "GCore/Components/PointsComponent.h"
#include "GCore/Components/SkelComponent.h"
#include "GCore/Components/VolumeComponent.h"
#include "GCore/Components/XformComponent.h"
#include "glm/gtc/type_ptr.hpp"

RUZINO_NAMESPACE_OPEN_SCOPE
// ============================================================================
// read_geometry_from_usd - 从 USD prim 读取几何数据到 Geometry
// ============================================================================
bool read_geometry_from_usd(
    Geometry& geometry,
    const pxr::UsdPrim& prim,
    pxr::UsdTimeCode time)
{
    using namespace pxr;

    if (!prim || !prim.IsA<UsdGeomMesh>()) {
        spdlog::error(
            "[read_geometry_from_usd] Prim is not a valid UsdGeomMesh");
        return false;
    }

    UsdGeomMesh usd_mesh(prim);

    // 创建或获取 MeshComponent
    auto mesh_comp = geometry.get_component<MeshComponent>();
    if (!mesh_comp) {
        mesh_comp = std::make_shared<MeshComponent>(&geometry);
        geometry.attach_component(mesh_comp);
    }

    auto mesh_view = get_usd_view(*mesh_comp);

    // 读取顶点数据
    VtArray<GfVec3f> points;
    if (usd_mesh.GetPointsAttr()) {
        usd_mesh.GetPointsAttr().Get(&points, time);
        mesh_view.set_vertices(points);
    }

    // 读取拓扑
    VtArray<int> counts;
    if (usd_mesh.GetFaceVertexCountsAttr()) {
        usd_mesh.GetFaceVertexCountsAttr().Get(&counts, time);
    }

    VtArray<int> indices;
    if (usd_mesh.GetFaceVertexIndicesAttr()) {
        usd_mesh.GetFaceVertexIndicesAttr().Get(&indices, time);
    }

    if (!counts.empty() && !indices.empty()) {
        mesh_view.set_face_topology(counts, indices);
    }

    // 读取法线
    VtArray<GfVec3f> normals;
    if (usd_mesh.GetNormalsAttr()) {
        usd_mesh.GetNormalsAttr().Get(&normals, time);
        mesh_view.set_normals(normals);
    }

    // 读取颜色
    VtArray<GfVec3f> colors;
    if (usd_mesh.GetDisplayColorAttr()) {
        usd_mesh.GetDisplayColorAttr().Get(&colors, time);
        mesh_view.set_display_colors(colors);
    }

    // 读取 UV 坐标
    UsdGeomPrimvarsAPI primvar_api(usd_mesh);
    auto uv_primvar = primvar_api.GetPrimvar(TfToken("UVMap"));
    if (!uv_primvar) {
        uv_primvar = primvar_api.GetPrimvar(TfToken("st"));
    }
    if (uv_primvar) {
        VtArray<GfVec2f> uvs;
        uv_primvar.Get(&uvs, time);
        mesh_view.set_uv_coordinates(uvs);
    }

    // 读取 Transform (作为 Geometry 的 XformComponent)
    GfMatrix4d xform_matrix = usd_mesh.ComputeLocalToWorldTransform(time);
    if (xform_matrix != GfMatrix4d().SetIdentity()) {
        auto xform_comp = geometry.get_component<XformComponent>();
        if (!xform_comp) {
            xform_comp = std::make_shared<XformComponent>(&geometry);
            geometry.attach_component(xform_comp);
        }

        auto translation = xform_matrix.ExtractTranslation();
        xform_comp->translation.clear();
        xform_comp->translation.push_back(
            glm::vec3(translation[0], translation[1], translation[2]));
        xform_comp->rotation.push_back(glm::vec3(0.0f));  // TODO: 提取旋转
        xform_comp->scale.push_back(glm::vec3(1.0f));     // TODO: 提取缩放
    }

    // 读取 Skeleton 数据
    UsdSkelBindingAPI binding = UsdSkelBindingAPI(usd_mesh);
    SdfPathVector targets;
    binding.GetSkeletonRel().GetTargets(&targets);

    if (targets.size() == 1) {
        auto stage = prim.GetStage();
        auto skel_prim = stage->GetPrimAtPath(targets[0]);
        UsdSkelSkeleton skeleton(skel_prim);

        if (skeleton) {
            UsdSkelCache skelCache;
            UsdSkelSkeletonQuery skelQuery = skelCache.GetSkelQuery(skeleton);

            auto skel_component = geometry.get_component<SkelComponent>();
            if (!skel_component) {
                skel_component = std::make_shared<SkelComponent>(&geometry);
                geometry.attach_component(skel_component);
            }

            VtArray<GfMatrix4f> xforms;
            skelQuery.ComputeJointLocalTransforms(&xforms, time);

            skel_component->localTransforms = xforms;
            skel_component->jointOrder = skelQuery.GetJointOrder();
            skel_component->topology = skelQuery.GetTopology();

            VtArray<float> jointWeight;
            binding.GetJointWeightsAttr().Get(&jointWeight, time);

            VtArray<GfMatrix4d> bindTransforms;
            skeleton.GetBindTransformsAttr().Get(&bindTransforms, time);
            skel_component->bindTransforms = bindTransforms;

            VtArray<int> jointIndices;
            binding.GetJointIndicesAttr().Get(&jointIndices, time);
            skel_component->jointWeight = jointWeight;
            skel_component->jointIndices = jointIndices;
        }
    }

    return true;
}

// Utility functions for ConstMeshUSDView
static pxr::VtArray<pxr::GfVec3f> vec3f_array_to_vt_array(
    const std::vector<glm::vec3>& array)
{
    return pxr::VtArray<pxr::GfVec3f>(
        reinterpret_cast<const pxr::GfVec3f*>(array.data()),
        reinterpret_cast<const pxr::GfVec3f*>(array.data() + array.size()));
}

static pxr::VtArray<pxr::GfVec2f> vec2f_array_to_vt_array(
    const std::vector<glm::vec2>& array)
{
    return pxr::VtArray<pxr::GfVec2f>(
        reinterpret_cast<const pxr::GfVec2f*>(array.data()),
        reinterpret_cast<const pxr::GfVec2f*>(array.data() + array.size()));
}
static pxr::VtArray<float> float_array_to_vt_array(
    const std::vector<float>& array)
{
    return pxr::VtArray<float>(array.begin(), array.end());
}

// int
static pxr::VtArray<int> int_array_to_vt_array(const std::vector<int>& array)
{
    return pxr::VtArray<int>(array.begin(), array.end());
}

bool legal(const std::string& string)
{
    if (string.empty()) {
        return false;
    }
    if (std::find_if(string.begin(), string.end(), [](char val) {
            return val == '(' || val == ')' || val == ',';
        }) == string.end()) {
        return true;
    }
    return false;
}

bool write_geometry_to_usd(
    const Geometry& geometry,
    pxr::UsdStageRefPtr stage,
    const pxr::SdfPath& sdf_path,
    pxr::UsdTimeCode time)
{
    auto mesh = geometry.get_component<MeshComponent>();
    auto points = geometry.get_component<PointsComponent>();
    auto curve = geometry.get_component<CurveComponent>();
    auto volume = geometry.get_component<VolumeComponent>();
    auto instancer = geometry.get_component<InstancerComponent>();

    assert(!(points && mesh));

    pxr::SdfPath actual_path = sdf_path;
    if (instancer) {
        actual_path = sdf_path.AppendPath(pxr::SdfPath("Prototype"));
    }

    if (mesh) {
        auto mesh_usdview = get_usd_view(*mesh);
        pxr::UsdGeomMesh usdgeom = pxr::UsdGeomMesh::Define(stage, actual_path);

        if (usdgeom) {
            usdgeom.CreatePointsAttr().Set(mesh_usdview.get_vertices(), time);
            usdgeom.CreateFaceVertexCountsAttr().Set(
                mesh_usdview.get_face_vertex_counts(), time);
            usdgeom.CreateFaceVertexIndicesAttr().Set(
                mesh_usdview.get_face_vertex_indices(), time);

            auto primVarAPI = pxr::UsdGeomPrimvarsAPI(usdgeom);

            if (!mesh_usdview.get_normals().empty()) {
                usdgeom.CreateNormalsAttr().Set(
                    mesh_usdview.get_normals(), time);

                // Check if normals are per-vertex or per-face-vertex
                // (face-varying)
                size_t num_normals = mesh_usdview.get_normals().size();
                size_t num_vertices = mesh_usdview.get_vertices().size();
                size_t num_face_vertices =
                    mesh_usdview.get_face_vertex_indices().size();

                if (num_normals == num_vertices) {
                    // One normal per vertex - vertex interpolation
                    usdgeom.SetNormalsInterpolation(pxr::UsdGeomTokens->vertex);
                }
                else if (num_normals == num_face_vertices) {
                    // One normal per face-vertex - faceVarying interpolation
                    usdgeom.SetNormalsInterpolation(
                        pxr::UsdGeomTokens->faceVarying);
                }
                else {
                    // Mismatch - log error but set faceVarying as fallback
                    usdgeom.SetNormalsInterpolation(
                        pxr::UsdGeomTokens->faceVarying);
                }

                usdgeom.CreateSubdivisionSchemeAttr().Set(
                    pxr::UsdGeomTokens->none);
            }
            else {
                auto normals_attr = usdgeom.GetNormalsAttr();
                if (normals_attr) {
                    normals_attr.Block();
                }
            }

            if (!mesh_usdview.get_display_colors().empty()) {
                auto colorPrimvar = primVarAPI.CreatePrimvar(
                    pxr::TfToken("displayColor"),
                    pxr::SdfValueTypeNames->Color3fArray);

                // Determine interpolation based on color count
                size_t num_colors = mesh_usdview.get_display_colors().size();
                size_t num_vertices = mesh_usdview.get_vertices().size();
                size_t num_faces = mesh_usdview.get_face_vertex_counts().size();

                if (num_colors == num_vertices) {
                    colorPrimvar.SetInterpolation(pxr::UsdGeomTokens->vertex);
                }
                else if (num_colors == num_faces) {
                    colorPrimvar.SetInterpolation(pxr::UsdGeomTokens->uniform);
                }
                else {
                    // Default to vertex interpolation
                    colorPrimvar.SetInterpolation(pxr::UsdGeomTokens->vertex);
                }

                colorPrimvar.Set(mesh_usdview.get_display_colors(), time);
            }
            else {
                auto colorPrimvar =
                    primVarAPI.GetPrimvar(pxr::TfToken("displayColor"));
                if (colorPrimvar) {
                    colorPrimvar.GetAttr().Block();
                }
            }

            if (!mesh_usdview.get_uv_coordinates().empty()) {
                auto primvar = primVarAPI.CreatePrimvar(
                    pxr::TfToken("UVMap"),
                    pxr::SdfValueTypeNames->TexCoord2fArray);
                primvar.Set(mesh_usdview.get_uv_coordinates(), time);
                if (mesh_usdview.get_uv_coordinates().size() ==
                    mesh_usdview.get_vertices().size()) {
                    primvar.SetInterpolation(pxr::UsdGeomTokens->vertex);
                }
                else {
                    primvar.SetInterpolation(pxr::UsdGeomTokens->faceVarying);
                }
            }
            else {
                auto uvPrimvar = primVarAPI.GetPrimvar(pxr::TfToken("UVMap"));
                if (uvPrimvar) {
                    uvPrimvar.GetAttr().Block();
                }
            }

            usdgeom.CreateDoubleSidedAttr().Set(true);

            // Write all mesh attributes
            for (const std::string& name :
                 mesh->get_vertex_scalar_quantity_names()) {
                auto values = mesh_usdview.get_vertex_scalar_quantity(name);
                const std::string primvar_name = "vertex:scalar:" + name;
                auto primvar = primVarAPI.CreatePrimvar(
                    pxr::TfToken(primvar_name.c_str()),
                    pxr::SdfValueTypeNames->FloatArray);
                primvar.SetInterpolation(pxr::UsdGeomTokens->vertex);
                primvar.Set(values, time);
            }

            for (const std::string& name :
                 mesh->get_face_scalar_quantity_names()) {
                auto values = mesh_usdview.get_face_scalar_quantity(name);
                const std::string primvar_name = "face:scalar:" + name;
                auto primvar = primVarAPI.CreatePrimvar(
                    pxr::TfToken(primvar_name.c_str()),
                    pxr::SdfValueTypeNames->FloatArray);
                primvar.SetInterpolation(pxr::UsdGeomTokens->uniform);
                primvar.Set(values, time);
            }

            for (const std::string& name :
                 mesh->get_vertex_vector_quantity_names()) {
                auto values = mesh_usdview.get_vertex_vector_quantity(name);
                const std::string primvar_name = "vertex:vector:" + name;
                auto primvar = primVarAPI.CreatePrimvar(
                    pxr::TfToken(primvar_name.c_str()),
                    pxr::SdfValueTypeNames->Vector3fArray);
                primvar.SetInterpolation(pxr::UsdGeomTokens->vertex);
                primvar.Set(values, time);
            }

            for (const std::string& name :
                 mesh->get_face_vector_quantity_names()) {
                auto values = mesh_usdview.get_face_vector_quantity(name);
                const std::string primvar_name = "face:vector:" + name;
                auto primvar = primVarAPI.CreatePrimvar(
                    pxr::TfToken(primvar_name.c_str()),
                    pxr::SdfValueTypeNames->Vector3fArray);
                primvar.SetInterpolation(pxr::UsdGeomTokens->uniform);
                primvar.Set(values, time);
            }

            for (const std::string& name :
                 mesh->get_face_corner_parameterization_quantity_names()) {
                auto values =
                    mesh_usdview.get_face_corner_parameterization_quantity(
                        name);
                const std::string primvar_name =
                    "face_corner:parameterization:" + name;
                auto primvar = primVarAPI.CreatePrimvar(
                    pxr::TfToken(primvar_name.c_str()),
                    pxr::SdfValueTypeNames->TexCoord2fArray);
                primvar.SetInterpolation(pxr::UsdGeomTokens->faceVarying);
                primvar.Set(values, time);
            }

            for (const std::string& name :
                 mesh->get_vertex_parameterization_quantity_names()) {
                auto values =
                    mesh_usdview.get_vertex_parameterization_quantity(name);
                const std::string primvar_name =
                    "vertex:parameterization:" + name;
                auto primvar = primVarAPI.CreatePrimvar(
                    pxr::TfToken(primvar_name.c_str()),
                    pxr::SdfValueTypeNames->TexCoord2fArray);
                primvar.SetInterpolation(pxr::UsdGeomTokens->vertex);
                primvar.Set(values, time);
            }
        }
    }
    else if (points) {
        pxr::UsdGeomPoints usdpoints =
            pxr::UsdGeomPoints::Define(stage, actual_path);

        usdpoints.CreatePointsAttr().Set(
            vec3f_array_to_vt_array(points->get_vertices()), time);

        if (points->get_width().size() > 0) {
            usdpoints.CreateWidthsAttr().Set(
                float_array_to_vt_array(points->get_width()), time);
        }

        auto PrimVarAPI = pxr::UsdGeomPrimvarsAPI(usdpoints);
        if (points->get_display_color().size() > 0) {
            pxr::UsdGeomPrimvar colorPrimvar = PrimVarAPI.CreatePrimvar(
                pxr::TfToken("displayColor"),
                pxr::SdfValueTypeNames->Color3fArray);
            colorPrimvar.SetInterpolation(pxr::UsdGeomTokens->vertex);
            colorPrimvar.Set(
                vec3f_array_to_vt_array(points->get_display_color()), time);
        }
    }
    else if (curve) {
        pxr::UsdGeomBasisCurves usd_curve =
            pxr::UsdGeomBasisCurves::Define(stage, actual_path);
        if (usd_curve) {
            // Set curve type and basis first
            usd_curve.CreateTypeAttr().Set(
                curve->get_type() == CurveComponent::CurveType::Linear
                    ? pxr::UsdGeomTokens->linear
                    : pxr::UsdGeomTokens->cubic,
                time);

            // Set basis for cubic curves
            if (curve->get_type() != CurveComponent::CurveType::Linear) {
                usd_curve.CreateBasisAttr().Set(
                    pxr::UsdGeomTokens->bspline, time);
            }

            // Set wrap mode
            if (curve->get_periodic())
                usd_curve.CreateWrapAttr().Set(pxr::UsdGeomTokens->periodic);
            else
                usd_curve.CreateWrapAttr().Set(pxr::UsdGeomTokens->nonperiodic);

            // Set geometry data
            usd_curve.CreatePointsAttr().Set(
                vec3f_array_to_vt_array(curve->get_vertices()), time);
            usd_curve.CreateWidthsAttr().Set(
                float_array_to_vt_array(curve->get_width()), time);
            usd_curve.CreateCurveVertexCountsAttr().Set(
                int_array_to_vt_array(curve->get_vert_count()), time);
            usd_curve.CreateNormalsAttr().Set(
                vec3f_array_to_vt_array(curve->get_curve_normals()), time);
            usd_curve.CreateDisplayColorAttr().Set(
                vec3f_array_to_vt_array(curve->get_display_color()), time);
        }
    }
    else if (volume) {
        pxr::UsdVolVolume usd_volume =
            pxr::UsdVolVolume::Define(stage, actual_path);

        if (!usd_volume) {
            return false;
        }

        auto vdb_asset_path = actual_path.AppendChild(pxr::TfToken("field"));
        auto openvdb_asset =
            pxr::UsdVolOpenVDBAsset::Define(stage, vdb_asset_path);

        if (!openvdb_asset) {
            return false;
        }

        auto file_name = "volume" + actual_path.GetName() +
                         std::to_string(time.GetValue()) + ".vdb";
        volume->write_disk(file_name);

        openvdb_asset.CreateFilePathAttr().Set(
            pxr::SdfAssetPath(file_name), time);

        if (!usd_volume.CreateFieldRelationship(
                pxr::TfToken("field"), vdb_asset_path)) {
            return false;
        }

        usd_volume.MakeVisible(time);
    }
    else {
        return false;
    }

    // Handle instancer
    if (instancer) {
        auto instancer_component =
            pxr::UsdGeomPointInstancer::Define(stage, sdf_path);
        instancer_component.CreatePrototypesRel().SetTargets({ actual_path });

        const auto& positions_glm = instancer->get_positions();
        const auto& orientations_glm = instancer->get_orientations();
        const auto& scales_glm = instancer->get_scales();

        size_t instance_count = instancer->get_instance_count();

        // Convert positions directly
        pxr::VtVec3fArray positions(instance_count);
        for (size_t i = 0; i < instance_count; ++i) {
            positions[i] = pxr::GfVec3f(
                positions_glm[i].x, positions_glm[i].y, positions_glm[i].z);
        }

        // Handle orientations if rotations are enabled
        if (instancer->has_rotations_enabled()) {
            // Only write orientations if array is not empty
            if (!orientations_glm.empty()) {
                pxr::VtQuathArray orientations(instance_count);
                for (size_t i = 0; i < instance_count; ++i) {
                    const auto& q = orientations_glm[i];
                    orientations[i] = pxr::GfQuath(q.w, q.x, q.y, q.z);
                }
                instancer_component.CreateOrientationsAttr().Set(
                    orientations, time);
            }
            else {
                // No orientations data, clear the attribute
                auto orientations_attr =
                    instancer_component.GetOrientationsAttr();
                if (orientations_attr) {
                    orientations_attr.Block();
                }
            }

            // Only write scales if array is not empty
            if (!scales_glm.empty()) {
                pxr::VtVec3fArray scales(instance_count);
                for (size_t i = 0; i < instance_count; ++i) {
                    scales[i] = pxr::GfVec3f(
                        scales_glm[i].x, scales_glm[i].y, scales_glm[i].z);
                }
                instancer_component.CreateScalesAttr().Set(scales, time);
            }
            else {
                // No scales data, clear the attribute
                auto scales_attr = instancer_component.GetScalesAttr();
                if (scales_attr) {
                    scales_attr.Block();
                }
            }
        }
        else {
            // Clear orientation attribute when rotations are disabled
            auto orientations_attr = instancer_component.GetOrientationsAttr();
            if (orientations_attr) {
                orientations_attr.Block();
            }
        }

        instancer_component.CreateProtoIndicesAttr().Set(
            pxr::VtIntArray(instancer->get_proto_indices()), time);
        instancer_component.CreatePositionsAttr().Set(positions, time);
    }

    // Handle materials
    auto material_component = geometry.get_component<MaterialComponent>();
    if (material_component) {
        auto usdgeom = pxr::UsdGeomXformable::Get(stage, actual_path);
        if (legal(std::string(material_component->textures[0].c_str()))) {
            auto material_path = material_component->get_material_path();
            auto material =
                material_component->define_material(stage, material_path);
            usdgeom.GetPrim().ApplyAPI(pxr::UsdShadeTokens->MaterialBindingAPI);
            pxr::UsdShadeMaterialBindingAPI(usdgeom).Bind(material);
        }
    }

    // Handle transforms
    auto xform_component = geometry.get_component<XformComponent>();
    auto usdgeom = pxr::UsdGeomXformable::Get(stage, actual_path);
    auto xform_op = usdgeom.GetTransformOp();
    if (!xform_op) {
        xform_op = usdgeom.AddTransformOp();
    }

    if (xform_component) {
        assert(
            xform_component->translation.size() ==
            xform_component->rotation.size());
        glm::mat4 final_transform = xform_component->get_transform();
        pxr::GfMatrix4d usd_transform;
        const float* src = glm::value_ptr(final_transform);
        double* dst = usd_transform.GetArray();
        for (int i = 0; i < 16; ++i) {
            dst[i] = static_cast<double>(src[i]);
        }
        xform_op.Set(usd_transform, time);
    }
    else {
        xform_op.Set(pxr::GfMatrix4d().SetIdentity(), time);
    }

    pxr::UsdGeomImageable(stage->GetPrimAtPath(actual_path)).MakeVisible();
    return true;
}

RUZINO_NAMESPACE_CLOSE_SCOPE

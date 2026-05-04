#include "../geometries/mesh.h"
#include "../light.h"
#include "nodes/core/def/node_def.hpp"
#include "pxr/base/gf/frustum.h"
#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/hd/tokens.h"
#include "render_node_base.h"
#include "rich_type_buffer.hpp"
#include "utils/draw_fullscreen.h"

NODE_DEF_OPEN_SCOPE
NODE_DECLARATION_FUNCTION(shadow_mapping)
{
    b.add_input<int>("resolution").default_val(2048).min(256).max(4096);
    b.add_input<std::string>("Shader").default_val("shaders/shadow_mapping.fs");
    b.add_output<GLTextureHandle>("Shadow Maps");
}

NODE_EXECUTION_FUNCTION(shadow_mapping)
{
    auto resolution = params.get_input<int>("resolution");

    // 🌟 防爆装甲 1：绝不让 array_size 变成 0！
    int valid_light_count = lights.size() > 0 ? lights.size() : 1;

    GLTextureDesc texture_desc;
    texture_desc.array_size = valid_light_count;
    texture_desc.size = pxr::GfVec2i(resolution);
    texture_desc.format = HdFormatFloat32; // 王奶奶的单通道高精度格式
    auto shadow_map_texture = resource_allocator.create(texture_desc);
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, shadow_map_texture->texture_id);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    const float shadow_border_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, shadow_border_color);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    auto shaderPath = params.get_input<std::string>("Shader");
    GLShaderDesc shader_desc;
    shader_desc.set_vertex_path(std::filesystem::path(RENDER_NODES_FILES_DIR) / "shaders/shadow_mapping.vs");
    shader_desc.set_fragment_path(std::filesystem::path(RENDER_NODES_FILES_DIR) / shaderPath);
    auto shader_handle = resource_allocator.create(shader_desc);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    constexpr GLenum draw_attachments[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, draw_attachments);

    GLTextureDesc depth_desc = texture_desc;
    depth_desc.format = HdFormatFloat32UInt8;
    depth_desc.array_size = 1;
    auto depth_texture_for_opengl = resource_allocator.create(depth_desc);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depth_texture_for_opengl->texture_id, 0);
    glViewport(0, 0, resolution, resolution);

    for (int light_id = 0; light_id < lights.size(); ++light_id) {
        shader_handle->shader.use();
        if (!lights[light_id]->GetId().IsEmpty()) {
            pxr::GlfSimpleLight light_params = lights[light_id]->Get(HdTokens->params).Get<pxr::GlfSimpleLight>();

            bool has_light = false;
            pxr::GfMatrix4f light_view_mat;
            pxr::GfMatrix4f light_projection_mat;

            if (lights[light_id]->GetLightType() == HdPrimTypeTokens->sphereLight ||
                lights[light_id]->GetLightType() == HdPrimTypeTokens->rectLight) 
            {
                pxr::GfVec3f light_pos = { light_params.GetPosition()[0], light_params.GetPosition()[1], light_params.GetPosition()[2] };
                light_view_mat = pxr::GfMatrix4f().SetLookAt(light_pos, pxr::GfVec3f(0, 0, 0), pxr::GfVec3f(0, 0, 1));
                
                pxr::GfFrustum frustum;
                frustum.SetPerspective(90.f, 1.0, 0.1f, 1000.f); // 完美视角
                light_projection_mat = pxr::GfMatrix4f(frustum.ComputeProjectionMatrix());
                has_light = true;
            }

            if (!has_light) continue;

            shader_handle->shader.setMat4("light_view", light_view_mat);
            shader_handle->shader.setMat4("light_projection", light_projection_mat);

            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, shadow_map_texture->texture_id, 0, light_id);

            // 清空背景为无穷远
            glClearColor(1.f, 1.f, 1.f, 1.0f);
            glClearDepth(1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            for (int mesh_id = 0; mesh_id < meshes.size(); ++mesh_id) {
                auto mesh = meshes[mesh_id];
                shader_handle->shader.setMat4("model", mesh->transform);
                mesh->RefreshGLBuffer();
                glBindVertexArray(mesh->VAO);
                glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(mesh->triangulatedIndices.size() * 3), GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
        }
    }

    resource_allocator.destroy(depth_texture_for_opengl);
    auto shader_error = shader_handle->shader.get_error();
    resource_allocator.destroy(shader_handle);
    
    // 🌟 防爆装甲 2：乖乖解绑 Framebuffer！不污染全局状态！
    glBindFramebuffer(GL_FRAMEBUFFER, 0); 
    glDeleteFramebuffers(1, &framebuffer);

    params.set_output("Shadow Maps", shadow_map_texture);
    if (!shader_error.empty()) return false;
    return true;
}
NODE_DECLARATION_UI(shadow_mapping);
NODE_DEF_CLOSE_SCOPE
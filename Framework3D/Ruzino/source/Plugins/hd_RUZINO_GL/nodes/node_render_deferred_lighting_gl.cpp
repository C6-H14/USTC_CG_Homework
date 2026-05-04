#include "../camera.h"
#include "../light.h"
#include "nodes/core/def/node_def.hpp"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/hd/tokens.h"
#include "render_node_base.h"
#include "rich_type_buffer.hpp"
#include "utils/draw_fullscreen.h"
#include "pxr/base/gf/frustum.h"

NODE_DEF_OPEN_SCOPE

NODE_DECLARATION_FUNCTION(deferred_lighting) {
    b.add_input<GLTextureHandle>("Position");
    b.add_input<GLTextureHandle>("diffuseColor");
    b.add_input<GLTextureHandle>("MetallicRoughness");
    b.add_input<GLTextureHandle>("Normal");
    b.add_input<GLTextureHandle>("Shadow Maps");
    b.add_input<GLTextureHandle>("SSAO_Map");
    b.add_input<std::string>("Lighting Shader").default_val("shaders/blinn_phong.fs");
    b.add_output<GLTextureHandle>("Color");
}

struct alignas(16) LightInfo {
    pxr::GfMatrix4f light_projection;
    pxr::GfMatrix4f light_view;
    pxr::GfVec3f position;
    float radius;
    pxr::GfVec3f luminance;
    int shadow_map_id;
};

NODE_EXECUTION_FUNCTION(deferred_lighting) {
    GLTextureHandle position_texture, diffuseColor_texture, metallic_roughness, normal_texture, shadow_maps;
    std::string shaderPath;

    try {
        position_texture = params.get_input<GLTextureHandle>("Position");
        diffuseColor_texture = params.get_input<GLTextureHandle>("diffuseColor");
        metallic_roughness = params.get_input<GLTextureHandle>("MetallicRoughness");
        normal_texture = params.get_input<GLTextureHandle>("Normal");
        shadow_maps = params.get_input<GLTextureHandle>("Shadow Maps");
        shaderPath = params.get_input<std::string>("Lighting Shader");
    } catch (...) { return false; }

    if (!position_texture || !diffuseColor_texture || !metallic_roughness || !normal_texture || !shadow_maps) return false;

    Hd_RUZINO_Camera* free_camera = get_free_camera(params);
    if (!free_camera) return false;

    auto size = position_texture->desc.size;
    GLTextureDesc color_output_desc;
    color_output_desc.format = HdFormatFloat32Vec4;
    color_output_desc.size = size;
    auto color_texture = resource_allocator.create(color_output_desc);

    unsigned int VBO, VAO;
    CreateFullScreenVAO(VAO, VBO);

    GLShaderDesc shader_desc;
    shader_desc.set_vertex_path(std::filesystem::path(RENDER_NODES_FILES_DIR) / "shaders/fullscreen.vs");
    shader_desc.set_fragment_path(std::filesystem::path(RENDER_NODES_FILES_DIR) / shaderPath);
    auto shader = resource_allocator.create(shader_desc);

    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture->texture_id, 0);

    glClearColor(0.f, 0.f, 0.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    shader->shader.use();
    shader->shader.setVec2("iResolution", size);

    shader->shader.setInt("diffuseColorSampler", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseColor_texture->texture_id);

    shader->shader.setInt("normalMapSampler", 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normal_texture->texture_id);

    shader->shader.setInt("metallicRoughnessSampler", 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, metallic_roughness->texture_id);

    shader->shader.setInt("shadow_maps", 3);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D_ARRAY, shadow_maps->texture_id);

    shader->shader.setInt("position", 4);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, position_texture->texture_id);

    try {
        auto ssao_map = params.get_input<GLTextureHandle>("SSAO_Map");
        if (ssao_map && ssao_map->texture_id != 0) { // 加强安全判定
            shader->shader.setInt("ssaoMap", 5);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, ssao_map->texture_id);
        }
    } catch (...) {}

    pxr::GfVec3f camPos = pxr::GfMatrix4f(free_camera->GetTransform()).ExtractTranslation();
    shader->shader.setVec3("camPos", camPos);

    GLuint lightBuffer;
    glGenBuffers(1, &lightBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightBuffer);
    glViewport(0, 0, size[0], size[1]);

    std::vector<LightInfo> light_vector;
    for (int i = 0; i < lights.size(); ++i) {
        if (lights[i] && !lights[i]->GetId().IsEmpty()) {
            pxr::GlfSimpleLight light_params = lights[i]->Get(HdTokens->params).Get<pxr::GlfSimpleLight>();
            auto diffuse4 = light_params.GetDiffuse();
            pxr::GfVec3f diffuse3(diffuse4[0], diffuse4[1], diffuse4[2]);
            auto position4 = light_params.GetPosition();
            pxr::GfVec3f position3(position4[0], position4[1], position4[2]);

            // 安全判定：如果刚加了灯还没拖半径，默认给个 1.0 防止罢工
            float radius = 1.0f;
            if (lights[i]->Get(HdLightTokens->radius).IsHolding<float>()) {
                radius = lights[i]->Get(HdLightTokens->radius).Get<float>();
            }

            pxr::GfMatrix4f light_view_mat = pxr::GfMatrix4f().SetLookAt(
                position3, pxr::GfVec3f(0, 0, 0), pxr::GfVec3f(0, 0, 1));
            
            pxr::GfFrustum frustum;
            frustum.SetPerspective(90.f, 1.0, 0.1f, 1000.f);
            pxr::GfMatrix4f light_projection_mat = pxr::GfMatrix4f(frustum.ComputeProjectionMatrix());

            light_vector.emplace_back(LightInfo{
                light_projection_mat.GetTranspose(), 
                light_view_mat.GetTranspose(), 
                position3, radius, diffuse3, i
            });
        }
    }

    shader->shader.setInt("light_count", light_vector.size());
    
    // 🌟 防爆装甲 3：如果压根没拿到合法的灯，也强行分配一个 sizeof 的空间给 GPU 闭嘴！
    size_t ssbo_size = light_vector.empty() ? sizeof(LightInfo) : light_vector.size() * sizeof(LightInfo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ssbo_size, light_vector.empty() ? nullptr : light_vector.data(), GL_STATIC_DRAW);
    
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightBuffer);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    DestroyFullScreenVAO(VAO, VBO);
    auto shader_error = shader->shader.get_error();
    resource_allocator.destroy(shader);
    glDeleteBuffers(1, &lightBuffer);
    
    // 🌟 防爆装甲 4：乖乖解绑 Framebuffer！
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &framebuffer);
    
    params.set_output("Color", color_texture);
    
    if (!shader_error.empty()) return false;
    return true;
}

NODE_DECLARATION_UI(deferred_lighting);
NODE_DEF_CLOSE_SCOPE
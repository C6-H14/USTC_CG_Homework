#version 430 core

struct Light {
    mat4 light_projection;
    mat4 light_view;
    vec3 position;
    float radius;
    vec3 color; 
    int shadow_map_id;
};

layout(binding = 0) buffer lightsBuffer {
    Light lights[4];
};

uniform vec2 iResolution;
uniform sampler2D diffuseColorSampler;
uniform sampler2D normalMapSampler;
uniform sampler2D metallicRoughnessSampler;
uniform sampler2DArray shadow_maps;
uniform sampler2D position;
// uniform sampler2D ssaoMap; // 暂时屏蔽，防未连线报错

uniform vec3 camPos;
uniform int light_count;

layout(location = 0) out vec4 Color;

void main() {
    vec2 uv = gl_FragCoord.xy / iResolution;

    vec3 pos = texture(position, uv).xyz;
    vec3 normal = texture(normalMapSampler, uv).xyz;
    vec4 metalnessRoughness = texture(metallicRoughnessSampler, uv);
    float metal = metalnessRoughness.x;
    float roughness = metalnessRoughness.y;
    vec3 albedo = texture(diffuseColorSampler, uv).rgb;
Color = vec4(albedo, 1.0);
    return;
    // 剔除天空背景
    if (length(normal) < 0.1) {
        Color = vec4(albedo, 1.0);
        return;
    }

    vec3 N = normalize(normal);
    vec3 V = normalize(camPos - pos);

    vec3 final_color = albedo * 0.05; // 基础环境光

    for(int i = 0; i < light_count; i++) {
        vec3 light_pos = lights[i].position;
        vec3 light_color = lights[i].color;

        vec3 L = normalize(light_pos - pos);
        float dist = length(light_pos - pos);
        float attenuation = 1.0 / (dist * dist + 0.0001);

        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = diff * albedo * light_color * attenuation;

        vec3 H = normalize(L + V);
        float shininess = max(2.0, pow(2.0, 10.0 * (1.0 - roughness))); 
        float spec = pow(max(dot(N, H), 0.0), shininess);
        vec3 spec_color = mix(vec3(0.04), albedo, metal);
        vec3 specular = spec * spec_color * light_color * attenuation;

        // 🌟 极简版硬阴影 (Hard Shadow)，去除一切容易报错的循环和采样
        float shadow = 1.0;
        vec4 fragPosLightSpace = lights[i].light_projection * lights[i].light_view * vec4(pos, 1.0);
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
        projCoords = projCoords * 0.5 + 0.5;

        if(projCoords.z >= 0.0 && projCoords.z <= 1.0 && 
           projCoords.x >= 0.0 && projCoords.x <= 1.0 && 
           projCoords.y >= 0.0 && projCoords.y <= 1.0) 
        {
            float currentDepth = projCoords.z;
            float closestDepth = texture(shadow_maps, vec3(projCoords.xy, lights[i].shadow_map_id)).r;
            float bias = max(0.05 * (1.0 - dot(N, L)), 0.005); 
            
            if(currentDepth > closestDepth + bias) {
                shadow = 0.0; // 被挡住了
            }
        }
        
        final_color += (diffuse + specular) * shadow;
    }

    final_color = final_color / (final_color + vec3(1.0));
    final_color = pow(final_color, vec3(1.0 / 2.2));
    
    Color = vec4(final_color, 1.0);
}
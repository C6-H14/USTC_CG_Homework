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
uniform sampler2D ssaoMap; 

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
    
    // Discard background pixels

    vec3 N = normalize(normal);
    vec3 V = normalize(camPos - pos);

    // Apply SSAO if bounded, otherwise fallback to 1.0
    float ao_val = textureSize(ssaoMap, 0).x > 0 ? texture(ssaoMap, uv).r : 1.0;
    vec3 final_color = albedo * 0.05 * ao_val; // Ambient term

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

        // PCSS Soft Shadows
        float shadow = 1.0;
        vec4 fragPosLightSpace = lights[i].light_projection * lights[i].light_view * vec4(pos, 1.0);
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
        projCoords = projCoords * 0.5 + 0.5;

        if(projCoords.z >= 0.0 && projCoords.z <= 1.0 && 
           projCoords.x >= 0.0 && projCoords.x <= 1.0 && 
           projCoords.y >= 0.0 && projCoords.y <= 1.0) 
        {
            float currentDepth = projCoords.z;
            float bias = max(0.005 * (1.0 - dot(N, L)), 0.001); 
            float lightSizeUV = 0.02; 
            
            int blockerCount = 0;
            float avgBlockerDepth = 0.0;
            for(int x = -2; x <= 2; ++x) {
                for(int y = -2; y <= 2; ++y) {
                    vec2 searchUV = projCoords.xy + vec2(float(x), float(y)) * (lightSizeUV / 2.0);
                    float z = texture(shadow_maps, vec3(searchUV, lights[i].shadow_map_id)).r;
                    if(z < currentDepth - bias) {
                        blockerCount++;
                        avgBlockerDepth += z;
                    }
                }
            }

            if (blockerCount > 0) {
                avgBlockerDepth /= float(blockerCount);
                float penumbraRatio = max((currentDepth - avgBlockerDepth) / (avgBlockerDepth + 0.001), 0.0);
                float filterRadius = clamp(penumbraRatio * lightSizeUV, 0.001, 0.05);

                shadow = 0.0;
                vec2 texelSize = 1.0 / vec2(textureSize(shadow_maps, 0).xy);
                for(int x = -2; x <= 2; ++x) {
                    for(int y = -2; y <= 2; ++y) {
                        vec2 pcfUV = projCoords.xy + vec2(float(x), float(y)) * filterRadius;
                        float pcfDepth = texture(shadow_maps, vec3(pcfUV, lights[i].shadow_map_id)).r;
                        shadow += (currentDepth - bias > pcfDepth) ? 0.0 : 1.0;        
                    }    
                }
                shadow /= 25.0; 
            }
        }
        final_color += (diffuse + specular) * shadow;
    }

    final_color = final_color / (final_color + vec3(1.0));
    final_color = pow(final_color, vec3(1.0 / 2.2));
    Color = vec4(final_color, 1.0);
}
#version 430 core
layout(location = 0) out vec4 FragColor;

uniform vec2 iResolution;
uniform sampler2D position; // 只依赖位置贴图

// Shader 内部的伪随机数生成器
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec2 texCoords = gl_FragCoord.xy / iResolution;

    // 1. 获取 3D 世界坐标
    vec3 fragPos = texture(position, texCoords).xyz;
    
    // 防御：如果是虚空背景，不需要遮蔽
    if (length(fragPos) < 0.01) {
        FragColor = vec4(1.0);
        return;
    }

    // 2. 🌟 现场徒手捏法线！利用硬件求导！
    vec3 dx = dFdx(fragPos);
    vec3 dy = dFdy(fragPos);
    vec3 N = normalize(cross(dx, dy));

    // 3. 开始 SSAO 采样
    float occlusion = 0.0;
    int kernelSize = 16;
    float radius_uv = 0.02; // 采样半径 (屏幕空间)
    float radius_3d = 0.5;  // 遮蔽判定距离 (3D空间)

    for(int i = 0; i < kernelSize; ++i) {
        // 生成螺旋随机偏移
        float angle = hash(texCoords * float(i)) * 6.2831853; 
        float r = (float(i) + 0.5) / float(kernelSize);
        vec2 offset = vec2(cos(angle), sin(angle)) * r * radius_uv;
        vec2 sampleUV = texCoords + offset;

        // 防止采样越界
        if(sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) continue;

        vec3 samplePos = texture(position, sampleUV).xyz;
        if(length(samplePos) < 0.01) continue;

        vec3 diff = samplePos - fragPos;
        float dist = length(diff);
        vec3 dir = diff / (dist + 0.0001);

        // 如果采样点在我正前方，且离我很近，就产生遮挡！
        if (dist < radius_3d && dot(N, dir) > 0.05) {
            occlusion += smoothstep(radius_3d, 0.0, dist);
        }
    }

    occlusion = 1.0 - (occlusion / float(kernelSize));
    
    // 强化对比度，让死角更黑
    float final_ao = pow(clamp(occlusion, 0.0, 1.0), 2.0);
    
    FragColor = vec4(vec3(final_ao), 1.0);
}
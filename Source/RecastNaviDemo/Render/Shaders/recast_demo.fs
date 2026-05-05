#version 330 core
// =============================================================================
// recast_demo.fs
//   与 recast_demo.vs 配对。光照模型：
//     Directional (uLightType = 0)：L = normalize(uLightVec)，atten = 1
//     Point       (uLightType = 1)：L = normalize(pos - frag)，
//                                   atten = r²/(r² + d²)，r = |uLightVec|
//   颜色合成：
//     diffK  = max(N·L, 0) * uLightIntensity * atten * shadowVisibility
//     litMul = uAmbient + (1 - uAmbient) * diffK * uLightColor   // 逐通道
//     fragColor = vCol * mix(1.0, litMul, vLit)
//   vLit = 0 时（线段类）跳过光照，直接用顶点色。
//
//   阴影 (Shadow Mapping)：
//     仅 Directional + uShadowsOn=1 时采样 uShadowMap；
//     visibility ∈ [1 - uShadowStrength, 1]：
//       - 在 light frustum 外 / 比较通过 → 1（全亮）
//       - 比较失败（被遮挡）         → 1 - uShadowStrength
//     uShadowPcf=1 时做 3×3 PCF（9 次采样，软边）。
// =============================================================================

in vec4  vCol;
in vec3  vNormal;
in vec3  vWorld;
in float vLit;
in vec4  vLightSpace;

uniform int   uLightType;        // 0 = Directional, 1 = Point
uniform vec3  uLightVec;         // dir (Directional) 或 world pos (Point)
uniform vec3  uLightColor;       // 线性 RGB
uniform float uLightIntensity;   // 漫反射倍数
uniform float uAmbient;          // 环境光强度 (0-1)

// ---- Shadow ----
uniform sampler2D uShadowMap;
uniform int       uShadowsOn;       // 0 / 1
uniform float     uShadowStrength;  // 0..1
uniform float     uShadowBias;      // 0.0005..0.005 典型
uniform int       uShadowPcf;       // 0 单点采样，1 3x3 PCF

out vec4 fragColor;

float ShadowVisibility(vec4 lightSpace, vec3 N, vec3 L)
{
    // 透视除法 → NDC，再映射到 [0,1] 的 UV/depth
    vec3 proj = lightSpace.xyz / lightSpace.w;
    proj      = proj * 0.5 + 0.5;

    // 出 light frustum：当作完全照亮（不投影也不遮挡）
    if (proj.z > 1.0 || proj.z < 0.0
        || proj.x < 0.0 || proj.x > 1.0
        || proj.y < 0.0 || proj.y > 1.0)
        return 1.0;

    // 角度自适应 bias：表面与光夹角越大，需要更大 bias
    float bias = max(uShadowBias * (1.0 - dot(N, L)), uShadowBias * 0.1);
    float currentDepth = proj.z - bias;

    if (uShadowPcf == 0)
    {
        // 单点采样
        float closest = texture(uShadowMap, proj.xy).r;
        return currentDepth > closest ? 0.0 : 1.0;
    }

    // 3×3 PCF
    float sum   = 0.0;
    vec2  texel = 1.0 / vec2(textureSize(uShadowMap, 0));
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
        {
            float closest = texture(uShadowMap, proj.xy + vec2(x, y) * texel).r;
            sum += currentDepth > closest ? 0.0 : 1.0;
        }
    return sum / 9.0;
}

void main()
{
    vec3  N     = normalize(vNormal);
    vec3  L;
    float atten = 1.0;

    if (uLightType == 1)
    {
        // 点光源：到灯方向 = pos - frag；衰减以 |uLightVec| 当作"特征半径"。
        // 当 |toL| == |uLightVec| 时 atten = 0.5；近场 → 1，远场 → 0。
        vec3  toL = uLightVec - vWorld;
        float d2  = dot(toL, toL);
        float d   = sqrt(max(d2, 1e-8));
        L         = toL / d;
        float r2  = max(dot(uLightVec, uLightVec), 1e-4);
        atten     = r2 / (r2 + d2);
    }
    else
    {
        // 平行光：只看方向。
        L = normalize(uLightVec);
    }

    // 阴影可见性：仅 Directional + 阴影开关启用 → 真正采样比较；其它情况恒 1
    float visibility = 1.0;
    if (uShadowsOn == 1 && uLightType == 0)
    {
        float v   = ShadowVisibility(vLightSpace, N, L);
        // 阴影区只削弱 diffuse 项（保留环境光），强度由 uShadowStrength 控制
        visibility = mix(1.0 - uShadowStrength, 1.0, v);
    }

    float NdotL  = max(dot(N, L), 0.0);
    float diffK  = NdotL * uLightIntensity * atten * visibility;
    // 光照后逐通道 multiplier：环境光底 + 漫反射 × 颜色
    vec3  litMul = vec3(uAmbient) + (1.0 - uAmbient) * diffK * uLightColor;
    // vLit = 1 → 应用上面的 multiplier；vLit = 0（线段）→ 直接用顶点色
    vec3  mult   = mix(vec3(1.0), litMul, vLit);
    fragColor    = vec4(vCol.rgb * mult, vCol.a);
}

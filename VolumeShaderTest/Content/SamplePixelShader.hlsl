Texture3D<float4> voxelTexture : register(t0);
SamplerState voxelSampler : register(s0);

cbuffer ConstantBuffer : register(b0)
{
    float4x4 worldMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 worldviewprojection;
    float4x4 invworldviewprojection;
    float4x4 invWorldMatrix;
    float4 cameraPosition;
    float4 lightPosition;
};

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float3 texCoord : TEXCOORD0;
    float3 localPos : TEXCOORD1;
};

float IGN(float2 uv)
{
    float3 magic = float3(0.06711056, 0.00583715, 52.9829189);
    return frac(magic.z * frac(dot(uv, magic.xy)));
}

float2 IntersectBox(float3 ro, float3 rd, float3 boxMin, float3 boxMax)
{
    float3 invRd = 1.0f / (rd + 1e-6f);
    float3 t0 = (boxMin - ro) * invRd;
    float3 t1 = (boxMax - ro) * invRd;
    
    // Explicitly find min and max to avoid compiler constructor errors
    float3 tMin = min(t0, t1);
    float3 tMax = max(t0, t1);
    
    float tNear = max(max(tMin.x, tMin.y), tMin.z);
    float tFar = min(min(tMax.x, tMax.y), tMax.z);
    return float2(tNear, tFar);
}

float4 main(PixelShaderInput input) : SV_Target
{
    // 1. Ray Setup
    float4 localCam = mul(float4(cameraPosition.xyz, 1.0f), invWorldMatrix);
    float3 rayDir = normalize(input.localPos - localCam.xyz);
    
    // Define box bounds explicitly as float3 constructors
    float3 bMin = float3(-0.5f, -0.5f, -0.5f);
    float3 bMax = float3(0.5f, 0.5f, 0.5f);
    float2 t = IntersectBox(localCam.xyz, rayDir, bMin, bMax);
    
    float tEntry = max(t.x, 0.0f);
    float tExit = t.y;

    if (tEntry > tExit)
        discard;

    // 2. Performance Tuning: 128 steps to stop the "chugging"
    const int steps = 128;
    float stepSize = (tExit - tEntry) / float(steps);
    
    float jitter = IGN(input.position.xy);
    float tCurrent = tEntry + (jitter * stepSize);
    
    float3 localLightPos = mul(float4(lightPosition.xyz, 1.0f), invWorldMatrix).xyz;
    float globalDensity = 0.12f;
    float4 accumulatedColor = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // 3. Main Raymarching Loop
    for (int i = 0; i < steps; i++)
    {
        float3 currentPos = localCam.xyz + rayDir * tCurrent;
        float4 voxel = voxelTexture.SampleLevel(voxelSampler, currentPos + 0.5f, 0);

        if (voxel.a > 0.001f)
        {
            // Lightweight 3-sample shadow loop
            float3 lightDir = normalize(localLightPos - currentPos);
            float lightAccum = 0;
            [unroll] // Performance hint for the GPU
            for (int j = 1; j <= 3; j++)
            {
                float3 shadowPos = currentPos + lightDir * (float(j) * 0.04f);
                if (all(shadowPos > bMin) && all(shadowPos < bMax))
                    lightAccum += voxelTexture.SampleLevel(voxelSampler, shadowPos + 0.5f, 0).a;
            }
            float shadow = exp(-lightAccum * 10.0f * 0.04f);
            
            float localAlpha = voxel.a * globalDensity;
            accumulatedColor.rgb += voxel.rgb * localAlpha * shadow * (1.0f - accumulatedColor.a);
            accumulatedColor.a += localAlpha * (1.0f - accumulatedColor.a);
        }

        if (accumulatedColor.a >= 0.99f)
            break;
        tCurrent += stepSize;
    }

    // Final Dither to hide banding
    accumulatedColor.rgb += (jitter - 0.5f) / 255.0f;
    return accumulatedColor;
}
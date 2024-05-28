cbuffer ConstantBuffer : register(b0)
{
    float4x4 modelViewProjection;
    float3 cameraPosition;
    float3 lightDirection;
    float4 lightColor;
};

Texture3D<float4> myTexture3D : register(t0);
SamplerState mySampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    // Ray casting parameters
    float3 rayStart = input.worldPos;
    float3 rayDir = normalize(input.worldPos - cameraPosition);

    // Maximum distance to cast the ray
    float maxDistance = 2.0f;

    // Step size along the ray
    float stepSize = 0.01f;

    // Initialize the color and alpha accumulation
    float4 accumulatedColor = float4(0, 0, 0, 0);
    float accumulatedAlpha = 0.0f;

    // Iterate along the ray
    for (float t = 0; t < maxDistance; t += stepSize)
    {
        float3 samplePos = rayStart + t * rayDir;

        // Sample the 3D texture
        float4 sampleColor = myTexture3D.Sample(mySampler, samplePos);

        // Perform front-to-back compositing
        accumulatedColor.rgb += (1.0f - accumulatedAlpha) * sampleColor.rgb * sampleColor.a;
        accumulatedAlpha += (1.0f - accumulatedAlpha) * sampleColor.a;

        // Terminate if the alpha value reaches a threshold
        if (accumulatedAlpha >= 0.95f)
        {
            break;
        }
    }

    // Output the accumulated color
    return float4(accumulatedColor.rgb, accumulatedAlpha);
}

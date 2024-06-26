Texture3D<float4> voxelTexture : register(t0);
SamplerState voxelSampler : register(s0);

cbuffer ConstantBuffer : register(b0)
{
    float4x4 worldMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 worldviewprojection;
    float4x4 invworldviewprojection;
};


struct PixelShaderInput
{
    float4 outPosition : SV_POSITION;
    float3 outTexCoord : TEXCOORD0;
    float4 normal : NORMAL;
};

float4 main(PixelShaderInput input) : SV_Target
{
    float4 sPos = input.outPosition / input.outPosition.w;
    float4 result = mul(sPos, invworldviewprojection);
    // Sample the 3D texture
    float4 voxelColor = voxelTexture.Sample(voxelSampler, input.outTexCoord);

    // Return the color of the voxel
    return float4(normalize(input.normal));
}

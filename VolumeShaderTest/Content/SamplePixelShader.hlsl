Texture3D<float4> voxelTexture : register(t0);
SamplerState voxelSampler : register(s0);

struct PixelShaderInput
{
    float4 outPosition : SV_POSITION;
    float3 outTexCoord : TEXCOORD0;
};

float4 main(PixelShaderInput input) : SV_Target
{
    // Sample the 3D texture
    float4 voxelColor = voxelTexture.Sample(voxelSampler, input.outTexCoord);

    // Return the color of the voxel
    return voxelColor;
}

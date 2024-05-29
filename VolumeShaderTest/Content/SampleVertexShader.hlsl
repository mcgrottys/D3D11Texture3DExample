cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
    matrix model;
    matrix view;
    matrix projection;
};

struct VertexShaderInput
{
    float3 inPos : POSITION;
    float3 inTexCoord : TEXCOORD; // Changed from float2 to float3
};

struct PixelShaderInput
{
    float4 outPosition : SV_POSITION;
    float3 outTexCoord : TEXCOORD0; // Changed to TEXCOORD0 to match the pixel shader
    float3 worldPos : TEXCOORD1; // Pass the world position
};

PixelShaderInput main(VertexShaderInput input)
{
    PixelShaderInput output;
    float4 pos = float4(input.inPos, 1.0f);

    float4 worldPos = mul(pos, model);

    pos = mul(worldPos, view);
    pos = mul(pos, projection);
    output.outPosition = pos;

    output.outTexCoord = input.inTexCoord; // Pass the 3D texture coordinate
    output.worldPos = worldPos.xyz; // Pass the world position

    return output;
}

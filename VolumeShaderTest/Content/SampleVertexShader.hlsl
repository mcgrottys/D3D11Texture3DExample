cbuffer ConstantBuffer : register(b0)
{
    float4x4 worldMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 worldviewprojection;
    float4x4 invworldviewprojection;
    float4x4 invWorldMatrix;
    float4 cameraPosition;
};

struct VS_INPUT
{
    float3 position : POSITION;
    float3 texCoord : TEXCOORD0;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 texCoord : TEXCOORD0;
    float3 localPos : TEXCOORD1;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    float4 worldPos = mul(float4(input.position, 1.0f), worldMatrix);
    output.position = mul(worldPos, mul(viewMatrix, projectionMatrix));
    output.texCoord = input.texCoord;
    output.localPos = input.position; // Pass raw coordinates for raymarching [cite: 26, 33]
    return output;
}
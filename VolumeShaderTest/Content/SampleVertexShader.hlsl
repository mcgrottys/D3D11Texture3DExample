cbuffer ConstantBuffer : register(b0)
{
    float4x4 worldMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 worldviewprojection;
    float4x4 invworldviewprojection;
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
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    
    // Transform the position to world space
    output.position = mul(float4(input.position, 1.0f), worldviewprojection);
    
    // Pass the texture coordinates to the pixel shader
    output.texCoord = input.texCoord;
    
    return output;
}

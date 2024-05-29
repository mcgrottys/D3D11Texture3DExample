cbuffer ConstantBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
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
    float4 worldPosition = mul(float4(input.position, 1.0f), worldMatrix);
    float4 viewPosition = mul(worldPosition, viewMatrix);
    output.position = mul(viewPosition, projectionMatrix);
    
    // Pass the texture coordinates to the pixel shader
    output.texCoord = input.texCoord;
    
    return output;
}

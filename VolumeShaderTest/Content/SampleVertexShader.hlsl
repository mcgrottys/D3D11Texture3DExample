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
    float4 normal : NORMAL;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
        // Transform the vertex position to world space, then to view space, and finally to projection space
    float4 worldPosition = mul(float4(input.position, 1.0f), worldMatrix);
    float4 viewPosition = mul(worldPosition, viewMatrix);
    float4 projPosition = mul(viewPosition, projectionMatrix);

    // Alternatively, you can transform directly with the combined World-View-Projection matrix
    // output.pos = mul(float4(input.pos, 1.0f), worldViewProjectionMatrix);
    // Transform the position to world space
    output.position = projPosition;
    float4 sPos = projPosition / projPosition.w;
    float4 result = mul(sPos, invworldviewprojection);
    output.normal = result;
    
    // Pass the texture coordinates to the pixel shader
    output.texCoord = input.texCoord;
    
    return output;
}

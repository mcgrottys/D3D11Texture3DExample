#pragma once

namespace VolumeShaderTest
{
    struct ModelViewProjectionConstantBuffer
    {
        DirectX::XMFLOAT4X4 worldMatrix;
        DirectX::XMFLOAT4X4 viewMatrix;
        DirectX::XMFLOAT4X4 projectionMatrix;
        DirectX::XMFLOAT4X4 worldViewProjectionMatrix;
        DirectX::XMFLOAT4X4 invWorldViewProjectionMatrix;
        DirectX::XMFLOAT4X4 invWorldMatrix; // For local space transformation
        DirectX::XMFLOAT4 cameraPosition;   // For ray origin
        DirectX::XMFLOAT4 lightPosition;    // For animated self-shadowing
    };

    struct VertexPositionColor
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT3 texCoord;
    };
}
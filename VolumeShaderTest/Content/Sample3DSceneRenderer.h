#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"

using namespace DirectX;
namespace VolumeShaderTest
{
	// This sample renderer instantiates a basic rendering pipeline.
	class Sample3DSceneRenderer
	{
	public:
		Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void CreateVolumetricTexture();
		void ReleaseDeviceDependentResources();
		void Update(DX::StepTimer const& timer);
		void Render();
		void StartTracking();
		void TrackingUpdate(float positionX);
		void StopTracking();
		bool IsTracking() { return m_tracking; }


	private:
		void Rotate(float radians);

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// Direct3D resources for cube geometry.
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_inputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShader;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader>	m_geometryShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShader;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constantBuffer;
		Microsoft::WRL::ComPtr<ID3D11Texture3D>		m_volumeTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>		m_volumeTextureView;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>		m_samplerState;
		Microsoft::WRL::ComPtr<ID3D11BlendState>		m_blendState;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>		m_rasterState;

		// System resources for cube geometry.
		ModelViewProjectionConstantBuffer	m_constantBufferData;
		XMMATRIX	m_projectionMatrix;
		XMMATRIX	m_viewMatrix;
		XMMATRIX	m_worldMatrix;
		XMMATRIX	m_worldViewProjectionMatrix;
		XMMATRIX	m_invWorldViewProjectionMatrix;
		uint32	m_indexCount;
		uint32	m_vertexCount;

		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		float	m_degreesPerSecond;
		bool	m_tracking;
	};
}


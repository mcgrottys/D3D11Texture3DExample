#include "pch.h"
#include "Sample3DSceneRenderer.h"

#include "..\Common\DirectXHelper.h"

using namespace VolumeShaderTest;

using namespace DirectX;
using namespace Windows::Foundation;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_degreesPerSecond(45),
	m_indexCount(0),
	m_tracking(false),
	m_deviceResources(deviceResources)
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// Adjust FOV for portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// Right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
	);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();
	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	// Combine perspective and orientation matrices.
	m_projectionMatrix = perspectiveMatrix * orientationMatrix;
	XMStoreFloat4x4(&m_constantBufferData.projectionMatrix, XMMatrixTranspose(m_projectionMatrix));

	// Define the view matrix.
	static const XMVECTORF32 eye = { 0.0f, 0.7f, -1.3f, 0.0f };
	static const XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	m_viewMatrix = XMMatrixLookAtLH(eye, at, up);
	XMStoreFloat4x4(&m_constantBufferData.viewMatrix, XMMatrixTranspose(m_viewMatrix));

	// Define the world matrix.
	m_worldMatrix = XMMatrixIdentity();
	XMStoreFloat4x4(&m_constantBufferData.worldMatrix, XMMatrixTranspose(m_worldMatrix));

	// Calculate the World-View-Projection matrix.
	m_worldViewProjectionMatrix = XMMatrixMultiply(XMMatrixMultiply(m_worldMatrix, m_viewMatrix), m_projectionMatrix);
	XMStoreFloat4x4(&m_constantBufferData.worldViewProjectionMatrix, XMMatrixTranspose(m_worldViewProjectionMatrix));

	// Calculate the inverse World-View-Projection matrix.
	m_invWorldViewProjectionMatrix = XMMatrixInverse(nullptr, m_worldViewProjectionMatrix);
	XMStoreFloat4x4(&m_constantBufferData.invWorldViewProjectionMatrix, XMMatrixTranspose(m_invWorldViewProjectionMatrix));
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
	if (!m_tracking)
	{
		// Convert degrees to radians, then convert seconds to rotation angle
		float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
		double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
		float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

		//Rotate(radians);
	}
}

// Rotate the 3D cube model a set amount of radians.
void Sample3DSceneRenderer::Rotate(float radians)
{
	// Prepare to pass the updated model matrix to the shader
	m_worldMatrix = XMMatrixRotationY(radians);
	XMStoreFloat4x4(&m_constantBufferData.worldMatrix, XMMatrixTranspose(m_worldMatrix));

	// Calculate the World-View-Projection matrix.
	m_worldViewProjectionMatrix = XMMatrixMultiply(XMMatrixMultiply(m_worldMatrix, m_viewMatrix), m_projectionMatrix);
	XMStoreFloat4x4(&m_constantBufferData.worldViewProjectionMatrix, XMMatrixTranspose(m_worldViewProjectionMatrix));

	// Calculate the inverse World-View-Projection matrix.
	m_invWorldViewProjectionMatrix = XMMatrixInverse(nullptr, m_worldViewProjectionMatrix);
	XMStoreFloat4x4(&m_constantBufferData.invWorldViewProjectionMatrix, XMMatrixTranspose(m_invWorldViewProjectionMatrix));
}

void Sample3DSceneRenderer::StartTracking()
{
	m_tracking = true;
}

// When tracking, the 3D cube can be rotated around its Y axis by tracking pointer position relative to the output screen width.
void Sample3DSceneRenderer::TrackingUpdate(float positionX)
{
	if (m_tracking)
	{
		float radians = XM_2PI * 2.0f * positionX / m_deviceResources->GetOutputSize().Width;
		Rotate(radians);
	}
}

void Sample3DSceneRenderer::StopTracking()
{
	m_tracking = false;
}

// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render()
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(
		m_constantBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData,
		0,
		0,
		0
		);

	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		m_vertexBuffer.GetAddressOf(),
		&stride,
		&offset
		);

	context->IASetIndexBuffer(
		m_indexBuffer.Get(),
		DXGI_FORMAT_R32_UINT, // Each index is one unsigned integer.
		0
		);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->IASetInputLayout(m_inputLayout.Get());

	// Attach our vertex shader.
	context->VSSetShader(
		m_vertexShader.Get(),
		nullptr,
		0
		);
	//context->GSSetShader(m_geometryShader.Get(), nullptr, 0);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
		);

	// Attach our pixel shader.
	context->PSSetShader(
		m_pixelShader.Get(),
		nullptr,
		0
		);

	context->PSSetConstantBuffers(
		0,
		1,
		m_constantBuffer.GetAddressOf());

	context->PSSetShaderResources(0, 1, m_volumeTextureView.GetAddressOf());
	context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());
	// Send the constant buffer to the graphics device.
	// Bind the blend state
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	UINT sampleMask = 0xffffffff;
	//context->OMSetBlendState(m_blendState.Get(), blendFactor, sampleMask);



	// Step 3: Set the rasterizer state
	//context->RSSetState(m_rasterState.Get());
	// Draw the objects.
	context->DrawIndexed(
		m_indexCount,
		0,
		0
		);
}

void GenerateNestedCubes(std::vector<VertexPositionColor>& vertices, std::vector<unsigned int>& indices,
	XMFLOAT3 min, XMFLOAT3 max, int level, uint32& currentIndex) {


	float cubeCntWidth = 1.0f;
	float cubeSize = 1.0f / cubeCntWidth;
	unsigned int indexOffset = 0;

	for (float z = 0; z < cubeCntWidth; z++)
	{
		for (float y = 0; y < cubeCntWidth; y++)
		{
			for (float x = 0; x < cubeCntWidth; x++)
			{
				float startX = min.x + x * cubeSize;
				float endX = startX + cubeSize;
				float startY = min.y + y * cubeSize;
				float endY = startY + cubeSize;
				float startZ = min.z + z * cubeSize;
				float endZ = startZ + cubeSize;

				float startXt = x * cubeSize;
				float endXt = startXt + cubeSize;
				float startYt = y + y * cubeSize;
				float endYt = startYt + cubeSize;
				float startZt = z + z * cubeSize;
				float endZt = startZt + cubeSize;

				// Define vertices for the current cube
				vertices.push_back({ XMFLOAT3(startX, startY, startZ), XMFLOAT3(startXt, startYt, startZt)});
				vertices.push_back({ XMFLOAT3(startX, startY,  endZ), XMFLOAT3(startXt, startYt, endZt) });
				vertices.push_back({ XMFLOAT3(startX,  endY, startZ), XMFLOAT3(startXt, endYt, startZt) });
				vertices.push_back({ XMFLOAT3(startX, endY, endZ), XMFLOAT3(startXt, endYt, endZt) });
				vertices.push_back({ XMFLOAT3(endX, startY, startZ), XMFLOAT3(endXt, startYt, startZt) });
				vertices.push_back({ XMFLOAT3(endX, startY, endZ), XMFLOAT3(endXt, startYt, endZt) });
				vertices.push_back({ XMFLOAT3(endX, endY, startZ), XMFLOAT3(endXt, endYt, startZt) });
				vertices.push_back({ XMFLOAT3(endX, endY, endZ), XMFLOAT3(endXt, endYt, endZt) });

				// Define indices for the current cube
				unsigned int cubeIndices[] =
				{
					indexOffset,2 + indexOffset,  1+indexOffset, // -x
					1+ indexOffset,2+ indexOffset,3+ indexOffset,

					4+ indexOffset,5+ indexOffset,6+ indexOffset, // +x
					5+ indexOffset,7+ indexOffset,6+ indexOffset,

					0+ indexOffset,1+ indexOffset,5+ indexOffset, // -y
					0+ indexOffset,5+ indexOffset,4+ indexOffset,

					2+ indexOffset,6+ indexOffset,7+ indexOffset, // +y
					2+ indexOffset,7+ indexOffset,3+ indexOffset,

					0+ indexOffset,4+ indexOffset,6+ indexOffset, // -z
					0+ indexOffset,6+ indexOffset,2+ indexOffset,

					1+ indexOffset,3+ indexOffset,7+ indexOffset, // +z
					1+ indexOffset,7+ indexOffset,5+ indexOffset,
				};

				indices.insert(indices.end(), std::begin(cubeIndices), std::end(cubeIndices));

				indexOffset = vertices.size();
			}
		}
	}
	currentIndex = indices.size();
}
void Sample3DSceneRenderer::CreateDeviceDependentResources()
{
	// Load shaders asynchronously.
	auto loadVSTask = DX::ReadDataAsync(L"SampleVertexShader.cso");
	auto loadPSTask = DX::ReadDataAsync(L"SamplePixelShader.cso");
	//auto loadGSTask = DX::ReadDataAsync(L"GeometryShader.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_vertexShader
				)
			);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_inputLayout
				)
			);
	});

	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_pixelShader
				)
			);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer) , D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
				)
			);
	});

	//auto createGSTask = loadGSTask.then([this](const std::vector<byte>& fileData) {
	//	DX::ThrowIfFailed(
	//		m_deviceResources->GetD3DDevice()->CreateGeometryShader(
	//			&fileData[0],
	//			fileData.size(),
	//			nullptr,
	//			&m_geometryShader
	//		)
	//	);
	//});
	// Once both shaders are loaded, create the mesh.
	auto createCubeTask = (createPSTask && createVSTask).then([this] () {

		std::vector<VertexPositionColor> vertices;
		std::vector<unsigned int> indices;
		unsigned int currentIndex = 0;
		GenerateNestedCubes(vertices, indices, XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.5f, 0.5f, 0.5f), 0, m_indexCount);

		//// Load mesh vertices. Each vertex has a position and a color.
		//static const VertexPositionColor cubeVertices[] = 
		//{
		//	{XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)},
		//	{XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)},
		//	{XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
		//	{XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f)},
		//	{XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)},
		//	{XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f)},
		//	{XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)},
		//	{XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)},
		//};

				// Load mesh vertices. Each vertex has a position and a color.

		D3D11_SUBRESOURCE_DATA vertexBufferData = {0};
		vertexBufferData.pSysMem = vertices.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(vertices.size() * sizeof(VertexPositionColor), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_vertexBuffer
				)
			);

		// Load mesh indices. Each trio of indices represents
		// a triangle to be rendered on the screen.
		// For example: 0,2,1 means that the vertices with indexes
		// 0, 2 and 1 from the vertex buffer compose the 
		// first triangle of this mesh.

		D3D11_SUBRESOURCE_DATA indexBufferData = {0};
		indexBufferData.pSysMem = indices.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(indices.size() * sizeof(unsigned int), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&m_indexBuffer
				)
			);
	});

	// Once the cube is loaded, the object is ready to be rendered.
	createCubeTask.then([this] () {
		CreateVolumetricTexture();
		m_loadingComplete = true;
	});


}


void Sample3DSceneRenderer::CreateVolumetricTexture()
{
	const int textureWidth = 256;
	const int textureHeight = 256;
	const int textureDepth = 256;

	D3D11_TEXTURE3D_DESC textureDesc = {};
	textureDesc.Width = textureWidth;
	textureDesc.Height = textureHeight;
	textureDesc.Depth = textureDepth;
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	std::vector<float> textureData(textureWidth * textureHeight * textureDepth * 4, 0.0f);

	const float radius = textureWidth / 4.0f;
	const float centerX = textureWidth / 2.0f;
	const float centerY = textureHeight / 2.0f;
	const float centerZ = textureDepth / 2.0f;

	for (float z = 0; z < textureDepth; ++z)
	{
		for (float y = 0; y < textureHeight; ++y)
		{
			for (float x = 0; x < textureWidth; ++x)
			{
				float dx = fabsf(centerX - x);;
				float dy = fabsf(centerY - y);
				float dz = fabsf(centerZ - z);
				float distance = sqrt((dx * dx )+ (dy * dy) + (dz * dz));

				if(x<y)
				{
					float value = (distance < radius) ? 1.0f : 0.0f; // Use 1.0f instead of 255 for float format
					int index = (z * textureHeight * textureWidth + y * textureWidth + x) * 4;
					textureData[index] = 0.4f;     // R
					textureData[index + 1] = 1.0f; // Green
					textureData[index + 2] = 0.3f; // B
					textureData[index + 3] = 0.8f; // Alpha, scaled to float range 0.0f to 1.0f
				}
				else
				{
					int index = (z * textureHeight * textureWidth + y * textureWidth + x) * 4;
					textureData[index] = 0.0f; // R
					textureData[index + 1] = 0.5f; // Green
					textureData[index + 2] = 0.6f; // B
					textureData[index + 3] = 0.8f; // Alpha
				}
			}
		}
	}

	D3D11_SUBRESOURCE_DATA initialData = {};
	initialData.pSysMem = textureData.data();
	initialData.SysMemPitch = textureWidth * 4 * sizeof(float);
	initialData.SysMemSlicePitch = textureWidth * textureHeight * 4 * sizeof(float);

	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateTexture3D(&textureDesc, &initialData, &m_volumeTexture)
	);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	srvDesc.Texture3D.MostDetailedMip = 0;
	srvDesc.Texture3D.MipLevels = textureDesc.MipLevels;

	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateShaderResourceView(m_volumeTexture.Get(), &srvDesc, &m_volumeTextureView)
	);

	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	//// Step 2: Create the sampler state
	//HRESULT hr = m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, &m_samplerState);
	//if (FAILED(hr)) {
	//	// Handle the error (e.g., log it and clean up resources)
	//}
	//// Create the blend state for transparency
	//D3D11_BLEND_DESC blendDesc = {};
	//blendDesc.RenderTarget[0].BlendEnable = TRUE;
	//blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	//blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	//blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	//blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	//blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	//blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	//blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	//DX::ThrowIfFailed(
	//	m_deviceResources->GetD3DDevice()->CreateBlendState(
	//		&blendDesc,
	//		&m_blendState
	//	)
	//);
	//// Step 1: Define the rasterizer state description
	//D3D11_RASTERIZER_DESC rasterDesc;
	//ZeroMemory(&rasterDesc, sizeof(rasterDesc));
	//rasterDesc.FillMode = D3D11_FILL_SOLID; // or D3D11_FILL_WIREFRAME if you want to see wireframe
	//rasterDesc.CullMode = D3D11_CULL_NONE;  // Disable backface culling
	//rasterDesc.FrontCounterClockwise = false;
	//rasterDesc.DepthBias = 0;
	//rasterDesc.SlopeScaledDepthBias = 0.0f;
	//rasterDesc.DepthBiasClamp = 0.0f;
	//rasterDesc.DepthClipEnable = true;
	//rasterDesc.ScissorEnable = false;
	//rasterDesc.MultisampleEnable = false;
	//rasterDesc.AntialiasedLineEnable = false;

	//// Step 2: Create the rasterizer state
	//hr = m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rasterDesc, &m_rasterState);
	//if (FAILED(hr))
	//{
	//	// Handle the error (e.g., by logging or throwing an exception)
	//}

	// Remember to release the rasterizer state when done

}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_vertexShader.Reset();
	m_inputLayout.Reset();
	m_pixelShader.Reset();
	m_constantBuffer.Reset();
	m_vertexBuffer.Reset();
	m_indexBuffer.Reset();

}
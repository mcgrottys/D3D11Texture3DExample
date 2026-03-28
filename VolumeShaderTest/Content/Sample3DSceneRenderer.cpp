#include "pch.h"
#include "Sample3DSceneRenderer.h"
#include "Common\DirectXHelper.h"

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

	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// --- PROJECTION MATRIX (Clipping Planes) ---
	// Parameters: FOV, Aspect Ratio, Near Plane, Far Plane
	// Decreasing the near plane to 0.001f prevents clipping when very close.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(
		fovAngleY,
		aspectRatio,
		0.001f,   // Near Clipping Plane (Decreased)
		500.0f    // Far Clipping Plane (Increased)
	);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();
	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);
	m_projectionMatrix = perspectiveMatrix * orientationMatrix;
	XMStoreFloat4x4(&m_constantBufferData.projectionMatrix, XMMatrixTranspose(m_projectionMatrix));

	// --- VIEW MATRIX (Camera Position) ---
	// Change the Z value from -1.3f to -3.0f to move the camera further away.
	static const XMVECTORF32 eye = { 0.0f, 0.7f, -3.0f, 0.0f }; // Moved further back
	static const XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	m_viewMatrix = XMMatrixLookAtLH(eye, at, up);
	XMStoreFloat4x4(&m_constantBufferData.viewMatrix, XMMatrixTranspose(m_viewMatrix));

	// IMPORTANT: Update the camera position in the constant buffer for the raymarcher
	XMStoreFloat4(&m_constantBufferData.cameraPosition, eye);

	// --- WORLD MATRIX ---
	m_worldMatrix = XMMatrixIdentity();
	XMStoreFloat4x4(&m_constantBufferData.worldMatrix, XMMatrixTranspose(m_worldMatrix));

	XMMATRIX invWorld = XMMatrixInverse(nullptr, m_worldMatrix);
	XMStoreFloat4x4(&m_constantBufferData.invWorldMatrix, XMMatrixTranspose(invWorld));

	m_worldViewProjectionMatrix = XMMatrixMultiply(XMMatrixMultiply(m_worldMatrix, m_viewMatrix), m_projectionMatrix);
	XMStoreFloat4x4(&m_constantBufferData.worldViewProjectionMatrix, XMMatrixTranspose(m_worldViewProjectionMatrix));

	m_invWorldViewProjectionMatrix = XMMatrixInverse(nullptr, m_worldViewProjectionMatrix);
	XMStoreFloat4x4(&m_constantBufferData.invWorldViewProjectionMatrix, XMMatrixTranspose(m_invWorldViewProjectionMatrix));
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
	if (!m_tracking)
	{
		float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
		double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
		float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

		Rotate(radians);
	}

	// --- NEW: LIGHT ANIMATION ---
	float lightOrbitSpeed = 1.2f;
	float lightTime = static_cast<float>(timer.GetTotalSeconds()) * lightOrbitSpeed;

	// Calculate a circular orbit for the light
	// We place it at a distance of 2.0 units from the center
	m_constantBufferData.lightPosition = XMFLOAT4(
		2.0f * cos(lightTime),
		1.5f,                  // Keep it slightly above the sphere
		2.0f * sin(lightTime),
		1.0f
	);
}

// Rotate the 3D cube model a set amount of radians.
void Sample3DSceneRenderer::Rotate(float radians)
{
	// Prepare to pass the updated model matrix to the shader
	m_worldMatrix = XMMatrixRotationY(radians);
	XMStoreFloat4x4(&m_constantBufferData.worldMatrix, XMMatrixTranspose(m_worldMatrix));

	// Update Inverse World Matrix when rotating
	XMMATRIX invWorld = XMMatrixInverse(nullptr, m_worldMatrix);
	XMStoreFloat4x4(&m_constantBufferData.invWorldMatrix, XMMatrixTranspose(invWorld));

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

	// Preparereat the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(
		m_constantBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData,
		0,
		0,
		0
	);

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
		DXGI_FORMAT_R32_UINT,
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

	// Bind the blend state for volume accumulation
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	UINT sampleMask = 0xffffffff;
	context->OMSetBlendState(m_blendState.Get(), blendFactor, sampleMask);

	// Set the rasterizer state for Front-Face Culling
	context->RSSetState(m_rasterState.Get());

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
				vertices.push_back({ XMFLOAT3(startX, startY, startZ), XMFLOAT3(startXt, startYt, startZt) });
				vertices.push_back({ XMFLOAT3(startX, startY,  endZ), XMFLOAT3(startXt, startYt, endZt) });
				vertices.push_back({ XMFLOAT3(startX,  endY, startZ), XMFLOAT3(startXt, endYt, startZt) });
				vertices.push_back({ XMFLOAT3(startX, endY, endZ), XMFLOAT3(startXt, endYt, endZt) });
				vertices.push_back({ XMFLOAT3(endX, startY, startZ), XMFLOAT3(endXt, startYt, startZt) });
				vertices.push_back({ XMFLOAT3(endX, startY, endZ), XMFLOAT3(endXt, startYt, endZt) });
				vertices.push_back({ XMFLOAT3(endX, endY, startZ), XMFLOAT3(endXt, endYt, startZt) });
				vertices.push_back({ XMFLOAT3(endX, endY, endZ), XMFLOAT3(endXt, endYt, endZt) });

				// Define corrected indices for outward-facing triangles
// Corrected cube indices (Uniform Clockwise Winding)
				unsigned int cubeIndices[] =
				{
					// -X (Left Face)
					indexOffset + 1, indexOffset + 3, indexOffset + 2, indexOffset + 1, indexOffset + 2, indexOffset + 0,

					// +X (Right Face)
					indexOffset + 4, indexOffset + 6, indexOffset + 7, indexOffset + 4, indexOffset + 7, indexOffset + 5,

					// -Y (Bottom Face)
					indexOffset + 1, indexOffset + 0, indexOffset + 4, indexOffset + 1, indexOffset + 4, indexOffset + 5,

					// +Y (Top Face)
					indexOffset + 2, indexOffset + 3, indexOffset + 7, indexOffset + 2, indexOffset + 7, indexOffset + 6,

					// -Z (Front Face)
					indexOffset + 0, indexOffset + 2, indexOffset + 6, indexOffset + 0, indexOffset + 6, indexOffset + 4,

					// +Z (Back Face)
					indexOffset + 5, indexOffset + 7, indexOffset + 3, indexOffset + 5, indexOffset + 3, indexOffset + 1
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

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
			)
		);
		});

	// Once both shaders are loaded, create the mesh.
	auto createCubeTask = (createPSTask && createVSTask).then([this]() {

		std::vector<VertexPositionColor> vertices;
		std::vector<unsigned int> indices;
		GenerateNestedCubes(vertices, indices, XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.5f, 0.5f, 0.5f), 0, m_indexCount);

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
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

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
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

		// Create the rasterizer state for front culling
		D3D11_RASTERIZER_DESC rasterDesc;
		ZeroMemory(&rasterDesc, sizeof(rasterDesc));
		rasterDesc.FillMode = D3D11_FILL_SOLID;
		rasterDesc.CullMode = D3D11_CULL_FRONT; // Cull front faces to see the back of the box
		rasterDesc.FrontCounterClockwise = false;
		rasterDesc.DepthBias = 0;
		rasterDesc.SlopeScaledDepthBias = 0.0f;
		rasterDesc.DepthBiasClamp = 0.0f;
		rasterDesc.DepthClipEnable = true;
		rasterDesc.ScissorEnable = false;
		rasterDesc.MultisampleEnable = false;
		rasterDesc.AntialiasedLineEnable = false;

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rasterDesc, &m_rasterState)
		);

		// Create the blend state for transparency accumulation
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBlendState(&blendDesc, &m_blendState)
		);
		});
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // Important: Don't write to depth!
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

	// Create and bind this state in your Render() function
	m_deviceResources->GetD3DDevice()->CreateDepthStencilState(&dsDesc, &m_depthStencilState);
	// Once the cube is loaded, the object is ready to be rendered.
	createCubeTask.then([this]() {
		CreateVolumetricTexture();
		m_loadingComplete = true;
		});
}
// Add this helper at the top of Sample3DSceneRenderer.cpp to generate fog noise
float FractalNoise(float x, float y, float z) {
	auto hash = [](int n) {
		n = (n << 13) ^ n;
		return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
		};
	float h = hash(static_cast<int>(x + y * 57 + z * 113));
	float h2 = hash(static_cast<int>(x * 2 + y * 114 + z * 226)) * 0.5f;
	return (h + h2 + 1.5f) / 3.0f;
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

	std::vector<float> textureData(textureWidth * textureHeight * textureDepth * 4, 0.0f);

	const float centerX = textureWidth / 2.0f;
	const float centerY = textureHeight / 2.0f;
	const float centerZ = textureDepth / 2.0f;
	const float maxRadius = textureWidth / 2.0f;

	for (int z = 0; z < textureDepth; ++z) {
		for (int y = 0; y < textureHeight; ++y) {
			for (int x = 0; x < textureWidth; ++x) {
				float dx = centerX - (float)x;
				float dy = centerY - (float)y;
				float dz = centerZ - (float)z;
				float dist = sqrt((dx * dx) + (dy * dy) + (dz * dz));

				// FIXED: Using a ternary operator to avoid the std::max macro conflict
				float rawAlpha = 1.0f - (dist / maxRadius);
				float sphereAlpha = (rawAlpha > 0.0f) ? rawAlpha : 0.0f;

				// Add fractal noise for the "actual fog" look
				float noise = FractalNoise((float)x * 0.15f, (float)y * 0.15f, (float)z * 0.15f);
				float finalAlpha = pow(sphereAlpha * noise, 1.5f);

				// Smooth color blending logic
				float factor = ((float)(y - x) / 30.0f) * 0.5f + 0.5f;
				factor = (factor < 0.0f) ? 0.0f : (factor > 1.0f ? 1.0f : factor);

				float r = (1.0f - factor) * 0.0f + factor * 0.4f;
				float g = (1.0f - factor) * 0.5f + factor * 1.0f;
				float b = (1.0f - factor) * 0.6f + factor * 0.3f;

				int index = (z * textureHeight * textureWidth + y * textureWidth + x) * 4;
				textureData[index] = r;
				textureData[index + 1] = g;
				textureData[index + 2] = b;
				textureData[index + 3] = finalAlpha;
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
	srvDesc.Texture3D.MipLevels = 1;

	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateShaderResourceView(m_volumeTexture.Get(), &srvDesc, &m_volumeTextureView)
	);

	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, &m_samplerState)
	);
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
	m_rasterState.Reset();
	m_blendState.Reset();
}

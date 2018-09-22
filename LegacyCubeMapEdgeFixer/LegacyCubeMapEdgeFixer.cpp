#include "pch.h"
#include <wrl/client.h>
#include "compiledShaders/ps.h"
#include "compiledShaders/vs.h"
#include "compiledShaders/gs.h"
#include "SimpleMath.h"
#include "SimpleMath.inl"
#include <DXProgrammableCapture.h>
#include <dxgi1_3.h>

using namespace Microsoft::WRL;
using namespace DirectX::SimpleMath;

#pragma pack(push,1)
struct QuadVertex
{
    XMFLOAT4 Position;
};

struct Uniforms
{
    int CurrentMipLevel;
    float pad[3];
};

#pragma pack(pop)

#define CHECK(call) { HRESULT _hr_ = (call); if (FAILED(_hr_)) throw std::runtime_error(#call); }

int main()
{
    try
    {
        const auto inputFilePath = L"C:\\temp\\villa_nova_street_cube_irradiance.dds";

        // Create DXGI factory
        ComPtr<IDXGIFactory1> dxgiFactory;
        CHECK(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(dxgiFactory.GetAddressOf())));

        ComPtr<IDXGraphicsAnalysis> graphicsAnalysis;
        DXGIGetDebugInterface1(0, __uuidof(IDXGraphicsAnalysis), reinterpret_cast<void**>(graphicsAnalysis.GetAddressOf()));

        if (graphicsAnalysis)
            graphicsAnalysis->BeginCapture();

        // Create D3D11 device and context
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;

        UINT createDeviceFlags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_DRIVER_TYPE driverTypes[] =
        {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP,
            D3D_DRIVER_TYPE_REFERENCE,
        };

        const UINT numDriverTypes = ARRAYSIZE(driverTypes);

        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
        };

        const UINT numFeatureLevels = ARRAYSIZE(featureLevels);

        D3D_FEATURE_LEVEL maxFeatureLevel;

        for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
        {
            const auto driverType = driverTypes[driverTypeIndex];

            HRESULT hr = D3D11CreateDevice(nullptr, driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                D3D11_SDK_VERSION, &device, &maxFeatureLevel, &context);

            if (SUCCEEDED(hr))
                break;
        }

        if (!device)
            throw std::runtime_error("No suitable device found");

        // Load source map
        TexMetadata sourceMetaData;
        ScratchImage sourceScratchImage;

        ComPtr<ID3D11ShaderResourceView> inputSrv;
        CHECK(GetMetadataFromDDSFile(inputFilePath, DDS_FLAGS_NONE, sourceMetaData));
        CHECK(LoadFromDDSFile(inputFilePath, DDS_FLAGS_NONE, &sourceMetaData, sourceScratchImage));
        CHECK(CreateShaderResourceView(device.Get(), sourceScratchImage.GetImages(), sourceScratchImage.GetImageCount(), sourceMetaData, &inputSrv));

        // Create the shaders
        ComPtr<ID3D11PixelShader> pixelShader;
        CHECK(device->CreatePixelShader(g_ps, sizeof(g_ps), nullptr, &pixelShader));

        ComPtr<ID3D11VertexShader> vertexShader;
        CHECK(device->CreateVertexShader(g_vs, sizeof(g_vs), nullptr, &vertexShader));

        ComPtr<ID3D11GeometryShader> geometryShader;
        CHECK(device->CreateGeometryShader(g_gs, sizeof(g_gs), nullptr, &geometryShader));

        // Create a quad for rendering
        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        // Create the input layout
        ComPtr<ID3D11InputLayout> vertexLayout;
        CHECK(device->CreateInputLayout(layout, _countof(layout), g_vs, sizeof(g_vs), &vertexLayout));

        // Create quad vertex buffer
        static const QuadVertex verticesQuad[] =
        {
            { XMFLOAT4(+1, +1, 0.5, 1) },
            { XMFLOAT4(+1, -1, 0.5, 1) },
            { XMFLOAT4(-1, -1, 0.5, 1) },
            { XMFLOAT4(-1, +1, 0.5, 1) },
        };

        D3D11_BUFFER_DESC bufferDesc = {};
        D3D11_SUBRESOURCE_DATA bufferData = {};

        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = sizeof(QuadVertex) * _countof(verticesQuad);
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;

        bufferData.pSysMem = &verticesQuad;

        ComPtr<ID3D11Buffer> vertexBuffer;
        CHECK(device->CreateBuffer(&bufferDesc, &bufferData, &vertexBuffer));

        // Create index buffer
        const WORD indicesQuad[] =
        {
            0, 1, 2,
            2, 3, 0
        };

        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = _countof(indicesQuad) * sizeof(WORD);
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;

        bufferData.pSysMem = &indicesQuad;

        ComPtr<ID3D11Buffer> indexBuffer;
        CHECK(device->CreateBuffer(&bufferDesc, &bufferData, &indexBuffer));

        // Create the constant buffers
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.ByteWidth = sizeof(Uniforms);
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        ComPtr<ID3D11Buffer> uniformBuffer;
        CHECK(device->CreateBuffer(&bufferDesc, nullptr, &uniformBuffer));

        // Create the target cube map
        // Create the target cube map TextureCube (array of 6 textures)
        D3D11_TEXTURE2D_DESC outputTexDesc = {};
        outputTexDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        outputTexDesc.Width = UINT(sourceMetaData.width);
        outputTexDesc.Height = UINT(sourceMetaData.height);
        outputTexDesc.ArraySize = 6;
        outputTexDesc.MipLevels = UINT(sourceMetaData.mipLevels);
        outputTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        outputTexDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
        outputTexDesc.SampleDesc = { 1,0 };
        outputTexDesc.Usage = D3D11_USAGE_DEFAULT;
        outputTexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        ComPtr<ID3D11Texture2D> outputTexture;
        CHECK(device->CreateTexture2D(&outputTexDesc, nullptr, &outputTexture));

        // Create the SRV for the texture cube
        D3D11_SHADER_RESOURCE_VIEW_DESC outputSrvDesc = {};
        outputSrvDesc.Format = outputTexDesc.Format;
        outputSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        outputSrvDesc.TextureCube.MostDetailedMip = 0;
        outputSrvDesc.TextureCube.MipLevels = -1;
        ComPtr<ID3D11ShaderResourceView> outputSrv;
        CHECK(device->CreateShaderResourceView(outputTexture.Get(), &outputSrvDesc, &outputSrv));

        // Create the input sampler 
        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MinLOD = 0;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
        ComPtr<ID3D11SamplerState> samplerState;
        CHECK(device->CreateSamplerState(&sampDesc, &samplerState));

        //// Generate cube map view matrices
        //auto& cubeViews = uniforms.CubeFaceMatrix;
        //cubeViews[0] = Matrix::Identity; //Matrix::CreateLookAt(Vector3::Zero, Vector3::Right, Vector3::Up) * proj;
        //cubeViews[1] = Matrix::Identity; //Matrix::CreateLookAt(Vector3::Zero, Vector3::Left, Vector3::Up) * proj;
        //cubeViews[2] = Matrix::Identity; //Matrix::CreateLookAt(Vector3::Zero, Vector3::Up, Vector3::Forward) * proj;
        //cubeViews[3] = Matrix::Identity; //Matrix::CreateLookAt(Vector3::Zero, Vector3::Down, Vector3::Forward) * proj;
        //cubeViews[4] = Matrix::Identity; //Matrix::CreateLookAt(Vector3::Zero, Vector3::Backward, Vector3::Up) * proj;
        //cubeViews[5] = Matrix::Identity; //Matrix::CreateLookAt(Vector3::Zero, Vector3::Forward, Vector3::Up) * proj;

        // Render the quad to all cube faces
        context->VSSetShader(vertexShader.Get(), nullptr, 0);
        context->PSSetShader(pixelShader.Get(), nullptr, 0);
        context->GSSetShader(geometryShader.Get(), nullptr, 0);
        context->PSSetConstantBuffers(0, 1, uniformBuffer.GetAddressOf());
        context->GSSetConstantBuffers(0, 1, uniformBuffer.GetAddressOf());
        context->PSSetShaderResources(0, 1, inputSrv.GetAddressOf());
        context->PSSetSamplers(0, 1, samplerState.GetAddressOf());

        // Set the input layout
        context->IASetInputLayout(vertexLayout.Get());

        // Set index buffer
        context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

        // Set primitive topology
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Set vertex buffer
        UINT stride = sizeof(QuadVertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);

        // Render all cube faces at all mip levels
        for (int mipLevel=0; mipLevel<outputTexDesc.MipLevels; ++mipLevel)
        {
            // Create the RTVs
            D3D11_RENDER_TARGET_VIEW_DESC outputRtvDesc = {};
            outputRtvDesc.Format = outputTexDesc.Format;
            outputRtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            outputRtvDesc.Texture2DArray.MipSlice = mipLevel;
            outputRtvDesc.Texture2DArray.FirstArraySlice = 0;
            outputRtvDesc.Texture2DArray.ArraySize = 6;
            ComPtr<ID3D11RenderTargetView> outputRtv;
            CHECK(device->CreateRenderTargetView(outputTexture.Get(), &outputRtvDesc, &outputRtv));

            context->OMSetRenderTargets(1, outputRtv.GetAddressOf(), nullptr);

            D3D11_VIEWPORT viewport = {};
            viewport.Width = float(outputTexDesc.Width >> mipLevel);
            viewport.Height = float(outputTexDesc.Height >> mipLevel);
            context->RSSetViewports(1, &viewport);

            Uniforms uniforms = {};
            uniforms.CurrentMipLevel = mipLevel;
            context->UpdateSubresource(uniformBuffer.Get(), 0, nullptr, &uniforms, 0, 0);

            context->DrawIndexed(_countof(indicesQuad), 0, 0);

            context->Flush();
        }

        if (graphicsAnalysis)
            graphicsAnalysis->EndCapture();

        // Save the image.
        ScratchImage outputImage;
        CHECK(CaptureTexture(device.Get(), context.Get(), outputTexture.Get(), outputImage));
        CHECK(SaveToDDSFile(outputImage.GetImages(), outputImage.GetImageCount(), outputImage.GetMetadata(), DDS_FLAGS_NONE, L"C:\\Temp\\Output.dds"));

        std::cout << "Done" << std::endl;
    }

    catch (std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }

    //getchar();
}


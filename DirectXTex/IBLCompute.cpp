//-------------------------------------------------------------------------------------
// BCDirectCompute.cpp
//  
// Direct3D 11 Compute Shader BC Compressor
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#include "DirectXTexp.h"

#include "IBLCompute.h"

#if defined(_DEBUG) || defined(PROFILE)
#pragma comment(lib,"dxguid.lib")
#endif

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
#include "Shaders\Compiled\IBLSampler_VS_CubeMap.inc"
#include "Shaders\Compiled\IBLSampler_GS_CubeMap.inc"
#include "Shaders\Compiled\IBLSampler_PS_SpecularCubeMap.inc"
#include "Shaders\Compiled\IBLSampler_PS_DiffuseCubeMap.inc"

    struct Constants
    {
    };

    //static_assert(sizeof(Constants) == sizeof(UINT) * 8, "Constant buffer size mismatch");

    inline void RunComputeShader(ID3D11DeviceContext* pContext,
        ID3D11ComputeShader* shader,
        ID3D11ShaderResourceView** pSRVs,
        UINT srvCount,
        ID3D11Buffer* pCB,
        ID3D11UnorderedAccessView* pUAV,
        UINT X)
    {
        // Force UAV to nullptr before setting SRV since we are swapping buffers
        ID3D11UnorderedAccessView* nullUAV = nullptr;
        pContext->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

        pContext->CSSetShader(shader, nullptr, 0);
        pContext->CSSetShaderResources(0, srvCount, pSRVs);
        pContext->CSSetUnorderedAccessViews(0, 1, &pUAV, nullptr);
        pContext->CSSetConstantBuffers(0, 1, &pCB);
        pContext->Dispatch(X, 1, 1);
    }

    inline void ResetContext(ID3D11DeviceContext* pContext)
    {
        ID3D11UnorderedAccessView* nullUAV = nullptr;
        pContext->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

        ID3D11ShaderResourceView* nullSRV[3] = { nullptr, nullptr, nullptr };
        pContext->CSSetShaderResources(0, 3, nullSRV);

        ID3D11Buffer* nullBuffer[1] = { nullptr };
        pContext->CSSetConstantBuffers(0, 1, nullBuffer);
    }
};

IBLCompute::IBLCompute() noexcept
{
}


//-------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT IBLCompute::Initialize(ID3D11Device* pDevice)
{
    if (!pDevice)
        return E_INVALIDARG;

    // Check for DirectCompute support
    D3D_FEATURE_LEVEL fl = pDevice->GetFeatureLevel();

    if (fl < D3D_FEATURE_LEVEL_10_0)
    {
        // DirectCompute not supported on Feature Level 9.x hardware
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }

    if (fl < D3D_FEATURE_LEVEL_11_0)
    {
        // DirectCompute support on Feature Level 10.x hardware is optional, and this function needs it
        D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hwopts;
        HRESULT hr = pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &hwopts, sizeof(hwopts));
        if (FAILED(hr))
        {
            memset(&hwopts, 0, sizeof(hwopts));
        }

        if (!hwopts.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x)
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
    }

    // Save a device reference and obtain immediate context
    m_device = pDevice;

    pDevice->GetImmediateContext(m_context.ReleaseAndGetAddressOf());
    assert(m_context);

    //--- Create shaders -----------------------------------------

    HRESULT hr = pDevice->CreateVertexShader(
        IBLSampler_VS_CubeMap, sizeof(IBLSampler_VS_CubeMap), nullptr,
        m_CubeMapVS.ReleaseAndGetAddressOf());
    if (FAILED(hr))
        return hr;

    hr = pDevice->CreateGeometryShader(
        IBLSampler_GS_CubeMap, sizeof(IBLSampler_GS_CubeMap), nullptr,
        m_CubeMapGS.ReleaseAndGetAddressOf());
    if (FAILED(hr))
        return hr;

    hr = pDevice->CreatePixelShader(
        IBLSampler_PS_SpecularCubeMap, sizeof(IBLSampler_PS_SpecularCubeMap), nullptr,
        m_SpecularCubeMapPS.ReleaseAndGetAddressOf());
    if (FAILED(hr))
        return hr;

    hr = pDevice->CreatePixelShader(
        IBLSampler_PS_DiffuseCubeMap, sizeof(IBLSampler_PS_DiffuseCubeMap), nullptr,
        m_DiffuseCubeMapPS.ReleaseAndGetAddressOf());
    if (FAILED(hr))
        return hr;

    return S_OK;
}


//-------------------------------------------------------------------------------------

HRESULT IBLCompute::GenerateSpecularMap(const Image& srcImage, const Image& destImage)
{
    return E_NOTIMPL;
}

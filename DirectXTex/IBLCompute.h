//-------------------------------------------------------------------------------------
// BCDirectCompute.h
//  
// Direct3D 11 Compute Shader BC Compressor
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#pragma once

namespace DirectX
{

    class IBLCompute
    {
    public:
        IBLCompute() noexcept;

        HRESULT Initialize(_In_ ID3D11Device* pDevice);

        HRESULT GenerateSpecularMap(const Image& srcImage, const Image& destImage);

    private:
        Microsoft::WRL::ComPtr<ID3D11Device>                m_device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext>         m_context;

        Microsoft::WRL::ComPtr<ID3D11Buffer>                m_srcBuffer;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_srcTexture;

        Microsoft::WRL::ComPtr<ID3D11Buffer>                m_output;
        Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>   m_outputUAV;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                m_constBuffer;

        // Compute shader library
        Microsoft::WRL::ComPtr<ID3D11VertexShader>        m_CubeMapVS;
        Microsoft::WRL::ComPtr<ID3D11GeometryShader>      m_CubeMapGS;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>         m_SpecularCubeMapPS;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>         m_DiffuseCubeMapPS;
    };

} // namespace

TextureCube SourceCubeMap : register(t0);

cbuffer PerEnvironmentMap : register(b0)
{
    uint CurrentMipLevel;
};

SamplerState inputSampler : register(s0);

struct VertexShaderData
{
    float4 position : POSITION;
};

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

struct PixelShaderOut
{
    float4 color : SV_TARGET0;
};

VertexShaderData vs(VertexShaderData data)
{
    return data;
}

float3 getVertexDirection(int face, float u, float v)
{
    switch (face)
    {
        case 0:
            return float3(+1, +v, -u);
        case 1:
            return float3(-1, +v, +u);
        case 2:
            return float3(+u, +1, -v);
        case 3:
            return float3(+u, -1, +v);
        case 4:
            return float3(+u, +v, +1);
        case 5:
            return float3(-u, +v, -1);
        default:
            return 0;
    }
}

[maxvertexcount(18)]
void gs(triangle VertexShaderData input[3],
         inout TriangleStream<PixelShaderInput> outStream)
{
    for (int f = 0; f < 6; ++f)
    {
        PixelShaderInput output;
        output.RTIndex = f;
        for (int v = 0; v < 3; v++)
        {
            float4 p = input[v].position;
            //output.position = mul(input[v].position, CubeFaceViewProj[f]);
            //output.normal = mul(float4(0, 0, 1, 0), CubeFaceViewProj[f]);
            output.position = p;
            output.normal = getVertexDirection(f, p.x, p.y);
            outStream.Append(output);
        }
        outStream.RestartStrip();
    }
}

float3 fix_cube_lookup(float3 v, float cubeSize)
{
    float eps = 1 - 1e-6;

    float M = max(max(abs(v.x), abs(v.y)), abs(v.z));

    float scale = cubeSize > 1 ? cubeSize / (cubeSize - 1) : 1;

    v /= M;
    if (abs(v.x) <= eps)
        v.x *= scale;
    if (abs(v.y) <= eps)
        v.y *= scale;
    if (abs(v.z) <= eps)
        v.z *= scale;
    return v;
}

PixelShaderOut ps(PixelShaderInput psi)
{
    PixelShaderOut output;
    //output.color = float4(psi.normal * 0.5 + 0.5, 1);
    //output.color = SourceCubeMap.SampleLevel(inputSampler, psi.normal, CurrentMipLevel);
    //return output;

    uint width, height, mipCount;
    SourceCubeMap.GetDimensions(CurrentMipLevel, width, height, mipCount);

    float4 outColor;

    if (width <= 1)
    {
        float4 c1 = SourceCubeMap.SampleLevel(inputSampler, float3(+1, 0, 0), CurrentMipLevel);
        float4 c2 = SourceCubeMap.SampleLevel(inputSampler, float3(-1, 0, 0), CurrentMipLevel);
        float4 c3 = SourceCubeMap.SampleLevel(inputSampler, float3(0, +1, 0), CurrentMipLevel);
        float4 c4 = SourceCubeMap.SampleLevel(inputSampler, float3(0, -1, 0), CurrentMipLevel);
        float4 c5 = SourceCubeMap.SampleLevel(inputSampler, float3(0, 0, +1), CurrentMipLevel);
        float4 c6 = SourceCubeMap.SampleLevel(inputSampler, float3(0, 0, -1), CurrentMipLevel);
        outColor = (c1 + c2 + c3 + c4 + c5 + c6) / 6.0;
    }
    else
    {
        float3 envDiffuseCoord = fix_cube_lookup(psi.normal, width);
        outColor = SourceCubeMap.SampleLevel(inputSampler, envDiffuseCoord, CurrentMipLevel);
    }

    // Gamma correct
    // outColor.rgb = pow(outColor.rgb, 1.0f / 2.2f);

    output.color = outColor;

    return output;
}

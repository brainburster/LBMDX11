#include "vs_ps_header.hlsli"
Texture2D<float4> quantities : register(t0);

SamplerState quantities_sampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

float4 main(VsOut vs_out) : SV_TARGET
{
    float4 data = quantities.Sample(quantities_sampler, vs_out.uv);
    const float4 back_color = float4(vs_out.uv, 0.f, 1.0f);
    const float4 wall_color = float4(0.1f, 0.1f, 0.1f, 1.0f);
    const float4 color1 = float4(1.0f, 0.1f, 0.1f, 1.0f);
    const float4 color2 = float4(0.1f, 1.0f, 0.1f, 1.0f);
    [branch] switch (data.w)
    {
        case 1.0f:
            return color1;
        case 2.0f:
            return color2;
        case 3.0f:
            return wall_color;
        default:
            return back_color;
    }
}

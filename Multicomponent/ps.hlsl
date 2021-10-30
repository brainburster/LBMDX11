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
    return float4(vs_out.uv, -data.w, 1.0f);
}

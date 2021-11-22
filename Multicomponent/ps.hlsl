#include "vs_ps_header.hlsli"
Texture2D<float4> srv_display : register(t0);

SamplerState srv_display_sampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

float4 main(VsOut vs_out) : SV_TARGET
{
    float4 data = srv_display.Sample(srv_display_sampler, vs_out.uv);
    const float4 wall_color = float4(0.4f, 0.4f, 0.4f, 1.f) /** (1.5f+data.w)*/;
    const float4 color = float4(float3(.9f,.9f,.9f)-data.xyz, 1.0f);

    return lerp(wall_color, color, step(data.w, 0));
}

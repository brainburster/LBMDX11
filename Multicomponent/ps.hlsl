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
    const float4 back_color = float4(vs_out.uv, 0.f, 1.0f);
    const float4 wall_color = float4(0.2f, 0.2f, 0.2f, 1.0f);
    const float4 color = float4(float3(.8f,.8f,.8f)-data.xyz, 1.0f);
    //return color3;
    if (data.w < 0.0f)
    {
        return wall_color;
    }
    return color;
}

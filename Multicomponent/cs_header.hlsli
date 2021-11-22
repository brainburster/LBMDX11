RWTexture2DArray<float> f_in[3] : register(u0);
RWTexture2DArray<float> f_out[3] : register(u3);
RWTexture2D<float4> uav_display : register(u6);

cbuffer PerFrame : register(b0)
{
    uint num_control_point;
};

struct ControlPoint
{
    float2 pos;
    float dis;
    float data;
};

StructuredBuffer<ControlPoint> control_points : register(t0);

static const float3 rho0 = { 0.25f, 0.1f, 0.01f };

RWTexture2DArray<float> f_in[2] : register(u0);
RWTexture2DArray<float> f_out[2] : register(u2);
RWTexture2D<float4> uav_display : register(u4);

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

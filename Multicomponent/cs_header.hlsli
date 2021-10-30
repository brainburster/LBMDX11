RWTexture2DArray<float4> f_in_0 : register(u0);
RWTexture2DArray<float4> f_in_1 : register(u1);
RWTexture2DArray<float4> f_in_2 : register(u2);
RWTexture2DArray<float4> f_out_0 : register(u3);
RWTexture2DArray<float4> f_out_1 : register(u4);
RWTexture2DArray<float4> f_out_2 : register(u5);
RWTexture2D<float4> quantities : register(u6);

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

RWTexture2DArray<float> f_in[3] : register(u0);
RWTexture2DArray<float> f_out[3] : register(u3);
RWTexture2D<float4> uav_display : register(u6);

cbuffer SimSetting : register(b0)
{
    uint width;
    uint height;
    uint grid_size;
}

cbuffer PerFrameOnMouseDown : register(b1)
{
    uint num_control_point;
};

cbuffer DisplaySetting : register(b2)
{
    uint show_air;
    uint velocitymode;
    uint vorticitymode;
    uint forcemode;
}

struct ControlPoint
{
    float2 pos;
    float dis;
    float data;
};

StructuredBuffer<ControlPoint> control_points : register(t0);

static const float3 rho0 = { .0001f, 0.00001f, 0.000001f };
//static const int height = 1024;
//static const int width = 1024;
//static const int grid_size = 8;
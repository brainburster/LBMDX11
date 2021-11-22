#include "cs_header.hlsli"
#include "cs_lbm_header.hlsli"

static const int2 c2[25] =
{
    { 0, 0 },
    { 1, 0 },{ 0, -1 },{ -1, 0 },{ 0, 1 },
    { 1, -1 }, { -1, -1 }, { -1, 1 }, { 1, 1 },
    { 2, 0 }, { 0, -2 }, { -2, 0 }, { 0, 2 },
    { 1, -2 }, { -1, -2 }, { -1, 2 }, { 1, 2 },
    { 2, -1 }, { -2, -1 }, { -2, 1 }, { 2, 1 },
    { 2, -2 }, { -2, -2 }, { -2, 2 }, { 2, 2 },
};

float is_wall(uint2 pos)
{
    if (pos.y > (600 / 4 - 1) && pos.y < (0u - 1u) || pos.x > (800 / 4 - 1))
    {
        return 0.6f;
    }
    return uav_display[pos].w > 0.0f ? 0.9f : 0.f;
}

static const float w2[9] =
{
    0, 1 / 21.f, 4 / 45.f, 0, 1 / 60.f, 2 / 315.f, 0, 0, 1 / 5040.f
};

static const float4x4 G =
{
    -0.32f, .08f, 0.01f, -.2f,
    .08f, -0.12f, 0.01f, -.08f,
     0.01f, 0.01f, 0.01f, 0.0f,
     -.2f, -.08f, 0.0f, 0,
};

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos = DTid.xy;
    uint i = 1;
    uint j = 0;
    
    [unroll]
    for (j = 0; j < 3; j++)
    {
        float psi = f_out[j][uint3(pos, 9)];
        float rho = f_in[j][uint3(pos, 9)];
        float2 F = { 0.f, 0.f };
        for (i = 1; i < 25; i++)
        {
            uint2 pos2 = pos + c2[i];
            float _psi0 = f_out[0][uint3(pos2, 9)];
            float _psi1 = f_out[1][uint3(pos2, 9)];
            float _psi2 = f_out[2][uint3(pos2, 9)];
            float _psi3 = is_wall(pos2);
            F += (G[j][0] * _psi0 + G[j][1] * _psi1 + G[j][2] * _psi2 + G[j][3] * _psi3)
            * c2[i] * 
            w2[c2[i].x * c2[i].x + c2[i].y * c2[i].y];
        }
        F *= -psi*5.8f;
        F += float2(0.f, 0.001f) * (rho + 1e-20f);
        f_out[j][uint3(pos, 10)] = F.x;
        f_out[j][uint3(pos, 11)] = F.y;
    }
}

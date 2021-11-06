#include "cs_header.hlsli"

// d2q9 velocity sets:
//
//    6    2     5
//      ↖ ↑ ↗    
//    3 ←  0 → 1
//      ↙ ↓ ↘
//    7    4     8

static const int2 v[9] = { { 0, 0 }, { 1, 0 }, { 0, -1 }, { -1, 0 }, { 0, 1 }, { 1, -1 }, { -1, -1 }, { -1, 1 }, { 1, 1 } };
static const float w[9] =
{
    4.f / 9.f,
    1.f / 9.f, 1.f / 9.f, 1.f / 9.f, 1.f / 9.f,
    1.f / 36.f, 1.f / 36.f, 1.f / 36.f, 1.f / 36.f
};
static const uint oppo[9] = {
    0, 3, 4, 1, 2, 7, 8, 5, 6
};
static const float k = 1.2f;

bool is_wall(uint2 pos)
{
    if (pos.x < 0 || pos.y > 599 || pos.y < 0 || pos.x > 799)
    {
        return true;
    }
    return uav_display[pos].w < 0.0;
}

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    const uint2 pos = DTid.xy;
    float f0[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint i = 0;
    //float f1[9] = { };
    //传输/反弹
    bool flag1 = is_wall(pos);
    f0[0] = f_in[0][uint3(pos, 0)];
    for (i = 1; i < 9; i++)
    {
        uint2 pos_2 = pos + v[oppo[i]];
        bool flag2 = is_wall(pos_2);
        if (flag1 && !flag2)
        {
            f0[i] = 0.0;
        }
        else if (!flag1 && flag2)
        {
            f0[i] = f_in[0][uint3(pos, oppo[i])];
            f0[i] += f_in[0][uint3(pos_2, i)];
        }
        else
        {
            f0[i] = f_in[0][uint3(pos_2, i)];
        }
    }
    
    //计算受力
    //...
    
    
    //预计算
    float rho0 = f0[0] + f0[1] + f0[2] + f0[3] + f0[4] + f0[5] + f0[6] + f0[7] + f0[8];
    float2 u0 = { 0.f, 0.f };
    
    [unroll]
    for (i = 1; i < 9; i++)
    {
        u0 += f0[0] * v[0];
    }
    u0 /= rho0;
    u0.y += 0.005;
    //float rho1 = f1[0] + f1[1] + f1[2] + f1[3] + f1[4] + f1[5] + f1[6] + f1[7] + f1[8];
    //f_in[0][uint3(pos.xy, 9)] = rho0;
    //f_in[1][uint3(pos.xy, 9)] = rho1;
    
    
    //碰撞
    float u_sqr = 1.5 * (u0.x * u0.x + u0.y * u0.y);
    for (i = 0; i < 9; i++)
    {
        float vu = 3.0 * (v[i].x * u0.x + v[i].y * u0.y);
        float f_eq = rho0 * w[i] * (1.0 + vu + 0.5 * vu * vu - u_sqr);
        f0[i] = (1.0 - k) * f0[i] + k * f_eq;
    }
    
    for (i = 0; i < 9; i++)
    {
        f_in[0][uint3(pos, i)] = f0[i];
    }
    
    //显示 
    uav_display[pos] = float4(rho0, rho0, rho0, uav_display[pos].w);
}

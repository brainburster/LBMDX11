#include "cs_header.hlsli"
#include "cs_lbm_header.hlsli"

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos = DTid.xy;
    float f0[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint i = 0;
    [unroll]
    for (i = 0; i < 9; i++)
    {
        f0[i] = f_in[0][uint3(pos, i)];
    }
    
    //更新物理量
    float rho0 = f0[0] + f0[1] + f0[2] + f0[3] + f0[4] + f0[5] + f0[6] + f0[7] + f0[8];
    float2 u0 = { 0.f, 0.f };
    [unroll]
    for (i = 1; i < 9; i++)
    {
        u0 += f0[0] * c[0];
    }
    u0 /= rho0;
     //计算有效密度
    float psi0 = 1.f - exp(-rho0 / 1.f);
    
    f_out[0][uint3(pos, 9)] = psi0;
    f_in[0][uint3(pos, 9)] = rho0;
    f_in[0][uint3(pos, 10)] = u0.x;
    f_in[0][uint3(pos, 11)] = u0.y;
}
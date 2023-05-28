#include "cs_header.hlsli"
#include "cs_lbm_header.hlsli"

[numthreads(32, 32, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    const uint2 pos = DTid.xy;

    float f[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint i = 0;
    uint j = 0;
    
    [unroll]
    for (j = 0; j < 3; j++)
    {
        float2 u = { 0.f, 0.f };
        float rho = 0.f;
        float psi = 0.f;
        [unroll]
        for (i = 0; i < 9; i++)
        {
            f[i] = f_in[j][uint3(pos, i)];
        }

        [unroll]
        for (i = 1; i < 9; i++)
        {
            u += f[i] * c[i];
        }
        
        rho = f[0] + f[1] + f[2] + f[3] + f[4] + f[5] + f[6] + f[7] + f[8];
        u = rho ? u / rho : 0;
        psi = sign(rho) * rho0[j] * (1.f - exp(-abs(rho) / rho0[j]) - (j == 2 ? 0. : clamp(0.1f * (abs(rho) / rho0[j] - 1.f), 0.f, 1.f)));
        //psi = sign(rho) * rho0[j]  * (1.f - exp(-abs(rho) / rho0[j]));
        //psi = rho0[j]  * (1.f - exp(-rho / rho0[j]));
        
        f_out[j][uint3(pos, 9)] = psi;
        f_in[j][uint3(pos, 9)] = rho;
        f_in[j][uint3(pos, 10)] = u.x;
        f_in[j][uint3(pos, 11)] = u.y;
    }
}

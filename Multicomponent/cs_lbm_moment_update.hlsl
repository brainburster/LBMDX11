#include "cs_header.hlsli"
#include "cs_lbm_header.hlsli"

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos = DTid.xy;
    float f[2][9] = {
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 } ,
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 } 
    };
    float rho[2] = { 0.f, 0.f };
    float2 u[2] = { { 0.f, 0.f }, { 0.f, 0.f } };
    float psi[2] = { 0.f, 0.f };
    uint i = 0;
    uint j = 0;
    const float rho0[2] = { 1.f, 0.1f };
    
    [unroll]
    for (j = 0; j < 2; j++)
    {
        [unroll]
        for (i = 0; i < 9; i++)
        {
            f[j][i] = f_in[j][uint3(pos, i)];
        }

        [unroll]
        for (i = 1; i < 9; i++)
        {
            u[j] += f[j][0] * c[0];
        }
        
        rho[j] = f[j][0] + f[j][1] + f[j][2] + f[j][3] + f[j][4] + f[j][5] + f[j][6] + f[j][7] + f[j][8];
    
        u[j] /= rho[j];   

        psi[j] = 1.f - exp(-rho[j] / rho0[j]);
        f_out[j][uint3(pos, 9)] = psi[j];
        f_in[j][uint3(pos, 9)] = rho[j];
        f_in[j][uint3(pos, 10)] = u[j].x;
        f_in[j][uint3(pos, 11)] = u[j].y;
    }
}
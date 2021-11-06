#include "cs_header.hlsli"
#include "cs_lbm_header.hlsli"

[numthreads(16, 16, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    const uint2 pos = DTid.xy;
    float rho0 = f_in[0][uint3(pos, 9)];
    float2 u0 = { 
        f_in[0][uint3(pos, 10)],
        f_in[0][uint3(pos, 11)] 
    };
    float2 F0 =
    {
        f_out[0][uint3(pos, 10)],
        f_out[0][uint3(pos, 11)]
    };
    float f0[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint i = 0;
    for (i = 0; i < 9; i++)
    {
        f0[i] = f_in[0][uint3(pos, i)];
    }
    if (rho0 > 1e-8f)
    {
        u0 += 0.5f * F0 / rho0;
    }
    if (length(u0) > 0.57735)
    {
        u0=normalize(u0) * 0.57735;
    }
    
    float u_sqr = 1.5 * (u0.x * u0.x + u0.y * u0.y);
    for (i = 0; i < 9; i++)
    {
        float cu = 3.0 * (c[i].x * u0.x + c[i].y * u0.y);
        float f_eq = rho0 * w[i] * (1.0f + cu - u_sqr + 0.5f * cu * cu);
        f0[i] = (1.0 - k) * f0[i] + k * f_eq;
        //if (rho0 > 1e-16f)
        //{
        //    float Si = (1 - 0.5 * k) * w[i] * dot(((c[i] - u0) / 3 + dot(c[i], u0) * c[i] / 9), F0 / rho0);
        //    f0[i] += Si;
        //}
    }
    
    for (i = 0; i < 9; i++)
    {
        f_out[0][uint3(pos, i)] = f0[i];
    }
}